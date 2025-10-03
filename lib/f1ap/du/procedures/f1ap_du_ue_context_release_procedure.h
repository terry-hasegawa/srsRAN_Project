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

#include "../ue_context/f1ap_du_ue.h"
#include "../ue_context/f1ap_du_ue_manager.h"
#include "srsran/asn1/f1ap/f1ap.h"

namespace srsran {
namespace srs_du {

struct f1ap_du_context;

class f1ap_du_ue_context_release_procedure
{
public:
  f1ap_du_ue_context_release_procedure(const asn1::f1ap::ue_context_release_cmd_s& msg_,
                                       f1ap_du_ue_manager&                         ues,
                                       const f1ap_du_context&                      ctxt_);

  void operator()(coro_context<async_task<void>>& ctx);

private:
  const char* name() const { return "UE Context Release"; }

  void send_ue_context_release_complete();

  async_task<bool> handle_rrc_container();

  const asn1::f1ap::ue_context_release_cmd_s msg;
  f1ap_du_ue&                                ue;
  srslog::basic_logger&                      logger = srslog::fetch_basic_logger("DU-F1");
  f1ap_message_notifier&                     cu_msg_notifier; // used after the UE context as been released.
  const f1ap_du_context&                     du_ctxt;
};

} // namespace srs_du
} // namespace srsran
