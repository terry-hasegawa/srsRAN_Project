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

#include "srsran/mac/mac_cell_control_information_handler.h"
#include "srsran/mac/mac_cell_rach_handler.h"
#include "srsran/mac/mac_cell_slot_handler.h"
#include "srsran/mac/mac_manager.h"
#include "srsran/mac/mac_paging_information_handler.h"
#include "srsran/mac/mac_pdu_handler.h"
#include "srsran/mac/mac_ue_control_information_handler.h"

namespace srsran {

class mac_interface : public mac_manager
{
public:
  virtual ~mac_interface() = default;

  /// Returns handler of PRACHs.
  virtual mac_cell_rach_handler& get_rach_handler(du_cell_index_t cell_index) = 0;

  /// Returns handler for UL and DL control information for a given cell.
  virtual mac_cell_control_information_handler& get_control_info_handler(du_cell_index_t cell_index) = 0;

  /// Returns handler for UE control information.
  virtual mac_ue_control_information_handler& get_ue_control_info_handler() = 0;

  /// \brief Returns handler of slot indications for a cell with provided cell_index.
  /// \param cell_index Index of cell currently activated in the DU.
  virtual mac_cell_slot_handler& get_slot_handler(du_cell_index_t cell_index) = 0;

  /// Returns PDU handler.
  virtual mac_pdu_handler& get_pdu_handler() = 0;

  /// Returns Paging handler.
  virtual mac_paging_information_handler& get_cell_paging_info_handler() = 0;
};

} // namespace srsran
