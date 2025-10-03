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

#include "f1ap_ue_context.h"
#include "ue_bearer_manager.h"
#include "srsran/adt/slotted_array.h"
#include "srsran/asn1/f1ap/f1ap_pdu_contents_ue.h"
#include "srsran/f1ap/du/f1ap_du.h"
#include "srsran/f1ap/f1ap_ue_id_types.h"
#include "srsran/ran/du_types.h"

namespace srsran {
namespace srs_du {

struct f1ap_du_context;

class f1ap_du_ue
{
public:
  f1ap_du_ue(du_ue_index_t          ue_index_,
             gnb_du_ue_f1ap_id_t    gnb_f1ap_du_ue_id_,
             f1ap_du_configurator&  du_handler_,
             f1ap_message_notifier& f1ap_msg_notifier_,
             task_executor&         ctrl_exec,
             task_executor&         ue_exec,
             timer_manager&         timers) :
    context(ue_index_, gnb_f1ap_du_ue_id_),
    f1ap_msg_notifier(f1ap_msg_notifier_),
    du_handler(du_handler_),
    bearers(context, f1ap_msg_notifier, du_handler, ctrl_exec, ue_exec, timers)
  {
  }

  f1ap_ue_context        context;
  f1ap_message_notifier& f1ap_msg_notifier;
  f1ap_du_configurator&  du_handler;
  ue_bearer_manager      bearers;

  /// \brief Handles UE CONTEXT MODIFICATION REQUEST as per TS38.473, Section 8.3.2.
  void handle_ue_context_modification_request(const asn1::f1ap::ue_context_mod_request_s& msg,
                                              const f1ap_du_context&                      ctxt_);
};

} // namespace srs_du
} // namespace srsran
