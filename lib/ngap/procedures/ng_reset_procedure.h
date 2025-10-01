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

#include "ngap_transaction_manager.h"
#include "ue_context/ngap_ue_context.h"
#include "srsran/asn1/ngap/ngap_pdu_contents.h"
#include "srsran/ngap/ngap.h"
#include "srsran/support/async/async_task.h"

namespace srsran {
namespace srs_cu_cp {

class ng_reset_procedure
{
public:
  ng_reset_procedure(const cu_cp_reset&        msg_,
                     ngap_message_notifier&    amf_notif_,
                     ngap_transaction_manager& ev_mng_,
                     ngap_ue_context_list&     ue_ctxt_list_,
                     srslog::basic_logger&     logger_);

  void operator()(coro_context<async_task<void>>& ctx);

  static const char* name() { return "NG Reset Procedure"; }

  bool send_ng_reset();

private:
  const cu_cp_reset         msg;
  ngap_message_notifier&    amf_notifier;
  ngap_transaction_manager& ev_mng;
  ngap_ue_context_list&     ue_ctxt_list;
  srslog::basic_logger&     logger;

  protocol_transaction_outcome_observer<asn1::ngap::ng_reset_ack_s> transaction_sink;
};

} // namespace srs_cu_cp
} // namespace srsran
