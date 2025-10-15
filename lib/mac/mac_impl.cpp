/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "mac_impl.h"
#include "mac_sched/srsran_scheduler_adapter.h"

using namespace srsran;

mac_impl::mac_impl(const mac_config& params) :
  rnti_table(params.mac_cfg.initial_crnti),
  mac_sched(std::make_unique<srsran_scheduler_adapter>(
      srsran_mac_sched_config{params.mac_cfg, params.ctrl_exec, params.timers.get_timer_manager(), params.sched_cfg},
      rnti_table)),
  dl_unit(mac_dl_config{params.ue_exec_mapper,
                        params.cell_exec_mapper,
                        params.ctrl_exec,
                        params.phy_notifier,
                        params.pcap,
                        params.timers.get_timer_manager()},
          *mac_sched,
          rnti_table),
  ul_unit(mac_ul_config{params.ctrl_exec,
                        params.ue_exec_mapper,
                        params.ul_ccch_notifier,
                        *mac_sched,
                        rnti_table,
                        params.pcap,
                        params.timers.get_timer_manager()}),
  ctrl_unit(mac_control_config{params.ul_ccch_notifier,
                               params.ctrl_exec,
                               params.timers,
                               mac_control_config::metrics_config{params.metrics.period,
                                                                  params.metrics.notifier,
                                                                  params.metrics.max_nof_ue_events}},
            ul_unit,
            dl_unit,
            rnti_table,
            *mac_sched)
{
}
