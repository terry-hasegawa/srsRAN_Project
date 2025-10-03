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

#include <yaml-cpp/yaml.h>

namespace srsran {

struct tracer_appconfig;

/// Fills the given node with the tracer application configuration values.
void fill_tracer_appconfig_in_yaml_schema(YAML::Node& node, const tracer_appconfig& config);

} // namespace srsran
