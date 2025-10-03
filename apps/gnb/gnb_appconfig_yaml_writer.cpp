/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "gnb_appconfig_yaml_writer.h"
#include "apps/helpers/logger/logger_appconfig_yaml_writer.h"
#include "apps/helpers/tracing/tracer_appconfig_yaml_writer.h"
#include "apps/services/app_execution_metrics/executor_metrics_config_yaml_writer.h"
#include "apps/services/app_resource_usage/app_resource_usage_config_yaml_writer.h"
#include "apps/services/metrics/metrics_config_yaml_writer.h"
#include "gnb_appconfig.h"

using namespace srsran;

static void fill_gnb_appconfig_hal_section(YAML::Node node, const std::optional<hal_appconfig>& config)
{
  if (!config.has_value()) {
    return;
  }
  YAML::Node hal_node  = node["hal"];
  hal_node["eal_args"] = config.value().eal_args;
}

static void fill_gnb_appconfig_expert_execution_section(YAML::Node node, const expert_execution_appconfig& config)
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
    YAML::Node threads_node   = node["threads"];
    YAML::Node main_pool_node = threads_node["main_pool"];
    if (config.threads.main_pool.nof_threads.has_value()) {
      main_pool_node["nof_threads"] = config.threads.main_pool.nof_threads.value();
    } else {
      main_pool_node["nof_threads"] = "auto";
    }
    main_pool_node["task_queue_size"] = config.threads.main_pool.task_queue_size;
    main_pool_node["backoff_period"]  = config.threads.main_pool.backoff_period;
  }
}

static void fill_gnb_appconfig_buffer_pool_section(YAML::Node node, const buffer_pool_appconfig& config)
{
  node["nof_segments"] = config.nof_segments;
  node["segment_size"] = config.segment_size;
}

static void fill_gnb_appconfig_remote_control_section(YAML::Node node, const remote_control_appconfig& config)
{
  node["enabled"]      = config.enabled;
  node["bind_address"] = config.bind_addr;
  node["port"]         = config.port;
}

void srsran::fill_gnb_appconfig_in_yaml_schema(YAML::Node& node, const gnb_appconfig& config)
{
  node["gnb_id"]            = config.gnb_id.id;
  node["gnb_id_bit_length"] = static_cast<unsigned>(config.gnb_id.bit_length);
  node["ran_node_name"]     = config.ran_node_name;

  app_services::fill_app_resource_usage_config_in_yaml_schema(node, config.metrics_cfg.rusage_config);
  app_services::fill_metrics_appconfig_in_yaml_schema(node, config.metrics_cfg.metrics_service_cfg);
  app_services::fill_app_exec_metrics_config_in_yaml_schema(node, config.metrics_cfg.executors_metrics_cfg);
  fill_logger_appconfig_in_yaml_schema(node, config.log_cfg);
  fill_tracer_appconfig_in_yaml_schema(node, config.trace_cfg);
  fill_gnb_appconfig_hal_section(node, config.hal_config);
  fill_gnb_appconfig_expert_execution_section(node["expert_execution"], config.expert_execution_cfg);
  fill_gnb_appconfig_buffer_pool_section(node["buffer_pool"], config.buffer_pool_config);
  fill_gnb_appconfig_remote_control_section(node["remote_control"], config.remote_control_config);
}
