/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "srsran/support/io/io_broker.h"
#include "srsran/support/synchronization/sync_event.h"

namespace srsran {

class timer_manager;

/// \brief Interface for a timer source.
class io_timer_source
{
public:
  io_timer_source(timer_manager&            tick_sink_,
                  io_broker&                broker_,
                  task_executor&            executor,
                  std::chrono::milliseconds tick_period,
                  bool                      auto_start = true);

  /// This call blocks until the last tick is processed.
  ~io_timer_source();

  /// Resume ticking in case it was previously halted.
  void resume();

  /// \brief Request the timer source to stop ticking.
  /// Note: This call does not block, so a tick might take place after this call.
  void request_stop();

private:
  enum class state_t : uint8_t { idle, stopped, started };

  void create_subscriber(scoped_sync_token token);
  void destroy_subscriber();

  void read_time(int raw_fd);

  const std::chrono::milliseconds tick_period;
  timer_manager&                  tick_sink;
  io_broker&                      broker;
  task_executor&                  tick_exec;
  srslog::basic_logger&           logger;
  io_broker::subscriber           io_sub;

  // Current state of the timer source.
  std::atomic<state_t> cur_state;

  // Synchronization primitive to stop the timer source.
  sync_event        stop_flag;
  scoped_sync_token read_time_token;
};

} // namespace srsran
