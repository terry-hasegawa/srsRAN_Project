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

#include "rrc_ue_srb_context.h"
#include "srsran/asn1/rrc_nr/ul_ccch_msg_ies.h"
#include "srsran/asn1/rrc_nr/ul_dcch_msg_ies.h"
#include "srsran/ran/plmn_identity.h"
#include "srsran/rrc/rrc_cell_context.h"
#include "srsran/rrc/rrc_ue.h"
#include "srsran/rrc/rrc_ue_config.h"

namespace srsran {
namespace srs_cu_cp {

/// Holds the RRC UE context used by the UE object and all its procedures.
class rrc_ue_context_t
{
public:
  rrc_ue_context_t(const ue_index_t                       ue_index_,
                   const rnti_t                           c_rnti_,
                   const rrc_cell_context&                cell_,
                   const rrc_ue_cfg_t&                    cfg_,
                   std::optional<rrc_ue_transfer_context> rrc_context_);

  const ue_index_t                   ue_index; // UE index assigned by the DU processor
  const rnti_t                       c_rnti;   // current C-RNTI
  rrc_cell_context                   cell;     // current cell
  const rrc_ue_cfg_t                 cfg;
  plmn_identity                      plmn_id = plmn_identity::test_value(); // PLMN identity of the UE
  rrc_state                          state   = rrc_state::idle;
  std::optional<rrc_meas_cfg>        meas_cfg;
  std::optional<cu_cp_five_g_s_tmsi> five_g_s_tmsi;
  std::variant<uint64_t, asn1::fixed_bitstring<39>>
                                     setup_ue_id; ///< this is either a random value or the 5G-S-TMSI-PART1
  establishment_cause_t              connection_cause;
  std::map<srb_id_t, ue_srb_context> srbs;
  std::optional<asn1::rrc_nr::ue_cap_rat_container_list_l> capabilities_list;
  std::optional<rrc_ue_transfer_context> transfer_context; // Context of old UE when created through mobility.
  bool                                   reestablishment_ongoing = false;
  srslog::basic_logger&                  logger;
};

} // namespace srs_cu_cp
} // namespace srsran
