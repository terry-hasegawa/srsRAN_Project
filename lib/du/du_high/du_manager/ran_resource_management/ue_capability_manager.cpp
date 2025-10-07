/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ue_capability_manager.h"
#include "srsran/asn1/rrc_nr/ul_dcch_msg_ies.h"
#include "srsran/ran/band_helper.h"

using namespace srsran;
using namespace srs_du;

expected<ue_capability_summary, std::string>
srsran::srs_du::decode_ue_nr_cap_container(const byte_buffer& ue_cap_container)
{
  asn1::rrc_nr::ue_nr_cap_s ue_cap;
  {
    asn1::cbit_ref bref{ue_cap_container};
    if (ue_cap.unpack(bref) != asn1::SRSASN_SUCCESS) {
      return make_unexpected(std::string("Couldn't unpack UE NR Capability RRC container"));
    }
  }

  ue_capability_summary ue_caps;

  // fill UE capability summary.
  if (ue_cap.phy_params.phy_params_fr1_present) {
    ue_caps.pdsch_qam256_supported = ue_cap.phy_params.phy_params_fr1.pdsch_256_qam_fr1_present;
  }
  if (ue_cap.phy_params.phy_params_frx_diff_present) {
    ue_caps.pdsch_qam64lowse_supported = ue_cap.phy_params.phy_params_frx_diff.dl_64_qam_mcs_table_alt_present;
    ue_caps.pusch_qam64lowse_supported = ue_cap.phy_params.phy_params_frx_diff.ul_64_qam_mcs_table_alt_present;
  }
  if (ue_cap.phy_params.phy_params_common_present) {
    ue_caps.pdsch_interleaving_vrb_to_prb_supported =
        ue_cap.phy_params.phy_params_common.interleaving_vrb_to_prb_pdsch_present;
  }
  for (const auto& band : ue_cap.rf_params.supported_band_list_nr) {
    // Create and convert band capability.
    ue_capability_summary::supported_band band_cap;
    band_cap.pusch_qam256_supported = band.pusch_256_qam_present;

    // Emplace the band capability in the map.
    ue_caps.bands.emplace(static_cast<nr_band>(band.band_nr), band_cap);
  }
  if (ue_cap.mac_params_present and ue_cap.mac_params.mac_params_xdd_diff_present) {
    ue_caps.long_drx_cycle_supported  = ue_cap.mac_params.mac_params_xdd_diff.long_drx_cycle_present;
    ue_caps.short_drx_cycle_supported = ue_cap.mac_params.mac_params_xdd_diff.short_drx_cycle_present;
  }

  // Convert advanced UE NR capabilities.
  decode_advanced_ue_nr_caps(ue_caps, ue_cap);

  // Convert advanced UE NR NTN capabilities.
  decode_advanced_ue_nr_ntn_caps(ue_caps, ue_cap);

  return ue_caps;
}

#ifndef SRSRAN_HAS_ENTERPRISE

void srsran::srs_du::decode_advanced_ue_nr_caps(ue_capability_summary&           ue_capability,
                                                const asn1::rrc_nr::ue_nr_cap_s& ue_caps)
{
  // Advanced UE capabilities is not implemented.
}

#endif // SRSRAN_HAS_ENTERPRISE

#ifndef SRSRAN_HAS_ENTERPRISE_NTN

void srsran::srs_du::decode_advanced_ue_nr_ntn_caps(ue_capability_summary&           ue_capability,
                                                    const asn1::rrc_nr::ue_nr_cap_s& ue_cap)
{
  // Advanced NTN UE capabilities is not implemented.
}

#endif // SRSRAN_HAS_ENTERPRISE_NTN

// Configure dedicated UE configuration to set MCS ant CQI tables.
static void set_pdsch_mcs_table(serving_cell_config& cell_cfg, pdsch_mcs_table mcs_table)
{
  cqi_table_t cqi_table;
  // Set CQI table according to the MCS table used for PDSCH.
  switch (mcs_table) {
    case pdsch_mcs_table::qam64:
      cqi_table = cqi_table_t::table1;
      break;
    case pdsch_mcs_table::qam256:
      cqi_table = cqi_table_t::table2;
      break;
    case pdsch_mcs_table::qam64LowSe:
      cqi_table = cqi_table_t::table3;
      break;
    default:
      report_error("Invalid MCS table={}\n", fmt::underlying(mcs_table));
  }

  // Set MCS index table for PDSCH. See TS 38.214, Table 5.1.3.1-[1-3].
  if (cell_cfg.init_dl_bwp.pdsch_cfg.has_value()) {
    cell_cfg.init_dl_bwp.pdsch_cfg->mcs_table = mcs_table;
  }

  if (cell_cfg.csi_meas_cfg.has_value()) {
    for (auto& csi_report_cfg : cell_cfg.csi_meas_cfg.value().csi_report_cfg_list) {
      // Set CQI table. See TS 38.214, Table 5.2.2.1-[1-4].
      csi_report_cfg.cqi_table = cqi_table;
    }
  }
}

// Configure dedicated UE configuration to set PUSCH MCS.
static void set_pusch_mcs_table(serving_cell_config& cell_cfg, pusch_mcs_table mcs_table)
{
  // Set MCS index table for PUSCH. See TS 38.214, Table 5.1.3.1-[1-3].
  if (cell_cfg.ul_config.has_value() and cell_cfg.ul_config->init_ul_bwp.pusch_cfg.has_value()) {
    cell_cfg.ul_config->init_ul_bwp.pusch_cfg->mcs_table = mcs_table;
  }
}

// Configure dedicated UE configuration to set UL-MIMO related parameters.
static void set_ul_mimo(serving_cell_config&      cell_cfg,
                        unsigned                  max_rank,
                        unsigned                  nof_srs_ports,
                        tx_scheme_codebook_subset codebook_subset)
{
  // Skip if the UL configuration is not present.
  if (SRSRAN_UNLIKELY(!cell_cfg.ul_config.has_value() || !cell_cfg.ul_config->init_ul_bwp.pusch_cfg ||
                      !cell_cfg.ul_config->init_ul_bwp.srs_cfg)) {
    return;
  }

  // Prepare codebook transmission parameters.
  cell_cfg.ul_config->init_ul_bwp.pusch_cfg->tx_cfg =
      tx_scheme_codebook{.max_rank = max_rank, .codebook_subset = codebook_subset};

  // Force the number of ports for all SRS resources to the maximum the UE supports.
  for (auto& srs_res : cell_cfg.ul_config->init_ul_bwp.srs_cfg->srs_res_list) {
    srs_res.nof_ports = static_cast<srs_config::srs_resource::nof_srs_ports>(nof_srs_ports);
  }
}

static void set_pdsch_interleaving(serving_cell_config& cell_cfg, vrb_to_prb::mapping_type interleaving_opt)
{
  // Set \c vrb-ToPRB-Interleaver for PDSCH, as per TS 38.331, \c PDSCH-Config.
  if (cell_cfg.init_dl_bwp.pdsch_cfg.has_value()) {
    cell_cfg.init_dl_bwp.pdsch_cfg->vrb_to_prb_interleaving = interleaving_opt;
  }
}

static void set_max_dl_nof_harqs(serving_cell_config& cell_cfg, unsigned nof_harqs)
{
  if (cell_cfg.pdsch_serv_cell_cfg.has_value()) {
    cell_cfg.pdsch_serv_cell_cfg->nof_harq_proc =
        static_cast<pdsch_serving_cell_config::nof_harq_proc_for_pdsch>(nof_harqs);
  }
}

static void set_max_ul_nof_harqs(serving_cell_config& cell_cfg, unsigned nof_harqs)
{
  if (cell_cfg.ul_config.has_value() and cell_cfg.ul_config->pusch_serv_cell_cfg.has_value()) {
    cell_cfg.ul_config->pusch_serv_cell_cfg->nof_harq_proc =
        static_cast<pusch_serving_cell_config::nof_harq_proc_for_pusch>(nof_harqs);
  }
}

static void set_dl_dci_harq_num_field_size(serving_cell_config& cell_cfg, unsigned field_size)
{
  if (cell_cfg.init_dl_bwp.pdsch_cfg.has_value()) {
    cell_cfg.init_dl_bwp.pdsch_cfg->harq_process_num_size_dci_1_1 =
        static_cast<pdsch_config::harq_process_num_dci_1_1_size>(field_size);
    cell_cfg.init_dl_bwp.pdsch_cfg->harq_process_num_size_dci_1_2 =
        static_cast<pdsch_config::harq_process_num_dci_1_2_size>(field_size);
  }
}

static void set_ul_dci_harq_num_field_size(serving_cell_config& cell_cfg, unsigned field_size)
{
  if (cell_cfg.ul_config.has_value() and cell_cfg.ul_config->init_ul_bwp.pusch_cfg.has_value()) {
    cell_cfg.ul_config->init_ul_bwp.pusch_cfg->harq_process_num_size_dci_0_1 =
        static_cast<pusch_config::harq_process_num_dci_0_1_size>(field_size);
    cell_cfg.ul_config->init_ul_bwp.pusch_cfg->harq_process_num_size_dci_0_2 =
        static_cast<pusch_config::harq_process_num_dci_0_2_size>(field_size);
  }
}

static void set_ul_harq_mode_b(serving_cell_config& cell_cfg, bool harq_mode_b_supported)
{
  if (cell_cfg.ul_config.has_value() and cell_cfg.ul_config->pusch_serv_cell_cfg.has_value()) {
    cell_cfg.ul_config->pusch_serv_cell_cfg->harq_mode_b = harq_mode_b_supported;
  }
}

ue_capability_manager::ue_capability_manager(span<const du_cell_config> cell_cfg_list_,
                                             du_drx_resource_manager&   drx_mng_,
                                             srslog::basic_logger&      logger_,
                                             const du_test_mode_config& test_mode_) :
  base_cell_cfg_list(cell_cfg_list_), drx_res_mng(drx_mng_), logger(logger_), test_cfg(test_mode_)
{
}

void ue_capability_manager::handle_ue_creation(du_ue_resource_config& ue_res_cfg)
{
  du_cell_index_t      cell_idx  = to_du_cell_index(0);
  serving_cell_config& pcell_cfg = ue_res_cfg.cell_group.cells[cell_idx].serv_cell_cfg;

  // Set default MCS tables and disable UL MIMO.
  set_pdsch_mcs_table(pcell_cfg, select_pdsch_mcs_table(cell_idx));
  set_pusch_mcs_table(pcell_cfg, select_pusch_mcs_table(cell_idx));
  set_pdsch_interleaving(pcell_cfg, select_pdsch_interleaving(cell_idx));
  set_ul_mimo(
      pcell_cfg, select_pusch_max_rank(cell_idx), select_srs_nof_ports(cell_idx), select_tx_codebook_subset(cell_idx));
  set_max_dl_nof_harqs(pcell_cfg, select_max_dl_nof_harqs(cell_idx));
  set_max_ul_nof_harqs(pcell_cfg, select_max_ul_nof_harqs(cell_idx));
  set_dl_dci_harq_num_field_size(pcell_cfg, select_dl_dci_harq_num_field_size(cell_idx));
  set_ul_dci_harq_num_field_size(pcell_cfg, select_ul_dci_harq_num_field_size(cell_idx));
  set_ul_harq_mode_b(pcell_cfg, select_ul_harq_mode_b(cell_idx));

  // Initialize UE with DRX disabled.
  drx_res_mng.handle_ue_creation(ue_res_cfg.cell_group);
}

void ue_capability_manager::update(du_ue_resource_config& ue_res_cfg, const byte_buffer& ue_cap_rat_list)
{
  // Decode new UE capabilities.
  if (not decode_ue_capability_list(ue_cap_rat_list)) {
    // No changes detected in the UE capabilities, and update(...) was called before. In this case, we can do not need
    // to apply any extra changes to the ue_res_cfg that weren't already applied.
    return;
  }

  update_impl(ue_res_cfg);
}

void ue_capability_manager::update(du_ue_resource_config& ue_res_cfg, const ue_capability_summary& summary)
{
  // Store injected UE capabilities.
  ue_caps = summary;

  update_impl(ue_res_cfg);
}

void ue_capability_manager::update_impl(du_ue_resource_config& ue_res_cfg)
{
  du_cell_index_t      cell_idx  = to_du_cell_index(0);
  serving_cell_config& pcell_cfg = ue_res_cfg.cell_group.cells[cell_idx].serv_cell_cfg;

  // Enable 256QAM for PDSCH, if supported.
  set_pdsch_mcs_table(pcell_cfg, select_pdsch_mcs_table(cell_idx));

  // Enable 256QAM for PUSCH, if supported.
  set_pusch_mcs_table(pcell_cfg, select_pusch_mcs_table(cell_idx));

  // Enable VRB-to-PRB interleaving for PDSCH, if supported.
  set_pdsch_interleaving(pcell_cfg, select_pdsch_interleaving(cell_idx));

  // Setup UL MIMO parameters.
  set_ul_mimo(
      pcell_cfg, select_pusch_max_rank(cell_idx), select_srs_nof_ports(cell_idx), select_tx_codebook_subset(cell_idx));

  // Set max DL/UL nof HARQs.
  set_max_dl_nof_harqs(pcell_cfg, select_max_dl_nof_harqs(cell_idx));
  set_max_ul_nof_harqs(pcell_cfg, select_max_ul_nof_harqs(cell_idx));

  // Set DL/UL DCI HARQ Process Number size.
  set_dl_dci_harq_num_field_size(pcell_cfg, select_dl_dci_harq_num_field_size(cell_idx));
  set_ul_dci_harq_num_field_size(pcell_cfg, select_ul_dci_harq_num_field_size(cell_idx));

  // Set UL HARQ Mode B enabled.
  set_ul_harq_mode_b(pcell_cfg, select_ul_harq_mode_b(cell_idx));

  // Setup DRX config.
  update_drx(ue_res_cfg);
}

void ue_capability_manager::release(du_ue_resource_config& ue_res_cfg)
{
  // Deallocate DRX resources.
  drx_res_mng.handle_ue_removal(ue_res_cfg.cell_group);
}

bool ue_capability_manager::decode_ue_capability_list(const byte_buffer& ue_cap_rat_list)
{
  using namespace asn1::rrc_nr;

  if (ue_cap_rat_list.empty()) {
    // No update.
    return false;
  }

  ue_cap_rat_container_list_l asn1_cap_list;
  {
    asn1::cbit_ref bref{ue_cap_rat_list};
    if (asn1::unpack_dyn_seq_of(asn1_cap_list, bref, 0, 8) != asn1::SRSASN_SUCCESS) {
      logger.error("Couldn't unpack UE Capability RAT Container List RRC container");
      return false;
    }
  }

  for (const ue_cap_rat_container_s& ue_cap_rat : asn1_cap_list) {
    if (ue_cap_rat.rat_type.value != rat_type_opts::nr) {
      logger.warning("Unsupported RAT type in UE Capability RAT Container List RRC container");
      continue;
    }
    expected<ue_capability_summary, std::string> asn1_cap = decode_ue_nr_cap_container(ue_cap_rat.ue_cap_rat_container);
    if (not asn1_cap.has_value()) {
      logger.warning("{}", asn1_cap.error());
      continue;
    }
    ue_caps = asn1_cap.value();
    return true;
  }

  return false;
}

pdsch_mcs_table ue_capability_manager::select_pdsch_mcs_table(du_cell_index_t cell_idx) const
{
  const auto& init_dl_bwp = base_cell_cfg_list[cell_idx].ue_ded_serv_cell_cfg.init_dl_bwp;

  if (init_dl_bwp.pdsch_cfg.has_value()) {
    pdsch_mcs_table app_mcs_table = init_dl_bwp.pdsch_cfg.value().mcs_table;
    if (ue_caps.has_value()) {
      if (app_mcs_table == pdsch_mcs_table::qam256 and ue_caps->pdsch_qam256_supported) {
        return pdsch_mcs_table::qam256;
      }
      if (app_mcs_table == pdsch_mcs_table::qam64LowSe and ue_caps->pdsch_qam64lowse_supported) {
        return pdsch_mcs_table::qam64LowSe;
      }
    } else if (test_cfg.test_ue.has_value() and test_cfg.test_ue->rnti != rnti_t::INVALID_RNTI) {
      // Has no capabilities but the UE is in test mode.
      return app_mcs_table;
    }
  }

  // Default to QAM64 if no base cell PDSCH config of if the UE capabilities are not available.
  return pdsch_mcs_table::qam64;
}

pusch_mcs_table ue_capability_manager::select_pusch_mcs_table(du_cell_index_t cell_idx) const
{
  nr_band     band        = base_cell_cfg_list[cell_idx].ul_carrier.band;
  const auto& base_ul_cfg = base_cell_cfg_list[cell_idx].ue_ded_serv_cell_cfg.ul_config;

  if (not base_ul_cfg.has_value() or not base_ul_cfg->init_ul_bwp.pusch_cfg.has_value()) {
    // No PUSCH config present. Default to QAM64.
    return pusch_mcs_table::qam64;
  }

  const pusch_mcs_table app_mcs_table = base_ul_cfg->init_ul_bwp.pusch_cfg->mcs_table;

  if (test_cfg.test_ue.has_value() and test_cfg.test_ue->rnti != rnti_t::INVALID_RNTI) {
    // In case of test mode, we do not need to rely on capabilities.
    return app_mcs_table;
  }
  if (not ue_caps.has_value()) {
    // UE capabilities have not been decoded yet. Default to QAM64.
    return pusch_mcs_table::qam64;
  }

  if (base_ul_cfg->init_ul_bwp.pusch_cfg->mcs_table == pusch_mcs_table::qam256) {
    // If the band capability is present, select the MCS table from this band.
    if (ue_caps->bands.count(band)) {
      return ue_caps->bands.at(band).pusch_qam256_supported ? pusch_mcs_table::qam256 : pusch_mcs_table::qam64;
    }

    // In case the preferred MCS table is 256QAM, but the UE does not support it, we default to QAM64.
    if (std::none_of(ue_caps->bands.begin(), ue_caps->bands.end(), [](const auto& b) {
          return b.second.pusch_qam256_supported;
        })) {
      return pusch_mcs_table::qam64;
    }
  } else if (base_ul_cfg->init_ul_bwp.pusch_cfg->mcs_table == pusch_mcs_table::qam64LowSe) {
    return ue_caps.value().pusch_qam64lowse_supported ? pusch_mcs_table::qam64LowSe : pusch_mcs_table::qam64;
  }

  return base_ul_cfg->init_ul_bwp.pusch_cfg->mcs_table;
}

vrb_to_prb::mapping_type ue_capability_manager::select_pdsch_interleaving(du_cell_index_t cell_idx) const
{
  const auto& init_dl_bwp = base_cell_cfg_list[cell_idx].ue_ded_serv_cell_cfg.init_dl_bwp;

  if (init_dl_bwp.pdsch_cfg.has_value()) {
    vrb_to_prb::mapping_type app_pdsch_interleaving = init_dl_bwp.pdsch_cfg.value().vrb_to_prb_interleaving;
    if (ue_caps.has_value()) {
      if (ue_caps->pdsch_interleaving_vrb_to_prb_supported) {
        // If the PDSCH interleaving is enabled in the application configuration and supported by the UE, return it.
        return app_pdsch_interleaving;
      }
    } else if (test_cfg.test_ue.has_value() and test_cfg.test_ue->rnti != rnti_t::INVALID_RNTI) {
      // Has no capabilities but the UE is in test mode.
      return app_pdsch_interleaving;
    }
  }

  // Default to \c non-interleaved if no base cell PDSCH config or if the UE capabilities are not available.
  return vrb_to_prb::mapping_type::non_interleaved;
}

tx_scheme_codebook_subset ue_capability_manager::select_tx_codebook_subset(du_cell_index_t cell_idx) const
{
  nr_band band = base_cell_cfg_list[cell_idx].ul_carrier.band;

  // If UE capabilities or the band are not available, return default value.
  if (not ue_caps.has_value() || ue_caps->bands.count(band) == 0) {
    return ue_capability_summary::default_pusch_tx_coherence;
  }

  return ue_caps->bands.at(band).pusch_tx_coherence;
}

unsigned ue_capability_manager::select_srs_nof_ports(du_cell_index_t cell_idx) const
{
  nr_band band = base_cell_cfg_list[cell_idx].ul_carrier.band;

  // If UE capabilities or the band are not available, return default value.
  if (not ue_caps.has_value() || ue_caps->bands.count(band) == 0) {
    return ue_capability_summary::default_nof_srs_tx_ports;
  }

  return ue_caps->bands.at(band).nof_srs_tx_ports;
}

unsigned ue_capability_manager::select_pusch_max_rank(du_cell_index_t cell_idx) const
{
  nr_band band = base_cell_cfg_list[cell_idx].ul_carrier.band;

  // Configured maximum number of layers.
  unsigned pusch_max_rank = base_cell_cfg_list[cell_idx].pusch_max_nof_layers;

  // If UE capabilities or the band are not available, return default value.
  if (not ue_caps.has_value() || ue_caps->bands.count(band) == 0) {
    return std::min(pusch_max_rank, ue_capability_summary::default_pusch_max_rank);
  }

  return std::min(pusch_max_rank, ue_caps->bands.at(band).pusch_max_rank);
}

unsigned ue_capability_manager::select_max_dl_nof_harqs(du_cell_index_t cell_idx) const
{
  // Configured maximum number of DL HARQs.
  const auto& pdsch_serv_cell_cfg = base_cell_cfg_list[cell_idx].ue_ded_serv_cell_cfg.pdsch_serv_cell_cfg;

  if (not pdsch_serv_cell_cfg.has_value()) {
    return ue_capability_summary::default_max_harq_process_num;
  }

  auto cell_max_nof_dl_harq_proc = (unsigned)pdsch_serv_cell_cfg->nof_harq_proc;

  if (test_cfg.test_ue.has_value() and test_cfg.test_ue->rnti != rnti_t::INVALID_RNTI) {
    // In case of test mode, we do not need to rely on capabilities.
    return cell_max_nof_dl_harq_proc;
  }

  nr_band band = base_cell_cfg_list[cell_idx].dl_carrier.band;

  // TODO: need a better solution. Currently, HARQs created before UE caps are received.
  // To bypass this: if no UE capabilities available yet, but NTN band, we reserve configured nof HARQs (up to 32).
  // Better: if 32 HARQ requested in cfg, return 16 here, then if UE supports change to 32 and reserve more HARQs.
  if (not ue_caps.has_value() and band_helper::is_ntn_band(band)) {
    // UE capabilities have not been decoded yet, but NTN band.
    return cell_max_nof_dl_harq_proc;
  }

  // UE capabilities have not been decoded yet, band info no available, or UE does not support NTN, return at most 16.
  if (not ue_caps.has_value() || ue_caps->bands.count(band) == 0 || not ue_caps->ntn_supported) {
    return std::min(cell_max_nof_dl_harq_proc, ue_capability_summary::default_max_harq_process_num);
  }

  return std::min(cell_max_nof_dl_harq_proc, (unsigned)ue_caps->bands.at(band).max_ul_harq_process_num);
}

unsigned ue_capability_manager::select_max_ul_nof_harqs(du_cell_index_t cell_idx) const
{
  const auto& ul_config = base_cell_cfg_list[cell_idx].ue_ded_serv_cell_cfg.ul_config;

  if (not ul_config.has_value()) {
    return ue_capability_summary::default_max_harq_process_num;
  }

  const auto& pusch_serv_cell_cfg = ul_config->pusch_serv_cell_cfg;
  if (not pusch_serv_cell_cfg.has_value()) {
    return ue_capability_summary::default_max_harq_process_num;
  }

  // Configured maximum number of UL HARQs.
  auto cell_max_nof_ul_harq_proc = (unsigned)pusch_serv_cell_cfg->nof_harq_proc;

  if (test_cfg.test_ue.has_value() and test_cfg.test_ue->rnti != rnti_t::INVALID_RNTI) {
    // In case of test mode, we do not need to rely on capabilities.
    return cell_max_nof_ul_harq_proc;
  }

  nr_band band = base_cell_cfg_list[cell_idx].ul_carrier.band;

  // TODO: need a better solution. Currently, HARQs created before UE caps are received.
  // To bypass this: if no UE capabilities available yet, but NTN band, we reserve configured nof HARQs (up to 32).
  // Better: if 32 HARQ requested in cfg, return 16 here, then if UE supports change to 32 and reserve more HARQs.
  if (not ue_caps.has_value() and band_helper::is_ntn_band(band)) {
    // UE capabilities have not been decoded yet, but NTN band.
    return cell_max_nof_ul_harq_proc;
  }

  // UE capabilities have not been decoded yet, band info no available, or UE does not support NTN, return at most 16.
  if (not ue_caps.has_value() || ue_caps->bands.count(band) == 0 || not ue_caps->ntn_supported) {
    return std::min(cell_max_nof_ul_harq_proc, ue_capability_summary::default_max_harq_process_num);
  }

  return std::min(cell_max_nof_ul_harq_proc, (unsigned)ue_caps->bands.at(band).max_ul_harq_process_num);
}

unsigned ue_capability_manager::select_dl_dci_harq_num_field_size(du_cell_index_t cell_idx) const
{
  auto default_dci_size = log2_ceil(ue_capability_summary::default_max_harq_process_num);

  if (test_cfg.test_ue.has_value() and test_cfg.test_ue->rnti != rnti_t::INVALID_RNTI) {
    // In case of test mode, we do not need to rely on capabilities.
    return default_dci_size;
  }

  // Configured maximum number of UL HARQs.
  const auto& pdsch_serv_cell_cfg = base_cell_cfg_list[cell_idx].ue_ded_serv_cell_cfg.pdsch_serv_cell_cfg;

  if (not pdsch_serv_cell_cfg.has_value()) {
    return default_dci_size;
  }

  auto cell_max_nof_dl_harq_proc = (unsigned)pdsch_serv_cell_cfg->nof_harq_proc;
  auto cell_dci_size             = log2_ceil(cell_max_nof_dl_harq_proc);

  nr_band band = base_cell_cfg_list[cell_idx].dl_carrier.band;

  // UE capabilities have not been decoded yet, band info no available, or UE does not support NTN.
  if (not band_helper::is_ntn_band(band) || not ue_caps.has_value() || ue_caps->bands.count(band) == 0 ||
      not ue_caps->ntn_supported) {
    // return 4
    return default_dci_size;
  }

  auto band_dci_size = log2_ceil((unsigned)ue_caps->bands.at(band).max_dl_harq_process_num);
  // return 4 or 5
  return std::max(std::min(cell_dci_size, band_dci_size), default_dci_size);
}

unsigned ue_capability_manager::select_ul_dci_harq_num_field_size(du_cell_index_t cell_idx) const
{
  auto default_dci_size = log2_ceil(ue_capability_summary::default_max_harq_process_num);

  if (test_cfg.test_ue.has_value() and test_cfg.test_ue->rnti != rnti_t::INVALID_RNTI) {
    // In case of test mode, we do not need to rely on capabilities.
    return default_dci_size;
  }

  const auto& ul_config = base_cell_cfg_list[cell_idx].ue_ded_serv_cell_cfg.ul_config;

  if (not ul_config.has_value()) {
    return default_dci_size;
  }

  const auto& pusch_serv_cell_cfg = ul_config->pusch_serv_cell_cfg;
  if (not pusch_serv_cell_cfg.has_value()) {
    return default_dci_size;
  }

  // Configured maximum number of UL HARQs.
  auto cell_max_nof_ul_harq_proc = (unsigned)pusch_serv_cell_cfg->nof_harq_proc;
  auto cell_dci_size             = log2_ceil(cell_max_nof_ul_harq_proc);

  nr_band band = base_cell_cfg_list[cell_idx].ul_carrier.band;

  // UE capabilities have not been decoded yet, band info no available, or UE does not support NTN.
  if (not band_helper::is_ntn_band(band) || not ue_caps.has_value() || ue_caps->bands.count(band) == 0 ||
      not ue_caps->ntn_supported) {
    // return 4
    return default_dci_size;
  }

  auto band_dci_size = log2_ceil((unsigned)ue_caps->bands.at(band).max_ul_harq_process_num);
  // return 4 or 5
  return std::max(std::min(cell_dci_size, band_dci_size), default_dci_size);
}

bool ue_capability_manager::select_ul_harq_mode_b(du_cell_index_t cell_idx) const
{
  // Configured disabled UL HARQ mode B.
  const auto& ul_config = base_cell_cfg_list[cell_idx].ue_ded_serv_cell_cfg.ul_config;

  if (not ul_config.has_value()) {
    return false;
  }
  const auto& pusch_serv_cell_cfg = ul_config->pusch_serv_cell_cfg;
  if (not pusch_serv_cell_cfg.has_value()) {
    return false;
  }

  bool cell_harq_mode_b = pusch_serv_cell_cfg->harq_mode_b;

  if (test_cfg.test_ue.has_value() and test_cfg.test_ue->rnti != rnti_t::INVALID_RNTI) {
    // In case of test mode, we do not need to rely on capabilities.
    return cell_harq_mode_b;
  }

  // UE capabilities have not been decoded yet, but NTN band.
  if (not ue_caps.has_value() || not ue_caps->ntn_supported) {
    // If the UE does not support NTN capabilities, HARQ mode B cannot be enabled.
    return false;
  }

  return cell_harq_mode_b and ue_caps->ul_harq_mode_b_supported;
}

void ue_capability_manager::update_drx(du_ue_resource_config& ue_res_cfg)
{
  cell_group_config& cell_group = ue_res_cfg.cell_group;

  // If UE capabilities are not available or DRX is disabled, disable DRX
  const bool long_drx_supported = ue_caps.has_value() and ue_caps->long_drx_cycle_supported;

  // Allocate DRX resources if DRX is enabled in the gNB.
  drx_res_mng.handle_ue_cap_update(cell_group, long_drx_supported);
}
