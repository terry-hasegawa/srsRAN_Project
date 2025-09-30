/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "cu_up_processor_factory.h"
#include "cu_up_processor_impl.h"

/// Notice this would be the only place were we include concrete class implementation files.

using namespace srsran;
using namespace srs_cu_cp;

std::unique_ptr<cu_up_processor>
srsran::srs_cu_cp::create_cu_up_processor(const cu_up_processor_config_t cu_up_processor_config_,
                                          e1ap_message_notifier&         e1ap_notifier_,
                                          e1ap_cu_cp_notifier&           cu_cp_notifier_,
                                          common_task_scheduler&         common_task_sched_)
{
  auto cu_up_processor = std::make_unique<cu_up_processor_impl>(
      cu_up_processor_config_, e1ap_notifier_, cu_cp_notifier_, common_task_sched_);
  return cu_up_processor;
}
