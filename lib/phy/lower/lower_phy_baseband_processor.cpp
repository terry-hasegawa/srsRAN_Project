/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "lower_phy_baseband_processor.h"
#include "srsran/adt/interval.h"
#include "srsran/instrumentation/traces/ru_traces.h"
#include "srsran/ran/slot_point.h"

using namespace srsran;

lower_phy_baseband_processor::lower_phy_baseband_processor(const lower_phy_baseband_processor::configuration& config) :
  nof_samples_per_super_frame(config.srate.to_kHz() * NOF_SFNS * NOF_SUBFRAMES_PER_FRAME),
  rx_buffer_size(config.rx_buffer_size),
  slot_duration(1000 / pow2(to_numerology_value(config.scs))),
  cpu_throttling_time(static_cast<uint64_t>(config.system_time_throttling * 1e6) /
                      pow2(to_numerology_value(config.scs))),
  rx_executor(*config.rx_task_executor),
  tx_executor(*config.tx_task_executor),
  uplink_executor(*config.ul_task_executor),
  receiver(*config.receiver),
  transmitter(*config.transmitter),
  uplink_processor(*config.ul_bb_proc),
  downlink_processor(*config.dl_bb_proc),
  rx_buffers(config.nof_rx_buffers),
  tx_time_offset(config.tx_time_offset),
  rx_to_tx_max_delay(config.rx_to_tx_max_delay),
  tx_state(config.stop_nof_slots),
  rx_state(config.stop_nof_slots)
{
  static constexpr interval<float> system_time_throttling_range(0, 1);

  srsran_assert(rx_buffer_size, "Invalid buffer size.");
  srsran_assert(config.rx_task_executor, "Invalid receive task executor.");
  srsran_assert(system_time_throttling_range.contains(config.system_time_throttling),
                "System time throttling (i.e., {}) is out of the range {}.",
                config.system_time_throttling,
                system_time_throttling_range);
  srsran_assert(config.tx_task_executor, "Invalid transmit task executor.");
  srsran_assert(config.ul_task_executor, "Invalid uplink task executor.");
  srsran_assert(config.receiver, "Invalid baseband receiver.");
  srsran_assert(config.transmitter, "Invalid baseband transmitter.");
  srsran_assert(config.ul_bb_proc, "Invalid uplink processor.");
  srsran_assert(config.dl_bb_proc, "Invalid downlink processor.");
  srsran_assert(config.nof_rx_ports != 0, "Invalid number of receive ports.");
  srsran_assert(config.nof_tx_ports != 0, "Invalid number of transmit ports.");

  // Create queue of receive buffers.
  while (!rx_buffers.full()) {
    rx_buffers.push_blocking(std::make_unique<baseband_gateway_buffer_dynamic>(config.nof_rx_ports, rx_buffer_size));
  }
}

void lower_phy_baseband_processor::start(baseband_gateway_timestamp init_time, baseband_gateway_timestamp sfn0_ref_time)
{
  // If it is required to start with system frame number 0, then set a time offset to start an SFN earlier.
  start_time_sfn0   = sfn0_ref_time;
  last_rx_timestamp = init_time;

  rx_state.start();
  report_fatal_error_if_not(rx_executor.defer([this]() { ul_process(); }), "Failed to execute initial uplink task.");

  tx_state.start();
  report_fatal_error_if_not(tx_executor.defer([this, init_time]() { dl_process(init_time + rx_to_tx_max_delay); }),
                            "Failed to execute initial downlink task.");
}

void lower_phy_baseband_processor::stop()
{
  rx_state.request_stop();
  tx_state.request_stop();
  rx_state.wait_stop();
  tx_state.wait_stop();
}

void lower_phy_baseband_processor::dl_process(baseband_gateway_timestamp timestamp)
{
  // Check if it is running, notify stop and return without enqueueing more tasks.
  if (!tx_state.on_process()) {
    return;
  }

  // Throttling mechanism to keep a maximum latency of one millisecond in the transmit buffer based on the latest
  // received timestamp.
  {
    // Calculate maximum waiting time to avoid deadlock.
    std::chrono::microseconds timeout_duration = 2 * slot_duration;
    // Maximum time point to wait for.
    std::chrono::time_point<std::chrono::steady_clock> wait_until_tp =
        std::chrono::steady_clock::now() + timeout_duration;
    // Wait until one of these conditions is met:
    // - The reception timestamp reaches the desired value;
    // - The system time reaches the maximum waiting time; or
    // - The lower PHY was stopped.
    while ((timestamp > (last_rx_timestamp.load(std::memory_order_acquire) + rx_to_tx_max_delay)) &&
           (std::chrono::steady_clock::now() < wait_until_tp)) {
      std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
  }

  // Throttling mechanism to slow down the baseband processing.
  if ((cpu_throttling_time.count() > 0) && (last_tx_time.has_value())) {
    std::chrono::time_point<std::chrono::high_resolution_clock> now     = std::chrono::high_resolution_clock::now();
    std::chrono::nanoseconds                                    elapsed = now - *last_tx_time;

    if (elapsed < cpu_throttling_time) {
      std::this_thread::sleep_until(*last_tx_time + cpu_throttling_time);
    }
  }
  last_tx_time.emplace(std::chrono::high_resolution_clock::now());

  // Process downlink buffer.
  downlink_processor_baseband::processing_result result =
      downlink_processor.process(apply_timestamp_sfn0_ref(timestamp));
  srsran_assert(result.buffer, "The buffer must be valid.");

  // Set transmission timestamp.
  result.metadata.ts = timestamp + tx_time_offset;

  // Enqueue transmission.
  trace_point tx_tp = ru_tracer.now();

  // Transmit buffer.
  transmitter.transmit(result.buffer->get_reader(), result.metadata);

  ru_tracer << trace_event("transmit_baseband", tx_tp);

  // Enqueue DL process task.
  report_fatal_error_if_not(tx_executor.defer([this, new_timestamp = timestamp + result.buffer->get_nof_samples()]() {
    dl_process(new_timestamp);
  }),
                            "Failed to execute downlink processing task");
}

void lower_phy_baseband_processor::ul_process()
{
  // Check if it is running, notify stop and return without enqueueing more tasks.
  if (!rx_state.on_process()) {
    return;
  }

  // Get receive buffer.
  std::unique_ptr<baseband_gateway_buffer_dynamic> rx_buffer = rx_buffers.pop_blocking();

  // Receive baseband.
  trace_point                         tp          = ru_tracer.now();
  baseband_gateway_receiver::metadata rx_metadata = receiver.receive(rx_buffer->get_writer());
  ru_tracer << trace_event("receive_baseband", tp);

  // Update last timestamp.
  last_rx_timestamp.store(rx_metadata.ts + rx_buffer->get_nof_samples(), std::memory_order_release);

  // Queue uplink buffer processing.
  report_fatal_error_if_not(uplink_executor.defer([this, ul_buffer = std::move(rx_buffer), rx_metadata]() mutable {
    trace_point ul_tp = ru_tracer.now();

    // Process UL.
    uplink_processor.process(ul_buffer->get_reader(), apply_timestamp_sfn0_ref(rx_metadata.ts));

    // Return buffer to receive.
    rx_buffers.push_blocking(std::move(ul_buffer));

    ru_tracer << trace_event("uplink_baseband", ul_tp);
  }),
                            "Failed to execute uplink processing task.");

  // Enqueue next iteration if it is running.
  report_fatal_error_if_not(rx_executor.defer([this]() { ul_process(); }), "Failed to execute receive task.");
}
