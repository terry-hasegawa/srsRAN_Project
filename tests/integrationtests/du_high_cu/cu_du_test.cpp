/*
 *
 * Copyright 2021-2024 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "lib/du_high/du_high_executor_strategies.h"
#include "tests/integrationtests/du_high/test_utils/du_high_worker_manager.h"
#include "tests/test_doubles/f1ap/f1c_test_local_gateway.h"
#include "tests/unittests/cu_cp/test_doubles/mock_amf.h"
#include "tests/unittests/ngap/ngap_test_messages.h"
#include "tests/unittests/ngap/test_helpers.h"
#include "srsran/cu_cp/cu_cp.h"
#include "srsran/cu_cp/cu_cp_factory.h"
#include "srsran/du/du_cell_config_helpers.h"
#include "srsran/du_high/du_high_factory.h"
#include <chrono>
#include <gtest/gtest.h>

using namespace srsran;

// Dummy classes required by DU
struct phy_cell_dummy : public mac_cell_result_notifier {
  void on_new_downlink_scheduler_results(const mac_dl_sched_result& dl_res) override {}
  void on_new_downlink_data(const mac_dl_data_result& dl_data) override {}
  void on_new_uplink_scheduler_results(const mac_ul_sched_result& ul_res) override {}
  void on_cell_results_completion(slot_point slot) override {}
};

struct phy_dummy : public mac_result_notifier {
public:
  mac_cell_result_notifier& get_cell(du_cell_index_t cell_index) override { return cell; }
  phy_cell_dummy            cell;
};

/// Fixture class for successful F1Setup
class cu_du_test : public ::testing::Test
{
protected:
  void SetUp() override
  {
    srslog::fetch_basic_logger("TEST").set_level(srslog::basic_levels::debug);
    srslog::init();

    // create CU-CP config
    srs_cu_cp::cu_cp_configuration cu_cfg;
    cu_cfg.ngap_config.tac                 = 7;
    cu_cfg.cu_cp_executor                  = &workers.ctrl_exec;
    cu_cfg.ngap_notifier                   = &*amf;
    cu_cfg.timers                          = &timers;
    cu_cfg.statistics_report_period        = std::chrono::seconds(1);
    cu_cfg.ue_config.max_nof_supported_ues = cu_cfg.max_nof_dus * srsran::srs_cu_cp::MAX_NOF_UES_PER_DU;

    // create CU-CP.
    cu_cp_obj = create_cu_cp(cu_cfg);

    // Create AMF response to NG Setup.
    amf->attach_cu_cp_pdu_handler(cu_cp_obj->get_ng_interface().get_ngap_message_handler());
    amf->enqueue_next_tx_pdu(srs_cu_cp::generate_ng_setup_response());

    // Start CU-CP.
    cu_cp_obj->start();

    // Attach F1-C gateway to CU-CP.
    f1c_gw.attach_cu_cp_du_repo(cu_cp_obj->get_dus());

    // create and start DU
    phy_dummy phy;

    srsran::srs_du::du_high_configuration du_cfg{};
    du_cfg.exec_mapper = &workers.exec_mapper;
    du_cfg.f1c_client  = &f1c_gw;
    du_cfg.phy_adapter = &phy;
    du_cfg.timers      = &timers;
    du_cfg.cells       = {config_helpers::make_default_du_cell_config()};
    du_cfg.sched_cfg   = config_helpers::make_default_scheduler_expert_config();
    du_cfg.gnb_du_name = "test_du";

    // create DU object
    du_obj = make_du_high(std::move(du_cfg));

    // start CU and DU
    du_obj->start();
  }

public:
  du_high_worker_manager workers;
  timer_manager          timers;
  srslog::basic_logger&  test_logger = srslog::fetch_basic_logger("TEST");
  f1c_test_local_gateway f1c_gw{};

  std::unique_ptr<srs_cu_cp::mock_amf>        amf{srs_cu_cp::create_mock_amf()};
  std::unique_ptr<srs_cu_cp::cu_cp_interface> cu_cp_obj;
  std::unique_ptr<du_high>                    du_obj;
};

/// Test the f1 setup procedure was successful
TEST_F(cu_du_test, when_f1setup_successful_then_du_connected)
{
  // check that DU has been added
  auto report = cu_cp_obj->get_metrics_handler().request_metrics_report();
  ASSERT_EQ(report.dus.size(), 1);
  ASSERT_EQ(report.dus[0].cells.size(), 1);
}
