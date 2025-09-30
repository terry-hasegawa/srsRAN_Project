/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "srsran/e1ap/cu_cp/e1ap_cu_cp_factory.h"
#include "e1ap_cu_cp_impl.h"
#include "srsran/cu_cp/cu_cp_types.h"

/// Notice this would be the only place were we include concrete class implementation files.

using namespace srsran;
using namespace srs_cu_cp;

std::unique_ptr<e1ap_cu_cp>
srsran::srs_cu_cp::create_e1ap(const e1ap_configuration&      e1ap_cfg_,
                               cu_up_index_t                  cu_up_index_,
                               e1ap_message_notifier&         e1ap_pdu_notifier_,
                               e1ap_cu_up_processor_notifier& e1ap_cu_up_processor_notifier_,
                               e1ap_cu_cp_notifier&           cu_cp_notifier_,
                               timer_manager&                 timers_,
                               task_executor&                 ctrl_exec_,
                               unsigned                       max_nof_supported_ues_)
{
  auto e1ap_cu_cp = std::make_unique<e1ap_cu_cp_impl>(e1ap_cfg_,
                                                      cu_up_index_,
                                                      e1ap_pdu_notifier_,
                                                      e1ap_cu_up_processor_notifier_,
                                                      cu_cp_notifier_,
                                                      timers_,
                                                      ctrl_exec_,
                                                      max_nof_supported_ues_);
  return e1ap_cu_cp;
}
