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

#include "../cell/resource_grid.h"
#include "../config/cell_configuration.h"
#include "../ue_scheduling/ue_repository.h"
#include "ran_slice_id.h"
#include "slice_ue_repository.h"
#include "srsran/scheduler/config/slice_rrm_policy_config.h"

namespace srsran {

/// This class stores all the internal information relative to a RAN slice instantiation.
class ran_slice_instance
{
  // Const for the max difference in number of slots between the current slot and the last allocation slot.
  // This limit ensures lack of ambiguity in the slot point comparison
  static constexpr unsigned MAX_SLOTS_SINCE_LAST_PXSCH = 256;

public:
  ran_slice_instance(ran_slice_id_t id_, const cell_configuration& cell_cfg_, const slice_rrm_policy_config& cfg_);

  void slot_indication(slot_point slot_tx);

  bool active() const { return not slice_ues.empty(); }

  /// Save PDSCH grant.
  /// \param[in] crbs Number of RBs allocated.
  /// \param[in] pdsch_slot Slot where the PDSCH is scheduled.
  /// \param[in,out] slot_rem_ded_rbs Remaining dedicated RBs of all slices for this slot.
  void store_pdsch_grant(unsigned crbs, slot_point pdsch_slot, unsigned& slot_rem_ded_rbs)
  {
    if (pdsch_rb_count < cfg.rbs.dedicated()) {
      // Deduct from remaining dedicated RBs of this slice to the total number of dedicated RBs left.
      slot_rem_ded_rbs -= std::min({slot_rem_ded_rbs, crbs, cfg.rbs.dedicated() - pdsch_rb_count});
    }
    pdsch_rb_count += crbs;
    last_pdsch_alloc_slot = pdsch_slot;
  }

  /// Save PUSCH grant.
  void store_pusch_grant(unsigned crbs, slot_point pusch_slot, unsigned& slot_rem_ded_rbs)
  {
    auto& pusch_count = pusch_rb_count_per_slot[pusch_slot.count() % pusch_rb_count_per_slot.size()];
    if (pusch_count < cfg.rbs.dedicated()) {
      // Deduct from remaining dedicated RBs of this slice to the total number of dedicated RBs left.
      slot_rem_ded_rbs -= std::min({slot_rem_ded_rbs, crbs, cfg.rbs.dedicated() - pusch_count});
    }
    pusch_count += crbs;
    last_pusch_alloc_slot = pusch_slot;
  }

  /// Returns UEs belonging to this slice.
  const slice_ue_repository& get_ues() const { return slice_ues; }
  slice_ue_repository&       get_ues() { return slice_ues; }

  unsigned nof_pusch_rbs_allocated(slot_point pusch_slot) const
  {
    return pusch_rb_count_per_slot[pusch_slot.count() % pusch_rb_count_per_slot.size()];
  }

  /// Number of slots elapsed between the provided PDSCH slot and the last time this slice was scheduled.
  unsigned nof_slots_since_last_pdsch(slot_point pdsch_slot) const
  {
    return last_pdsch_alloc_slot.valid()
               ? (pdsch_slot >= last_pdsch_alloc_slot ? pdsch_slot - last_pdsch_alloc_slot : 0)
               : MAX_SLOTS_SINCE_LAST_PXSCH;
  }
  unsigned nof_slots_since_last_pusch(slot_point pusch_slot) const
  {
    return last_pusch_alloc_slot.valid()
               ? (pusch_slot >= last_pusch_alloc_slot ? pusch_slot - last_pusch_alloc_slot : 0)
               : MAX_SLOTS_SINCE_LAST_PXSCH;
  }

  float average_pdsch_rbs_per_slot() const { return avg_pdsch_rbs_per_slot; }
  float average_pusch_rbs_per_slot() const { return avg_pusch_rbs_per_slot; }

  ran_slice_id_t            id;
  const cell_configuration* cell_cfg;
  slice_rrm_policy_config   cfg;
  const unsigned            min_k2;

  /// Counter of how many RBs have been scheduled for PDSCH in the current slot for this slice.
  unsigned pdsch_rb_count = 0;
  /// Ring of counters of how many RBs have been scheduled for PUSCH in a particular slot for this slice.
  std::vector<unsigned> pusch_rb_count_per_slot;

  /// Nof. of previous slots in which RB count needs to be cleared in \c pusch_rb_count_per_slot upon receiving slot
  /// indication.
  /// \remark This is needed since slot_indication() is called only over DL slots.
  unsigned nof_slots_to_clear =
      cell_cfg->tdd_cfg_common.has_value() ? nof_slots_per_tdd_period(*cell_cfg->tdd_cfg_common) : 1;

private:
  // Last slot that the PDSCH was allocated.
  slot_point last_pdsch_alloc_slot;

  // Last slot that the PUSCH was allocated.
  slot_point last_pusch_alloc_slot;

  // Track average number of RBs scheduler per slice.
  float avg_pdsch_rbs_per_slot = 0;
  float avg_pusch_rbs_per_slot = 0;

  slice_ue_repository slice_ues;
};

} // namespace srsran
