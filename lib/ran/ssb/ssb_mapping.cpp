/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "srsran/ran/ssb/ssb_mapping.h"
#include "srsran/ran/band_helper.h"
#include "srsran/scheduler/sched_consts.h"
#include "srsran/support/error_handling.h"

using namespace srsran;

uint8_t srsran::ssb_get_L_max(subcarrier_spacing ssb_scs, unsigned dl_arfcn, std::optional<nr_band> band)
{
  uint8_t L_max = 0;

  // Derive the SSB-specific parameters (SSB pattern case, SSB L_max and SSB paired_spectrum flag) from those in the
  // MAC Cell config.
  const nr_band gnb_band = band.value_or(band_helper::get_band_from_dl_arfcn(dl_arfcn));
  srsran_assert(gnb_band != nr_band::invalid, "Invalid NR band index");
  ssb_pattern_case ssb_case = band_helper::get_ssb_pattern(gnb_band, ssb_scs);

  // Flag indicating whether cell is on paired spectrum (FDD) or unpaired (TDD, SDL, SUL).
  bool paired_spectrum = band_helper::is_paired_spectrum(gnb_band);

  // Get L_max from SSB pattern case and carrier frequency and paired spectrum flag.
  uint32_t f_arfcn = dl_arfcn;
  switch (ssb_case) {
    case ssb_pattern_case::A:
    case ssb_pattern_case::B:
      L_max = (f_arfcn < CUTOFF_FREQ_ARFCN_CASE_A_B_C) ? 4 : 8;
      break;
    case ssb_pattern_case::C: {
      uint32_t ssb_cutoff_freq = paired_spectrum ? CUTOFF_FREQ_ARFCN_CASE_A_B_C : CUTOFF_FREQ_ARFCN_CASE_C_UNPAIRED;
      L_max                    = (f_arfcn < ssb_cutoff_freq) ? 4 : 8;
    } break;
    case ssb_pattern_case::D:
    case ssb_pattern_case::E:
      L_max = 64;
      break;
    case ssb_pattern_case::invalid:
      report_fatal_error("Invalid SS/PBCH block case for n{} and {}.", nr_band_to_uint(gnb_band), to_string(ssb_scs));
  }

  return L_max;
}

crb_interval srsran::get_ssb_crbs(subcarrier_spacing    ssb_scs,
                                  subcarrier_spacing    scs_common,
                                  ssb_offset_to_pointA  offset_to_pA,
                                  ssb_subcarrier_offset k_ssb)
{
  report_fatal_error_if_not(ssb_scs == scs_common, "Mixed numerology not supported");
  report_fatal_error_if_not((scs_common <= subcarrier_spacing::kHz30) || (scs_common == subcarrier_spacing::kHz120),
                            "{} subcarrier spacing for SCScommon is not supported",
                            to_string(scs_common));

  // With SCScommon kHz30, offset_to_pointA must be a multiple of 2. This is because it is measured in 15kHz RB, while
  // it points at a CRB which is based on 30kHz.
  if ((scs_common == subcarrier_spacing::kHz30) || ((scs_common == subcarrier_spacing::kHz120))) {
    srsran_sanity_check(offset_to_pA.to_uint() % 2 == 0,
                        "With SCScommon with {} OffsetToPointA must be an even number",
                        to_string(scs_common));
  }
  unsigned ssb_crbs_start =
      scs_common == subcarrier_spacing::kHz15 ? offset_to_pA.to_uint() : offset_to_pA.to_uint() / 2;
  // TODO: Extent this function to cover mixed numerologies. Depending on the reference grid, NOF_SSB_PRBS might be
  //       multiplied or divided by 2.

  // As per TS 38.211, Section 7.4.3.1, the SSB occupies 240 subcarriers, or 20 PRBs. In the case of FR1, and SSB SCS ==
  // SCScommon, if k_SSB > 1, the SSB PRBs will be shifted with respect to the Common RBs grid; this means that the SSB
  // will overlap over 1 additional CRB (at the end of the SS/PBCH Block).
  unsigned ssb_crbs_stop = k_ssb.to_uint() > 0 ? ssb_crbs_start + NOF_SSB_PRBS + 1 : ssb_crbs_start + NOF_SSB_PRBS;

  return crb_interval{ssb_crbs_start, ssb_crbs_stop};
}
