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

#include "external/nlohmann/json.hpp"
#include <string>

namespace srsran {

struct executor_metrics;

namespace app_helpers {
namespace json_generators {

/// Generates a nlohmann JSON object that codifies the given executor metrics.
nlohmann::json generate(const executor_metrics& metrics);

/// Generates a string in JSON format that codifies the given executor metrics.
std::string generate_string(const executor_metrics& metrics, int indent = -1);

} // namespace json_generators
} // namespace app_helpers
} // namespace srsran
