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

#include "../ue_context/ue_repository.h"
#include "srs_scheduler.h"
#include "srsran/ran/srs/srs_configuration.h"

namespace srsran {

struct cell_resource_allocator;
struct cell_slot_resource_allocator;
struct srs_info;

class srs_scheduler_impl : public srs_scheduler
{
public:
  explicit srs_scheduler_impl(const cell_configuration& cell_cfg_, ue_repository& ues_);

  ~srs_scheduler_impl() override;

  void run_slot(cell_resource_allocator& res_alloc) override;

  /// Called on cell deactivation.
  void stop();

  void add_ue(const ue_cell_configuration& ue_cfg) override;

  void rem_ue(const ue_cell_configuration& ue_cfg) override;

  void reconf_ue(const ue_cell_configuration& new_ue_cfg, const ue_cell_configuration& old_ue_cfg) override;

  void handle_positioning_measurement_request(const positioning_measurement_request::cell_info& req) override;

  void handle_positioning_measurement_stop(rnti_t pos_rnti) override;

private:
  /// Information on currently configured SRS resources and corresponding UEs to be scheduled periodically.
  struct periodic_srs_info {
    rnti_t                 rnti       = rnti_t::INVALID_RNTI;
    srs_config::srs_res_id srs_res_id = srs_config::srs_res_id::MAX_NOF_SRS_RES;
  };

  // Info related with a UE context update.
  struct ue_update {
    enum class type_t { new_ue, positioning_request, positioning_stop };

    rnti_t rnti;
    type_t type;
  };

  // Helper to fetch a UE cell config.
  const ue_cell_configuration* get_ue_cfg(rnti_t rnti) const;
  // Helper that schedules the SRS for a given slot.
  void schedule_slot_srs(cell_slot_resource_allocator& slot_alloc);
  // Helper that schedules the SRS for UEs that were recently added or reconfigured.
  void schedule_updated_ues_srs(cell_resource_allocator& res_alloc);
  // Helper that allocates an SRS opportunity for a given UE.
  bool allocate_srs_opportunity(cell_slot_resource_allocator& slot_alloc, const periodic_srs_info& srs_opportunity);

  void add_ue_to_grid(const ue_cell_configuration& ue_cfg, bool is_reconf);

  void add_resource(rnti_t crnti, srs_periodicity period, unsigned offset, srs_config::srs_res_id res_id);
  void rem_resource(rnti_t crnti, srs_periodicity period, unsigned offset, srs_config::srs_res_id res_id);

  // Cell configuration.
  const cell_configuration& cell_cfg;
  ue_repository&            ues;

  srslog::basic_logger& logger;

  // Storage of the periodic SRSs to be scheduled in the resource grid. Each position of the vector represents a slot
  // in a ring-like structure (ie slot % WHEEL_SIZE). Each of these vector indexes/slots contains a list of periodic
  // SRS information to be scheduled in the respective slot.
  std::vector<static_vector<periodic_srs_info, MAX_SRS_PDUS_PER_SLOT>> periodic_srs_slot_wheel;

  // UEs whose configuration has been updated in between the last and current slot indications.
  std::vector<ue_update> updated_ues;

  // Pending positioning requests for this cell.
  std::vector<positioning_measurement_request::cell_info> pending_pos_requests;
};

} // namespace srsran
