/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "cu_appconfig_yaml_writer.h"
#include "apps/helpers/f1u/f1u_config_yaml_writer.h"
#include "apps/helpers/logger/logger_appconfig_yaml_writer.h"
#include "apps/helpers/tracing/tracer_appconfig_yaml_writer.h"
#include "apps/services/app_execution_metrics/executor_metrics_config_yaml_writer.h"
#include "apps/services/app_resource_usage/app_resource_usage_config_yaml_writer.h"
#include "apps/services/metrics/metrics_config_yaml_writer.h"
#include "cu_appconfig.h"

using namespace srsran;

static void fill_cu_appconfig_buffer_pool_section(YAML::Node node, const buffer_pool_appconfig& config)
{
  node["nof_segments"] = config.nof_segments;
  node["segment_size"] = config.segment_size;
}

static void fill_cu_appconfig_remote_control_section(YAML::Node node, const remote_control_appconfig& config)
{
  node["enabled"]      = config.enabled;
  node["bind_address"] = config.bind_addr;
  node["port"]         = config.port;
}

static void fill_cu_appconfig_f1ap_section(YAML::Node node, const srs_cu::cu_f1ap_appconfig& config)
{
  YAML::Node cu_cp_node     = node["cu_cp"];
  YAML::Node f1ap_node      = cu_cp_node["f1ap"];
  f1ap_node["bind_address"] = config.bind_addr;
}

static void fill_cu_appconfig_f1u_section(YAML::Node& node, const f1u_sockets_appconfig& config)
{
  YAML::Node cu_up_node = node["cu_up"];
  fill_f1u_config_yaml_schema(cu_up_node, config);
}

void srsran::fill_cu_appconfig_in_yaml_schema(YAML::Node& node, const cu_appconfig& config)
{
  app_services::fill_app_resource_usage_config_in_yaml_schema(node, config.metrics_cfg.rusage_config);
  app_services::fill_metrics_appconfig_in_yaml_schema(node, config.metrics_cfg.metrics_service_cfg);
  app_services::fill_app_exec_metrics_config_in_yaml_schema(node, config.metrics_cfg.executors_metrics_cfg);
  fill_logger_appconfig_in_yaml_schema(node, config.log_cfg);
  fill_tracer_appconfig_in_yaml_schema(node, config.trace_cfg);
  fill_cu_appconfig_buffer_pool_section(node["buffer_pool"], config.buffer_pool_config);
  fill_cu_appconfig_remote_control_section(node["remote_control"], config.remote_control_config);
  fill_cu_appconfig_f1ap_section(node, config.f1ap_cfg);
  fill_cu_appconfig_f1u_section(node, config.f1u_cfg);
}
