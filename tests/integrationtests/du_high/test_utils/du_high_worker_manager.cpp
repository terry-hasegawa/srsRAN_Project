/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "du_high_worker_manager.h"
#include "srsran/support/executors/strand_executor.h"

using namespace srsran;

static constexpr unsigned nof_ue_strands   = 2;
static constexpr unsigned nof_cell_strands = 2;

du_high_worker_manager::du_high_worker_manager()
{
  using exec_config = srs_du::du_high_executor_config;

  exec_config cfg;
  cfg.cell_executors = exec_config::strand_based_worker_pool{
      nof_cell_strands, task_worker_queue_size, std::vector<task_executor*>{&high_prio_exec}};
  cfg.ue_executors       = {exec_config::ue_executor_config::map_policy::per_cell,
                            nof_ue_strands,
                            task_worker_queue_size,
                            task_worker_queue_size,
                            &low_prio_exec};
  cfg.ctrl_executors     = {task_worker_queue_size, &high_prio_exec};
  cfg.is_rt_mode_enabled = true;
  cfg.trace_exec_tasks   = false;

  exec_mapper = srs_du::create_du_high_executor_mapper(cfg);

  time_exec = std::make_unique<task_strand<priority_task_worker_pool_executor, concurrent_queue_policy::lockfree_mpmc>>(
      high_prio_exec, 128);
}

du_high_worker_manager::~du_high_worker_manager()
{
  stop();
}

void du_high_worker_manager::stop()
{
  worker_pool.stop();
}

void du_high_worker_manager::flush_pending_dl_pdus()
{
  std::vector<scoped_sync_token> tokens;
  tokens.reserve(nof_ue_strands);
  for (unsigned i = 0; i != nof_ue_strands; ++i) {
    tokens.push_back(ev.get_token());
  }
  for (unsigned i = 0; i != nof_ue_strands; ++i) {
    bool ret = exec_mapper->ue_mapper().f1u_dl_pdu_executor(to_du_ue_index(i)).defer([t = std::move(tokens[i])]() {});
    report_fatal_error_if_not(ret, "unable to dispatch task");
  }
  ev.wait();
  test_worker.run_pending_tasks();
}

void du_high_worker_manager::flush_pending_control_tasks()
{
  bool ret = exec_mapper->du_control_executor().defer([t = ev.get_token()]() {
    // do nothing, just signal
  });
  report_fatal_error_if_not(ret, "unable to dispatch task");
  ev.wait();
  test_worker.run_pending_tasks();
}
