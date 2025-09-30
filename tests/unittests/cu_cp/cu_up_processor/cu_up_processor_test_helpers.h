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

#include "../test_helpers.h"
#include "cu_up_processor_test_helpers.h"
#include "lib/cu_cp/cu_up_processor/cu_up_processor.h"
#include "lib/cu_cp/cu_up_processor/cu_up_processor_factory.h"
#include "lib/cu_cp/ue_manager/ue_manager_impl.h"
#include "tests/unittests/e1ap/cu_cp/e1ap_cu_cp_test_helpers.h"
#include "srsran/cu_cp/cu_cp_types.h"
#include "srsran/support/executors/manual_task_worker.h"
#include "srsran/support/test_utils.h"
#include <gtest/gtest.h>

namespace srsran {
namespace srs_cu_cp {

/// Fixture class for DU processor creation
class cu_up_processor_test : public ::testing::Test
{
protected:
  cu_up_processor_test();
  ~cu_up_processor_test() override;

  srslog::basic_logger& test_logger  = srslog::fetch_basic_logger("TEST");
  srslog::basic_logger& cu_cp_logger = srslog::fetch_basic_logger("CU-CP");

  timer_manager               timers;
  dummy_e1ap_message_notifier e1ap_notifier;
  manual_task_worker          ctrl_worker{128};
  cu_cp_configuration         cu_cp_cfg;

  ue_manager                             ue_mng{cu_cp_cfg};
  dummy_e1ap_cu_cp_notifier              cu_cp_notifier{ue_mng};
  std::unique_ptr<common_task_scheduler> common_task_sched;
  std::unique_ptr<cu_up_processor>       cu_up_processor_obj;
};

} // namespace srs_cu_cp
} // namespace srsran
