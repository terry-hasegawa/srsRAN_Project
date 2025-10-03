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
#include "srsran/asn1/f1ap/f1ap.h"

namespace srsran {
namespace srs_du {

struct f1ap_du_context;

class f1ap_du_ue_context_modification_procedure
{
public:
  f1ap_du_ue_context_modification_procedure(const asn1::f1ap::ue_context_mod_request_s& msg,
                                            f1ap_du_ue&                                 ue_,
                                            const f1ap_du_context&                      ctxt_);

  void operator()(coro_context<async_task<void>>& ctx);

private:
  const char* name() const { return "UE Context Modification"; }

  void create_du_request(const asn1::f1ap::ue_context_mod_request_s& msg);
  void send_ue_context_modification_response();
  void send_ue_context_modification_failure();

  async_task<bool> handle_rrc_container();
  async_task<void> handle_tx_action_indicator();

  const asn1::f1ap::ue_context_mod_request_s req;
  f1ap_du_ue&                                ue;
  const f1ap_du_context&                     du_ctxt;
  f1ap_ue_context_update_request             du_request;

  f1ap_ue_context_update_response du_response;

  srslog::basic_logger& logger;
};

} // namespace srs_du
} // namespace srsran
