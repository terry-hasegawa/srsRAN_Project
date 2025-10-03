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

#include "apps/helpers/logger/logger_appconfig.h"
#include "apps/helpers/tracing/tracer_appconfig.h"
#include "apps/services/app_execution_metrics/executor_metrics_config.h"
#include "apps/services/app_resource_usage/app_resource_usage_config.h"
#include "apps/services/buffer_pool/buffer_pool_appconfig.h"
#include "apps/services/metrics/metrics_appconfig.h"
#include "apps/services/remote_control/remote_control_appconfig.h"
#include "apps/services/worker_manager/worker_manager_appconfig.h"
#include <string>

namespace srsran {
namespace srs_cu_cp {

/// E1AP configuration.
struct e1ap_appconfig {
  /// CU-CP E1AP bind address.
  std::string bind_addr = "127.0.20.1";
};

/// F1AP configuration.
struct f1ap_appconfig {
  /// F1-C bind address.
  std::string bind_addr = "127.0.10.1";
};

/// Metrics report configuration.
struct metrics_appconfig {
  app_services::app_resource_usage_config rusage_config;
  app_services::metrics_appconfig         metrics_service_cfg;
  app_services::executor_metrics_config   executors_metrics_cfg;
};

} // namespace srs_cu_cp

/// CU application configuration.
struct cu_cp_appconfig {
  /// Default constructor to update the log filename.
  cu_cp_appconfig() { log_cfg.filename = "/tmp/cu_cp.log"; }
  /// Loggers configuration.
  logger_appconfig log_cfg;
  /// Tracers configuration.
  tracer_appconfig trace_cfg;
  /// Expert configuration.
  expert_execution_appconfig expert_execution_cfg;
  /// E1AP configuration.
  srs_cu_cp::e1ap_appconfig e1ap_cfg;
  /// F1AP configuration.
  srs_cu_cp::f1ap_appconfig f1ap_cfg;
  /// Buffer pool configuration.
  buffer_pool_appconfig buffer_pool_config;
  /// Remote control configuration.
  remote_control_appconfig remote_control_config;
  /// Metrics configuration.
  srs_cu_cp::metrics_appconfig metrics_cfg;
  /// Dryrun mode enabled flag.
  bool enable_dryrun = false;
};

} // namespace srsran
