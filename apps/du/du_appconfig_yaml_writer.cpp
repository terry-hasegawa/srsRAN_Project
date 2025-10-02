/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "du_appconfig_yaml_writer.h"
#include "apps/helpers/f1u/f1u_config_yaml_writer.h"
#include "apps/helpers/logger/logger_appconfig_yaml_writer.h"
#include "apps/services/app_execution_metrics/executor_metrics_config_yaml_writer.h"
#include "apps/services/app_resource_usage/app_resource_usage_config_yaml_writer.h"
#include "apps/services/metrics/metrics_config_yaml_writer.h"
#include "du_appconfig.h"

using namespace srsran;

static void fill_du_appconfig_hal_section(YAML::Node node, const std::optional<hal_appconfig>& config)
{
  if (!config.has_value()) {
    return;
  }
  YAML::Node hal_node  = node["hal"];
  hal_node["eal_args"] = config.value().eal_args;
}

static void fill_du_appconfig_expert_execution_section(YAML::Node node, const expert_execution_appconfig& config)
{
  {
    YAML::Node affinities_node = node["affinities"];

    if (config.affinities.main_pool_cpu_cfg.mask.any()) {
      affinities_node["main_pool_cpus"] =
          fmt::format("{:,}", span<const size_t>(config.affinities.main_pool_cpu_cfg.mask.get_cpu_ids()));
    }
    affinities_node["main_pool_pinning"] = to_string(config.affinities.main_pool_cpu_cfg.pinning_policy);
  }

  {
    YAML::Node threads_node = node["threads"];
    YAML::Node main_node    = threads_node["main_pool"];
    if (config.threads.main_pool.nof_threads.has_value()) {
      main_node["nof_threads"] = config.threads.main_pool.nof_threads.value();
    }
    main_node["task_queue_size"] = config.threads.main_pool.task_queue_size;
  }
}

static void fill_du_appconfig_buffer_pool_section(YAML::Node node, const buffer_pool_appconfig& config)
{
  node["nof_segments"] = config.nof_segments;
  node["segment_size"] = config.segment_size;
}

static void fill_du_appconfig_remote_control_section(YAML::Node node, const remote_control_appconfig& config)
{
  node["enabled"]      = config.enabled;
  node["bind_address"] = config.bind_addr;
  node["port"]         = config.port;
}

static void fill_du_appconfig_f1u_section(YAML::Node node, const srs_du::f1u_appconfig& config)
{
  node["queue_size"] = config.pdu_queue_size;
  fill_f1u_config_yaml_schema(node, config.f1u_sockets);
}

static void fill_du_appconfig_f1ap_section(YAML::Node node, const srs_du::f1ap_appconfig& config)
{
  node["cu_cp_addr"] = config.cu_cp_address;
  node["bind_addr"]  = config.bind_address;
}

void srsran::fill_du_appconfig_in_yaml_schema(YAML::Node& node, const du_appconfig& config)
{
  app_services::fill_app_resource_usage_config_in_yaml_schema(node, config.metrics_cfg.rusage_config);
  app_services::fill_metrics_appconfig_in_yaml_schema(node, config.metrics_cfg.metrics_service_cfg);
  app_services::fill_app_exec_metrics_config_in_yaml_schema(node, config.metrics_cfg.executors_metrics_cfg);
  fill_logger_appconfig_in_yaml_schema(node, config.log_cfg);
  fill_du_appconfig_hal_section(node, config.hal_config);
  fill_du_appconfig_expert_execution_section(node["expert_execution"], config.expert_execution_cfg);
  fill_du_appconfig_buffer_pool_section(node["buffer_pool"], config.buffer_pool_config);
  fill_du_appconfig_remote_control_section(node["remote_control"], config.remote_control_config);
  fill_du_appconfig_f1u_section(node["f1u"], config.f1u_cfg);
  fill_du_appconfig_f1ap_section(node["f1ap"], config.f1ap_cfg);
}
