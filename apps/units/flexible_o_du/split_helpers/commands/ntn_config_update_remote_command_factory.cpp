/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ntn_config_update_remote_command_factory.h"

using namespace srsran;

#ifndef SRSRAN_HAS_ENTERPRISE_NTN

void srsran::add_ntn_config_update_remote_command(application_unit_commands&               commands,
                                                  const ntn_config&                        ntn_cfg,
                                                  srs_du::du_configurator&                 du_cfgtr,
                                                  srs_du::du_manager_time_mapper_accessor& du_time_mapper_accessor,
                                                  ru_controller&                           ru_ctrl,
                                                  timer_manager&                           timers_,
                                                  task_executor&                           timer_exec_)
{
}

#endif // SRSRAN_HAS_ENTERPRISE_NTN
