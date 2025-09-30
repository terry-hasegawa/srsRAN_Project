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

#include "cu_up_processor.h"
#include "cu_up_processor_config.h"
#include "srsran/cu_cp/common_task_scheduler.h"
#include "srsran/e1ap/common/e1ap_common.h"
#include <memory>

namespace srsran {
namespace srs_cu_cp {

/// Creates an instance of an CU-UP processor interface
std::unique_ptr<cu_up_processor> create_cu_up_processor(const cu_up_processor_config_t cu_up_processor_config_,
                                                        e1ap_message_notifier&         e1ap_notifier_,
                                                        e1ap_cu_cp_notifier&           cu_cp_notifier_,
                                                        common_task_scheduler&         common_task_sched_);

} // namespace srs_cu_cp
} // namespace srsran
