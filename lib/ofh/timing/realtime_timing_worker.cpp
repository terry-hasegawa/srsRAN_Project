/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "realtime_timing_worker.h"
#include "../support/logger_utils.h"
#include "srsran/instrumentation/traces/ofh_traces.h"
#include "srsran/ofh/timing/ofh_ota_symbol_boundary_notifier.h"
#include "srsran/support/rtsan.h"
#include <future>
#include <thread>

using namespace srsran;
using namespace ofh;

realtime_timing_worker::realtime_timing_worker(srslog::basic_logger&      logger_,
                                               task_executor&             executor_,
                                               const realtime_worker_cfg& cfg) :
  logger(logger_),
  executor(executor_),
  scs(cfg.scs),
  nof_symbols_per_slot(get_nsymb_per_slot(cfg.cp)),
  nof_symbols_per_sec(nof_symbols_per_slot * get_nof_slots_per_subframe(scs) * NOF_SUBFRAMES_PER_FRAME * 100),
  symbol_duration(1e9 / nof_symbols_per_sec),
  sleep_time(std::chrono::duration_cast<std::chrono::nanoseconds>(symbol_duration) / 15),
  enable_log_warnings_for_lates(cfg.enable_log_warnings_for_lates)
{
  // The GPS time epoch starts on 1980.1.6 so make sure that the system time is set after this date.
  // For simplicity reasons, only allow dates after 1981.
  ::time_t tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  ::tm     utc_time;
  ::gmtime_r(&tt, &utc_time);
  int current_year = utc_time.tm_year + 1900;
  report_error_if_not(current_year >= 1981,
                      "The Open FrontHaul standard uses GPS time for synchronization. Make sure that system time is "
                      "set after the year 1981 since the GPS time epoch starts on 1980.1.6");

  gps_clock::gps_offset = std::chrono::nanoseconds(static_cast<unsigned>(std::round(cfg.gps_Alpha / 1.2288))) +
                          std::chrono::milliseconds(cfg.gps_Beta * 10) +
                          std::chrono::seconds(gps_clock::UNIX_TO_GPS_SECONDS_OFFSET);
}

/// Returns the symbol index inside a second.
static unsigned get_symbol_index(std::chrono::nanoseconds                 fractional_ns,
                                 std::chrono::duration<double, std::nano> symbol_duration)
{
  // Perform operation with enough precision to avoid rounding errors when the amount of fractional nanoseconds is big.
  return fractional_ns / symbol_duration;
}

void realtime_timing_worker::start()
{
  logger.info("Starting the realtime timing worker");

  stop_manager.reset();

  std::promise<void> p;
  std::future<void>  fut = p.get_future();

  if (!executor.defer([this, &p, token = stop_manager.get_token()]() {
        // Signal start() caller thread that the operation is complete.
        p.set_value();

        auto now                  = gps_clock::now();
        auto ns_fraction          = calculate_ns_fraction_from(now);
        previous_symb_index       = get_symbol_index(ns_fraction, symbol_duration);
        previous_time_since_epoch = now.time_since_epoch().count();

        timing_loop();
      })) {
    report_fatal_error("Unable to start the realtime timing worker");
  }

  // Block waiting for timing executor to start.
  fut.wait();

  logger.info("Started the realtime timing worker");
}

void realtime_timing_worker::stop()
{
  logger.info("Requesting stop of the realtime timing worker");
  stop_manager.stop();

  // Clear the subscribed notifiers.
  ota_notifiers.clear();

  logger.info("Stopped the realtime timing worker");
}

void realtime_timing_worker::timing_loop() noexcept SRSRAN_RTSAN_NONBLOCKING
{
  while (SRSRAN_LIKELY(!stop_manager.stop_was_requested())) {
    poll();
  }
}

/// Returns the difference between cur and prev taking into account a potential wrap around of the values.
static unsigned circular_distance(unsigned cur, unsigned prev, unsigned size)
{
  return (cur >= prev) ? (cur - prev) : (size + cur - prev);
}

/// Calculates the current slot point given the GPS time.
static slot_point
calculate_slot_point(subcarrier_spacing scs, uint64_t gps_seconds, uint32_t fractional_us, uint32_t slot_duration_us)
{
  unsigned subframe_index      = (fractional_us / (SUBFRAME_DURATION_MSEC * 1000)) % NOF_SUBFRAMES_PER_FRAME;
  unsigned slot_subframe_index = (fractional_us / slot_duration_us) % get_nof_slots_per_subframe(scs);

  static constexpr unsigned NUM_FRAMES_PER_SEC = 100;
  static constexpr unsigned FRAME_DURATION_US  = SUBFRAME_DURATION_MSEC * NOF_SUBFRAMES_PER_FRAME * 1000;
  unsigned                  sfn = (gps_seconds * NUM_FRAMES_PER_SEC + (fractional_us / FRAME_DURATION_US)) % NOF_SFNS;

  return {to_numerology_value(scs), sfn, subframe_index, slot_subframe_index};
}

void realtime_timing_worker::poll()
{
  auto now         = gps_clock::now();
  auto ns_fraction = calculate_ns_fraction_from(now);

  auto     current_time_since_epoch = now.time_since_epoch().count();
  unsigned current_symbol_index     = get_symbol_index(ns_fraction, symbol_duration);
  unsigned delta                    = circular_distance(current_symbol_index, previous_symb_index, nof_symbols_per_sec);
  auto     delta_ns                 = current_time_since_epoch - previous_time_since_epoch;

  // The clock may jump backwards as it is continuously being adjusted by PTP, in this case, do not update the symbol
  // index.
  if (delta_ns < 0) {
    logger.info("Real-time timing worker detected PTP-synchronized time going backward by {}ns", -delta_ns);

    SRSRAN_RTSAN_SCOPED_DISABLER(d);
    std::this_thread::sleep_for(sleep_time);
    return;
  }
  // The values are updated after the condition above to avoid notifying again the symbols that have already been
  // notified (this is possible in an extreme case when the time went backward by more than 1 symbol duration).
  previous_time_since_epoch = current_time_since_epoch;
  previous_symb_index       = current_symbol_index;

  // Are we still in the same symbol as before?
  if (delta == 0) {
    SRSRAN_RTSAN_SCOPED_DISABLER(d);
    std::this_thread::sleep_for(sleep_time);
    return;
  }

  // Check if we have missed more than one symbol.
  if (SRSRAN_UNLIKELY(delta > 1)) {
    logger.info("Real-time timing worker woke up late, skipped '{}' symbols", delta);
    metrics_collector.update_metrics(delta);
  }
  if (SRSRAN_UNLIKELY(delta >= nof_symbols_per_slot)) {
    log_conditional_warning(
        logger,
        enable_log_warnings_for_lates,
        "Real-time timing worker woke up late, sleep time has been '{}us', or equivalently, '{}' symbols",
        std::chrono::duration_cast<std::chrono::microseconds>(delta * symbol_duration).count(),
        delta);
  }

  slot_symbol_point symbol_point(
      calculate_slot_point(scs,
                           std::chrono::time_point_cast<std::chrono::seconds>(now).time_since_epoch().count(),
                           std::chrono::duration_cast<std::chrono::microseconds>(ns_fraction).count(),
                           1000 / get_nof_slots_per_subframe(scs)),
      current_symbol_index % nof_symbols_per_slot,
      nof_symbols_per_slot);

  for (unsigned i = 0; i != delta; ++i) {
    unsigned skipped_symbol_id = delta - 1 - i;
    // Notify pending symbols from oldest to newest.
    notify_slot_symbol_point({symbol_point - skipped_symbol_id, std::chrono::system_clock::now()});
  }
}

void realtime_timing_worker::notify_slot_symbol_point(const slot_symbol_point_context& slot_context)
{
  ofh_tracer << instant_trace_event("ofh_timing_notify_symbol", instant_trace_event::cpu_scope::global);

  for (auto* notifier : ota_notifiers) {
    notifier->on_new_symbol(slot_context);
  }
}

void realtime_timing_worker::subscribe(span<ota_symbol_boundary_notifier*> notifiers)
{
  // The defer() call in start() synchronizes the contents of ota_notifiers with the worker thread.
  ota_notifiers.assign(notifiers.begin(), notifiers.end());
}
