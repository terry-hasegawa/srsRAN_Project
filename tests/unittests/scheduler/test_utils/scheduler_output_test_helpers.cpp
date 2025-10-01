/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "scheduler_output_test_helpers.h"
#include "lib/scheduler/cell/resource_grid.h"
#include "lib/scheduler/support/pdcch/pdcch_mapping.h"
#include "lib/scheduler/support/sched_result_helpers.h"
#include "srsran/ran/pdcch/cce_to_prb_mapping.h"
#include "srsran/ran/prach/prach_configuration.h"
#include "srsran/ran/prach/prach_frequency_mapping.h"
#include "srsran/ran/prach/prach_preamble_information.h"
#include "srsran/ran/resource_allocation/vrb_to_prb.h"

using namespace srsran;

std::vector<grant_info> srsran::get_pdcch_grant_info(pci_t pci, const pdcch_dl_information& pdcch)
{
  std::vector<grant_info> grants;

  const bwp_configuration&     bwp_cfg = *pdcch.ctx.bwp_cfg;
  const coreset_configuration& cs_cfg  = *pdcch.ctx.coreset_cfg;
  prb_index_list               pdcch_prbs =
      pdcch_helper::cce_to_prb_mapping(bwp_cfg, cs_cfg, pci, pdcch.ctx.cces.aggr_lvl, pdcch.ctx.cces.ncce);
  for (unsigned prb : pdcch_prbs) {
    unsigned crb = prb_to_crb(bwp_cfg, prb);
    grants.push_back(grant_info{bwp_cfg.scs, ofdm_symbol_range{0U, (uint8_t)cs_cfg.duration}, {crb, crb + 1}});
  }
  return grants;
}

std::vector<grant_info> srsran::get_pdcch_grant_info(pci_t pci, const pdcch_ul_information& pdcch)
{
  std::vector<grant_info> grants;

  const bwp_configuration&     bwp_cfg = *pdcch.ctx.bwp_cfg;
  const coreset_configuration& cs_cfg  = *pdcch.ctx.coreset_cfg;
  prb_index_list               pdcch_prbs =
      pdcch_helper::cce_to_prb_mapping(bwp_cfg, cs_cfg, pci, pdcch.ctx.cces.aggr_lvl, pdcch.ctx.cces.ncce);
  for (unsigned prb : pdcch_prbs) {
    unsigned crb = prb_to_crb(bwp_cfg, prb);
    grants.push_back(grant_info{bwp_cfg.scs, ofdm_symbol_range{0U, (uint8_t)cs_cfg.duration}, {crb, crb + 1}});
  }
  return grants;
}

static grant_info get_common_pdsch_grant_info(const bwp_downlink_common& bwp_cfg, const pdsch_information& pdsch)
{
  crb_interval cs0_crbs = bwp_cfg.pdcch_common.coreset0->coreset0_crbs();
  crb_interval crbs     = {pdsch.rbs.type1().start() + cs0_crbs.start(), pdsch.rbs.type1().stop() + cs0_crbs.start()};
  return grant_info{bwp_cfg.generic_params.scs, pdsch.symbols, crbs};
}

grant_info srsran::get_pdsch_grant_info(const bwp_downlink_common& bwp_cfg, const sib_information& sib)
{
  return get_common_pdsch_grant_info(bwp_cfg, sib.pdsch_cfg);
}

grant_info srsran::get_pdsch_grant_info(const bwp_downlink_common& bwp_cfg, const rar_information& rar)
{
  return get_common_pdsch_grant_info(bwp_cfg, rar.pdsch_cfg);
}

grant_info srsran::get_pdsch_grant_info(const bwp_downlink_common& bwp_cfg, const dl_paging_allocation& pg)
{
  // See TS 38.212, section 7.3.1.2.1. DCI Format 1_0.
  return get_common_pdsch_grant_info(bwp_cfg, pg.pdsch_cfg);
}

std::pair<grant_info, grant_info> srsran::get_pdsch_grant_info(const bwp_downlink_common& bwp_cfg,
                                                               const dl_msg_alloc&        ue_grant,
                                                               vrb_to_prb::mapping_type   interleaving_bundle_size)
{
  const vrb_interval vrbs   = ue_grant.pdsch_cfg.rbs.type1();
  unsigned           ref_rb = 0;
  if (ue_grant.pdsch_cfg.ss_set_type != search_space_set_type::ue_specific and
      ue_grant.pdsch_cfg.dci_fmt == dci_dl_format::f1_0) {
    ref_rb = ue_grant.pdsch_cfg.coreset_cfg->get_coreset_start_crb();
  } else {
    ref_rb = ue_grant.pdsch_cfg.bwp_cfg->crbs.start();
  }

  if (ue_grant.pdsch_cfg.vrb_prb_mapping != vrb_to_prb::mapping_type::non_interleaved) {
    vrb_to_prb::interleaved_mapping mapping(vrb_to_prb::create_interleaved_other(
        ref_rb, ue_grant.pdsch_cfg.bwp_cfg->crbs.length(), interleaving_bundle_size));
    const auto                      prbs = mapping.vrb_to_prb(vrbs);
    return {
        grant_info{ue_grant.pdsch_cfg.bwp_cfg->scs,
                   ue_grant.pdsch_cfg.symbols,
                   prb_to_crb(ue_grant.pdsch_cfg.bwp_cfg->crbs, prbs.first)},
        grant_info{ue_grant.pdsch_cfg.bwp_cfg->scs,
                   ue_grant.pdsch_cfg.symbols,
                   prb_to_crb(ue_grant.pdsch_cfg.bwp_cfg->crbs, prbs.second)},
    };
  } else {
    crb_interval crbs = {vrbs.start() + ref_rb, vrbs.stop() + ref_rb};
    return {
        grant_info{ue_grant.pdsch_cfg.bwp_cfg->scs, ue_grant.pdsch_cfg.symbols, crbs},
        grant_info{},
    };
  }
}

std::vector<test_grant_info> srsran::get_dl_grants(const cell_configuration& cell_cfg, const dl_sched_result& dl_res)
{
  std::vector<test_grant_info> grants;

  // Fill SSB.
  for (const ssb_information& ssb : dl_res.bc.ssb_info) {
    grants.emplace_back();
    grants.back().type  = test_grant_info::SSB;
    grants.back().rnti  = rnti_t::INVALID_RNTI;
    grants.back().grant = grant_info{cell_cfg.ssb_cfg.scs, ssb.symbols, ssb.crbs};
  }

  // Fill DL PDCCHs.
  for (const pdcch_dl_information& pdcch : dl_res.dl_pdcchs) {
    std::vector<grant_info> grant_res_list = get_pdcch_grant_info(cell_cfg.pci, pdcch);
    for (const grant_info& grant : grant_res_list) {
      grants.emplace_back();
      grants.back().type  = test_grant_info::DL_PDCCH;
      grants.back().rnti  = pdcch.ctx.rnti;
      grants.back().grant = grant;
    }
  }

  // Fill UL PDCCHs.
  for (const pdcch_ul_information& pdcch : dl_res.ul_pdcchs) {
    std::vector<grant_info> grant_res_list = get_pdcch_grant_info(cell_cfg.pci, pdcch);
    for (const grant_info& grant : grant_res_list) {
      grants.emplace_back();
      grants.back().type  = test_grant_info::UL_PDCCH;
      grants.back().rnti  = pdcch.ctx.rnti;
      grants.back().grant = grant;
    }
  }

  // Fill SIB1 PDSCH.
  for (const sib_information& sib : dl_res.bc.sibs) {
    grants.emplace_back();
    grants.back().type  = test_grant_info::SIB;
    grants.back().rnti  = sib.pdsch_cfg.rnti;
    grants.back().grant = get_pdsch_grant_info(cell_cfg.dl_cfg_common.init_dl_bwp, sib);
  }

  // Register RAR PDSCHs.
  for (const rar_information& rar : dl_res.rar_grants) {
    grants.emplace_back();
    grants.back().type  = test_grant_info::RAR;
    grants.back().rnti  = rar.pdsch_cfg.rnti;
    grants.back().grant = get_pdsch_grant_info(cell_cfg.dl_cfg_common.init_dl_bwp, rar);
  }

  // Register UE PDSCHs.
  for (const dl_msg_alloc& ue_pdsch : dl_res.ue_grants) {
    const auto pdsch_grants = get_pdsch_grant_info(
        cell_cfg.dl_cfg_common.init_dl_bwp, ue_pdsch, cell_cfg.expert_cfg.ue.pdsch_interleaving_bundle_size);
    grants.emplace_back();
    grants.back().type  = test_grant_info::UE_DL;
    grants.back().rnti  = ue_pdsch.pdsch_cfg.rnti;
    grants.back().grant = pdsch_grants.first;
    if (not pdsch_grants.second.crbs.empty()) {
      grants.emplace_back();
      grants.back().type  = test_grant_info::UE_DL;
      grants.back().rnti  = ue_pdsch.pdsch_cfg.rnti;
      grants.back().grant = pdsch_grants.second;
    }
  }

  for (const dl_paging_allocation& pg : dl_res.paging_grants) {
    grants.emplace_back();
    grants.back().type  = test_grant_info::PAGING;
    grants.back().rnti  = pg.pdsch_cfg.rnti;
    grants.back().grant = get_pdsch_grant_info(cell_cfg.dl_cfg_common.init_dl_bwp, pg);
  }

  return grants;
}

std::vector<test_grant_info> srsran::get_ul_grants(const cell_configuration& cell_cfg, const ul_sched_result& ul_res)
{
  std::vector<test_grant_info> grants;

  // Fill PUSCHs.
  for (const ul_sched_info& pusch : ul_res.puschs) {
    grants.emplace_back();
    grants.back().type  = test_grant_info::UE_UL;
    grants.back().rnti  = rnti_t::INVALID_RNTI;
    grants.back().grant = get_pusch_grant_info(pusch);
  }

  // Fill PRACHs.
  if (not ul_res.prachs.empty()) {
    for (const grant_info& grant : get_prach_grant_info(cell_cfg, ul_res.prachs)) {
      grants.emplace_back();
      grants.back().type  = test_grant_info::PRACH;
      grants.back().rnti  = rnti_t::INVALID_RNTI;
      grants.back().grant = grant;
    }
  }

  // Fill PUCCHs.
  for (const pucch_info& pucch : ul_res.pucchs) {
    const auto pucch_grants = get_pucch_grant_info(pucch);
    grants.emplace_back();
    grants.back().type  = test_grant_info::PUCCH;
    grants.back().rnti  = rnti_t::INVALID_RNTI;
    grants.back().grant = pucch_grants.first;
    if (pucch_grants.second.has_value()) {
      // Add a second resource for Frequency Hopping.
      grants.emplace_back();
      grants.back().type  = test_grant_info::PUCCH;
      grants.back().rnti  = rnti_t::INVALID_RNTI;
      grants.back().grant = *pucch_grants.second;
    }
  }

  return grants;
}
