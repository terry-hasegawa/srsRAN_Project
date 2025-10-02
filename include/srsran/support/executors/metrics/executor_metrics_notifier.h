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

namespace srsran {

struct executor_metrics;

/// Notifier interface used by the executor metrics backend to report metrics.
class executor_metrics_notifier
{
public:
  virtual ~executor_metrics_notifier() = default;

  /// Notifies new executor metrics.
  virtual void on_new_metrics(const executor_metrics& metrics) = 0;
};

} // namespace srsran
