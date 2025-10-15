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

#include "srsran/mac/mac_cell_manager.h"
#include "srsran/mac/mac_cell_slot_handler.h"
#include "srsran/mac/mac_ue_control_information_handler.h"
#include "srsran/ran/du_types.h"

namespace srsran {

struct si_scheduling_update_request;
struct sched_result;

/// \brief Interface used by MAC Cell Processor to interact with the MAC scheduler.
class mac_scheduler_cell_info_handler : public mac_ue_control_information_handler
{
public:
  virtual ~mac_scheduler_cell_info_handler() = default;

  /// \brief Start scheduling for a given cell. If cell was already activated, this operation has no effect.
  /// \param cell_idx DU-specific index of the cell for which the slot is being processed.
  /// \remark This function must be called before the first slot indication is processed.
  virtual void handle_cell_activation(du_cell_index_t cell_idx) = 0;

  /// \brief Stop running cell. If cell was already deactivated, this operation has no effect.
  /// \param cell_idx DU-specific index of the cell for which the slot is being processed.
  /// \remark This function must be called after the last slot indication is processed.
  virtual void handle_cell_deactivation(du_cell_index_t cell_idx) = 0;

  /// \brief Processes a new slot for a specific cell in the MAC scheduler.
  /// \param slot_tx SFN + slot index of the Transmit slot to be processed.
  /// \param cell_idx DU-specific index of the cell for which the slot is being processed.
  /// \return Result of the scheduling operation. It contains both DL and UL scheduling information.
  virtual const sched_result& slot_indication(slot_point slot_tx, du_cell_index_t cell_idx) = 0;

  /// \brief Processes an error indication for a specific cell in the MAC scheduler.
  /// \param slot_tx SFN + slot index of the Transmit slot to be processed.
  /// \param cell_idx DU-specific index of the cell for which the indication is being processed.
  /// \param event Effect that the errors in the lower layers had on the result provided by the scheduler.
  virtual void
  handle_error_indication(slot_point slot_tx, du_cell_index_t cell_idx, mac_cell_slot_handler::error_event event) = 0;

  /// \brief Update SIB1 and SI scheduling information in scheduler.
  /// \param[in] request Request to change SI sched info and messages.
  virtual void handle_si_change_indication(const si_scheduling_update_request& request) = 0;

  /// \brief Handle request to update the slice configuration of a cell.
  /// \param[in] cell_index Index of the cell for which the measurement is directed.
  /// \param[in] req Request to update the RRM policies.
  virtual void handle_slice_reconfiguration_request(const du_cell_slice_reconfig_request& req) = 0;
};

} // namespace srsran
