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

#include "../config/sched_config_manager.h"
#include "ue.h"
#include "srsran/adt/flat_map.h"
#include "srsran/adt/ring_buffer.h"

namespace srsran {

/// Container that stores all the UEs that are configured in a given cell.
class ue_cell_repository
{
  using ue_list = slotted_id_vector<du_ue_index_t, std::shared_ptr<ue_cell>>;

public:
  using value_type     = ue_list::value_type;
  using iterator       = ue_list::iterator;
  using const_iterator = ue_list::const_iterator;

  ue_cell_repository(du_cell_index_t cell_idx, srslog::basic_logger& logger_);

  du_cell_index_t cell_index() const { return cell_idx; }
  bool            contains(du_ue_index_t ue_index) const { return ues.contains(ue_index); }
  bool     contains(rnti_t rnti) const { return rnti_to_ue_index_lookup.find(rnti) != rnti_to_ue_index_lookup.end(); }
  size_t   size() const { return ues.size(); }
  bool     empty() const { return ues.empty(); }
  iterator begin() { return ues.begin(); }
  iterator end() { return ues.end(); }
  const_iterator begin() const { return ues.begin(); }
  const_iterator end() const { return ues.end(); }

  ue_cell&       operator[](du_ue_index_t ue_index) { return *ues[ue_index]; }
  const ue_cell& operator[](du_ue_index_t ue_index) const { return *ues[ue_index]; }
  ue_cell*       find(du_ue_index_t ue_index) { return ues.contains(ue_index) ? ues[ue_index].get() : nullptr; }
  const ue_cell* find(du_ue_index_t ue_index) const { return ues.contains(ue_index) ? ues[ue_index].get() : nullptr; }
  ue_cell*       find_by_rnti(rnti_t rnti)
  {
    auto it = rnti_to_ue_index_lookup.find(rnti);
    return it != rnti_to_ue_index_lookup.end() ? ues[it->second].get() : nullptr;
  }
  const ue_cell* find_by_rnti(rnti_t rnti) const
  {
    auto it = rnti_to_ue_index_lookup.find(rnti);
    return it != rnti_to_ue_index_lookup.end() ? ues[it->second].get() : nullptr;
  }

private:
  friend class ue_repository;

  void add_ue(std::shared_ptr<ue_cell> u);
  void rem_ue(du_ue_index_t ue_index);

  const du_cell_index_t cell_idx;
  srslog::basic_logger& logger;

  // List of UEs in the cell.
  ue_list ues;

  // Mapping of RNTIs to UE indexes.
  flat_map<rnti_t, du_ue_index_t> rnti_to_ue_index_lookup;
};

/// Container that stores all scheduler UEs.
class ue_repository
{
  using ue_list = slotted_id_table<du_ue_index_t, std::unique_ptr<ue>, MAX_NOF_DU_UES>;

public:
  using value_type     = ue_list::value_type;
  using iterator       = ue_list::iterator;
  using const_iterator = ue_list::const_iterator;

  explicit ue_repository();
  ~ue_repository();

  /// \brief Mark start of new slot and update UEs states.
  void slot_indication(slot_point sl_tx);

  /// \brief Contains ue index.
  bool contains(du_ue_index_t ue_index) const { return ues.contains(ue_index); }

  ue&       operator[](du_ue_index_t ue_index) { return *ues[ue_index]; }
  const ue& operator[](du_ue_index_t ue_index) const { return *ues[ue_index]; }

  ue_cell_repository& add_cell(du_cell_index_t cell_index);
  void                rem_cell(du_cell_index_t cell_index);

  // Access UEs per cell.
  ue_cell_repository&       cell(du_cell_index_t cell_index) { return cell_ues[cell_index]; }
  const ue_cell_repository& cell(du_cell_index_t cell_index) const { return cell_ues[cell_index]; }

  /// \brief Search UE context based on TC-RNTI/C-RNTI.
  ue*       find_by_rnti(rnti_t rnti);
  const ue* find_by_rnti(rnti_t rnti) const;

  /// \brief Add new UE in the UE repository.
  void add_ue(std::unique_ptr<ue> u);

  /// \brief Reconfigure existing UE.
  void reconfigure_ue(const ue_reconf_command& cmd, bool reestablished_);

  /// \brief Initiate removal of existing UE from the repository.
  void schedule_ue_rem(ue_config_delete_event ev);

  ue*       find(du_ue_index_t ue_index) { return ues.contains(ue_index) ? ues[ue_index].get() : nullptr; }
  const ue* find(du_ue_index_t ue_index) const { return ues.contains(ue_index) ? ues[ue_index].get() : nullptr; }

  size_t size() const { return ues.size(); }

  bool empty() const { return ues.empty(); }

  iterator       begin() { return ues.begin(); }
  iterator       end() { return ues.end(); }
  const_iterator begin() const { return ues.begin(); }
  const_iterator end() const { return ues.end(); }

  const_iterator lower_bound(du_ue_index_t ue_index) const { return ues.lower_bound(ue_index); }

  void destroy_pending_ues();

  /// Handle cell deactivation by removing all UEs that are associated with the cell.
  void handle_cell_deactivation(du_cell_index_t cell_index);

private:
  /// Force the removal of the UE without waiting for the flushing of pending events.
  void rem_ue(const ue& u);

  srslog::basic_logger& logger;

  // Repository of UEs.
  ue_list ues;

  // Mapping of RNTIs to UE indexes.
  flat_map<rnti_t, du_ue_index_t> rnti_to_ue_index_lookup;

  // List of UEs per cell.
  slotted_id_table<du_cell_index_t, ue_cell_repository, MAX_NOF_DU_CELLS> cell_ues;

  // Queue of UEs marked for later removal. For each UE, we store the slot after which its removal can be safely
  // carried out, and the original UE removal command.
  ring_buffer<std::pair<slot_point, ue_config_delete_event>> ues_to_rem{MAX_NOF_DU_UES};

  // Last slot indication.
  slot_point last_sl_tx;

  // UE objects pending to be destroyed by a low priority thread.
  concurrent_queue<std::unique_ptr<ue>, concurrent_queue_policy::lockfree_mpmc> ues_to_destroy;
};

} // namespace srsran
