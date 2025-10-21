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

#include "srsran/phy/metrics/phy_metrics_notifiers.h"
#include "srsran/phy/metrics/phy_metrics_reports.h"
#include "srsran/phy/upper/signal_processors/pusch/dmrs_pusch_estimator.h"
#include "srsran/support/resource_usage/scoped_resource_usage.h"
#include <memory>

namespace srsran {

/// PUSCH channel estimator metric decorator.
class phy_metrics_pusch_channel_estimator_decorator : public dmrs_pusch_estimator
{
public:
  /// Creates an PUSCH channel estimator from a base estimator instance and a metric notifier.
  phy_metrics_pusch_channel_estimator_decorator(std::unique_ptr<dmrs_pusch_estimator>    base_,
                                                pusch_channel_estimator_metric_notifier& metrics_notifier_) :
    base(std::move(base_)), metrics_notifier(metrics_notifier_)
  {
    srsran_assert(base, "Invalid estimator.");
  }

  // See interface for documentation.
  void estimate(channel_estimate&              estimate,
                dmrs_pusch_estimator_notifier& notifier,
                const resource_grid_reader&    grid,
                const configuration&           config) override
  {
    pusch_channel_estimator_metrics metrics;
    {
      // Use scoped resource usage class to measure CPU usage of this block.
      resource_usage_utils::scoped_resource_usage rusage_tracker(metrics.measurements);
      base->estimate(estimate, notifier, grid, config);
    }
    metrics.nof_prb = static_cast<unsigned>(config.rb_mask.count());
    metrics_notifier.on_new_metric(metrics);
  }

private:
  std::unique_ptr<dmrs_pusch_estimator>    base;
  pusch_channel_estimator_metric_notifier& metrics_notifier;
};

} // namespace srsran
