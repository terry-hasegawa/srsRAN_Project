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

#include "srsran/adt/static_vector.h"
#include "srsran/ran/du_types.h"
#include "srsran/ran/srs/srs_configuration.h"
#include <optional>

namespace srsran {

/// \brief Request for positioning measurements to the scheduler.
struct positioning_measurement_request {
  struct cell_info {
    /// \brief This RNTI can correspond to either a real connected UE C-RNTI or an RNTI assigned just for positioning
    /// measurement.
    rnti_t pos_rnti;
    /// In case the positioning measurement is for a currently connected UE, we also define the UE index.
    std::optional<du_ue_index_t> ue_index;
    /// Cell at which the positioning is to be made.
    du_cell_index_t cell_index;
    /// SRS resources to measure.
    srs_config srs_to_measure;
  };

  // List of cells for which positioning measurements are to be made.
  static_vector<cell_info, MAX_NOF_DU_CELLS> cells;
};

struct positioning_measurement_stop_request {
  /// Cells for which to stop the positioning measurement.
  static_vector<std::pair<du_cell_index_t, rnti_t>, MAX_NOF_DU_CELLS> completed_meas;
};

/// Interface used to start and stop new positioning measurements in the scheduler.
class scheduler_positioning_handler
{
public:
  virtual ~scheduler_positioning_handler() = default;

  /// Initiates a new positioning measurement.
  virtual void handle_positioning_measurement_request(const positioning_measurement_request& req) = 0;

  /// Shuts down an on-going positioning measurement.
  virtual void handle_positioning_measurement_stop(const positioning_measurement_stop_request& req) = 0;
};

/// \brief Interface for handling positioning measurements for a specific cell.
class scheduler_cell_positioning_handler
{
public:
  virtual ~scheduler_cell_positioning_handler() = default;

  /// \brief  Handles a new positioning measurement request for a specific cell.
  /// \param req Request containing the SRS resources to measure.
  virtual void handle_positioning_measurement_request(const positioning_measurement_request::cell_info& req) = 0;

  /// \brief Stops an on-going positioning measurement for a specific cell.
  /// \param pos_rnti RNTI used for the positioning measurement.
  virtual void handle_positioning_measurement_stop(rnti_t pos_rnti) = 0;
};

} // namespace srsran
