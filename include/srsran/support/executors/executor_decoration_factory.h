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

#include "srsran/support/executors/detail/task_executor_utils.h"
#include "srsran/support/tracing/event_tracing.h"
#include <chrono>
#include <memory>
#include <optional>
#include <string>

namespace srsran {

class executor_metrics_backend;

/// Description of the decorators to be applied to an executor.
struct execution_decoration_config {
  struct sync_option {};
  struct throttle_option {
    /// Number of tasks pending after which the caller to the executor starts being throttled.
    unsigned nof_task_threshold;
  };
  struct trace_option {
    /// Name of the executor to be traced.
    std::string name;
  };
  struct metrics_option {
    /// Name of the executor for which metrics are to be reported.
    std::string name;
    /// Executor metrics backend.
    executor_metrics_backend& backend;
    /// Whether to use metric captures for tracing.
    bool tracing_enabled = false;
    /// Tracer.
    file_event_tracer<true>* tracer = nullptr;

    metrics_option(std::string               name_,
                   executor_metrics_backend& backend_,
                   bool                      tracing_enabled_,
                   file_event_tracer<true>*  tracer_ = nullptr) :
      name(std::move(name_)), backend(backend_), tracing_enabled(tracing_enabled_), tracer(tracer_)
    {
    }
  };

  /// If set, the executor will block the caller until the task is executed.
  std::optional<sync_option> sync;
  /// \brief If set, the executor will throttle the execute/defer caller if the number of pending tasks exceeds the
  /// specified threshold.
  std::optional<throttle_option> throttle;
  /// \brief If set, the executor will collect metrics on the task execution latencies.
  /// \remark This decorator should be only used with sequential executors (e.g. strands, single threads).
  std::optional<metrics_option> metrics;
  /// If set, the executor will trace the task execution latencies using the specified tracer.
  std::optional<trace_option> trace;
};

/// \brief Creates an executor decorator that applies the specified policies to the given executor.
std::unique_ptr<task_executor> decorate_executor(std::unique_ptr<task_executor>     exec,
                                                 const execution_decoration_config& config);
std::unique_ptr<task_executor> decorate_executor(task_executor& exec, const execution_decoration_config& config);

} // namespace srsran
