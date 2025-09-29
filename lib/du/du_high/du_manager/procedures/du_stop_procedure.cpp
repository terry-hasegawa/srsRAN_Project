/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "du_stop_procedure.h"
#include "await_all_ues.h"
#include "srsran/support/async/async_timer.h"
#include "srsran/support/async/execute_on.h"

using namespace srsran;
using namespace srs_du;

du_stop_procedure::du_stop_procedure(du_ue_manager&           ue_mng_,
                                     du_cell_manager&         cell_mng_,
                                     f1ap_connection_manager& f1ap_conn_mng_) :
  ue_mng(ue_mng_),
  cell_mng(cell_mng_),
  f1ap_conn_mng(f1ap_conn_mng_),
  proc_logger(srslog::fetch_basic_logger("DU-MNG"), "DU Stop")
{
}

void du_stop_procedure::operator()(coro_context<async_task<void>>& ctx)
{
  CORO_BEGIN(ctx);

  proc_logger.log_proc_started();

  // Stop all cells.
  // Note: This will stop all scheduling activity and unlock any pending scheduler UE update procedure.
  CORO_AWAIT(cell_mng.stop_all());

  proc_logger.log_progress("Stopped all cells");

  // Stop all UE traffic
  CORO_AWAIT(stop_ue_traffic());

  // Run F1 Remove Procedure.
  CORO_AWAIT(f1ap_conn_mng.handle_f1_removal_request());

  // Forcefully interrupt all UE activity.
  CORO_AWAIT(ue_mng.stop());

  proc_logger.log_proc_completed();

  CORO_RETURN();
}

async_task<void> du_stop_procedure::stop_ue_traffic()
{
  return await_all_ues(ue_mng, [](du_ue& u) { return u.handle_activity_stop_request(true); });
}
