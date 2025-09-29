/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "du_high_sim_dependencies.h"
#include "tests/test_doubles/f1ap/f1ap_test_messages.h"
#include "srsran/asn1/f1ap/common.h"
#include "srsran/asn1/f1ap/f1ap.h"

using namespace srsran;
using namespace srs_du;

namespace {

/// Dummy F1AP Tx PDU notifier that emulates the CU-CP side.
class dummy_du_f1ap_tx_pdu_notifier : public f1ap_message_notifier
{
public:
  dummy_du_f1ap_tx_pdu_notifier(task_executor&                         test_exec_,
                                std::map<uint64_t, f1ap_message>&      f1ap_ul_msgs_,
                                uint64_t&                              next_msg_number_,
                                std::unique_ptr<f1ap_message_notifier> du_rx_notifier_,
                                bool                                   cell_start_on_f1_setup_,
                                unique_function<void()>                on_destruction_) :
    test_exec(test_exec_),
    f1ap_ul_msgs(f1ap_ul_msgs_),
    next_msg_number(next_msg_number_),
    du_rx_notifier(std::move(du_rx_notifier_)),
    cell_start_on_f1_setup(cell_start_on_f1_setup_),
    on_destruction(std::move(on_destruction_))
  {
  }
  ~dummy_du_f1ap_tx_pdu_notifier() override
  {
    // Notify the test client that this Tx notifier is being destroyed (e.g. due to F1 connection loss).
    on_destruction();
  }

  void on_new_message(const f1ap_message& msg) override
  {
    if (msg.pdu.type().value == asn1::f1ap::f1ap_pdu_c::types_opts::init_msg) {
      if (msg.pdu.init_msg().proc_code == ASN1_F1AP_ID_F1_SETUP) {
        // Auto-schedule CU response.
        du_rx_notifier->on_new_message(test_helpers::generate_f1_setup_response(msg, cell_start_on_f1_setup));
      } else if (msg.pdu.init_msg().proc_code == ASN1_F1AP_ID_F1_REMOVAL) {
        // Auto-schedule CU response.
        du_rx_notifier->on_new_message(test_helpers::generate_f1_removal_response(msg));
      }
    }

    // Dispatch storing of message to test main thread so it can be safely checked in the test function body.
    // Note: F1AP Tx PDU notifier can be deleted by the F1AP-DU at any moment. Therefore, we cannot pass this in the
    // capture.
    bool result = test_exec.execute([ul_msgs = &f1ap_ul_msgs, next_msg_no = &next_msg_number, msg]() {
      static srslog::basic_logger& logger = srslog::fetch_basic_logger("TEST");
      logger.info("Received F1 UL message with {}", msg.pdu.type().to_string());
      ul_msgs->emplace(std::make_pair((*next_msg_no)++, msg));
    });
    report_fatal_error_if_not(result, "Failed to execute task");
  }

  void trigger_cu_connection_loss() { du_rx_notifier.reset(); }

  task_executor&                         test_exec;
  std::map<uint64_t, f1ap_message>&      f1ap_ul_msgs;
  uint64_t&                              next_msg_number;
  std::unique_ptr<f1ap_message_notifier> du_rx_notifier;
  const bool                             cell_start_on_f1_setup;
  unique_function<void()>                on_destruction;
};

} // namespace

dummy_f1c_test_client::dummy_f1c_test_client(task_executor& test_exec_, bool cell_start_on_f1_setup_) :
  cell_start_on_f1_setup(cell_start_on_f1_setup_), test_exec(test_exec_)
{
}

std::unique_ptr<f1ap_message_notifier>
dummy_f1c_test_client::handle_du_connection_request(std::unique_ptr<f1ap_message_notifier> du_rx_pdu_notifier)
{
  if (not f1c_is_up) {
    // Not accepting new connections.
    return nullptr;
  }
  du_released_client = false;
  auto ret           = std::make_unique<dummy_du_f1ap_tx_pdu_notifier>(
      test_exec, f1ap_ul_msgs, next_msg_number, std::move(du_rx_pdu_notifier), cell_start_on_f1_setup, [this]() {
        bool success = test_exec.defer([this]() {
          du_released_client = true;
          on_connection_loss = {};
        });
        report_fatal_error_if_not(success, "Failed to defer destruction task");
      });
  on_connection_loss = [tx_notif = ret.get()]() { tx_notif->trigger_cu_connection_loss(); };
  return ret;
}

void dummy_f1c_test_client::set_f1_channel_state(bool up)
{
  if (up == f1c_is_up) {
    // Nothing happens.
    return;
  }
  if (not up) {
    // Remote node disconnected.
    f1c_is_up = false;
    auto func = std::move(on_connection_loss);
    func();
  } else {
    f1c_is_up = true;
  }
}
