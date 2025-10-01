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
#include "srsran/cu_cp/cu_cp_types.h"
#include "srsran/e1ap/cu_cp/e1ap_cu_cp.h"
#include <string>

namespace srsran {
namespace srs_cu_cp {

class cu_up_processor_impl : public cu_up_processor
{
public:
  cu_up_processor_impl(const cu_up_processor_config_t cu_up_processor_config_,
                       e1ap_message_notifier&         e1ap_notifier_,
                       e1ap_cu_cp_notifier&           cu_cp_notifier_,
                       common_task_scheduler&         common_task_sched_);

  void stop(ue_index_t ue_index) override;

  // Message handlers.
  void             handle_cu_up_e1_setup_request(const cu_up_e1_setup_request& msg) override;
  async_task<void> handle_cu_cp_e1_reset_message(const cu_cp_reset& reset) override;

  // Getter functions.
  e1ap_cu_cp&                          get_e1ap_handler() override { return *e1ap; }
  cu_up_index_t                        get_cu_up_index() override { return context.cu_up_index; }
  cu_up_processor_context&             get_context() override { return context; }
  e1ap_message_handler&                get_e1ap_message_handler() override { return *e1ap; }
  e1ap_bearer_context_manager&         get_e1ap_bearer_context_manager() override { return *e1ap; }
  e1ap_bearer_context_removal_handler& get_e1ap_bearer_context_removal_handler() override { return *e1ap; }
  e1ap_statistics_handler&             get_e1ap_statistics_handler() override { return *e1ap; }

  void update_ue_index(ue_index_t ue_index, ue_index_t old_ue_index) override;

private:
  class e1ap_cu_up_processor_adapter;

  // E1AP senders.

  /// \brief Create and transmit the GNB-CU-UP E1 Setup response message.
  /// \param[in] du_ctxt The context of the DU that should receive the message.
  void send_cu_up_e1_setup_response();

  /// \brief Create and transmit the GNB-CU-UP E1 Setup failure message.
  /// \param[in] cause The cause of the failure.
  void send_cu_up_e1_setup_failure(e1ap_cause_t cause);

  srslog::basic_logger&    logger = srslog::fetch_basic_logger("CU-CP");
  cu_up_processor_config_t cfg;

  e1ap_message_notifier& e1ap_notifier;
  e1ap_cu_cp_notifier&   cu_cp_notifier;

  cu_up_processor_context context;

  // E1AP to CU-UP processor adapter.
  std::unique_ptr<e1ap_cu_up_processor_notifier> e1ap_ev_notifier;

  // Components.
  std::unique_ptr<e1ap_cu_cp> e1ap;
};

} // namespace srs_cu_cp
} // namespace srsran
