/*
 *
 * Copyright 2021-2024 Software Radio Systems Limited
 *
 * This file is part of srsRAN.
 *
 * srsRAN is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsRAN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#include "asn1_helpers.h"
#include "srsran/ran/bcd_helpers.h"

using namespace srsran;

nr_cell_global_id_t srsran::cgi_from_asn1(const asn1::f1ap::nr_cgi_s& asn1_cgi)
{
  nr_cell_global_id_t cgi          = {};
  uint32_t            encoded_plmn = asn1_cgi.plmn_id.to_number();
  ngap_plmn_to_mccmnc(encoded_plmn, &cgi.mcc, &cgi.mnc);

  std::string mcc_string, mnc_string;
  mcc_to_string(cgi.mcc, &mcc_string);
  mnc_to_string(cgi.mnc, &mnc_string);
  cgi.plmn = mcc_string + mnc_string;

  // Set PLMN hex string
  cgi.plmn_hex = asn1_cgi.plmn_id.to_string();

  cgi.nci = asn1_cgi.nr_cell_id.to_number();

  return cgi;
}

pdcp_sn_size srsran::pdcp_sn_size_from_f1ap_asn1(const asn1::f1ap::pdcp_sn_len_e& asn1_pdcp_sn_size)
{
  switch (asn1_pdcp_sn_size) {
    case asn1::f1ap::pdcp_sn_len_opts::twelve_bits:
      return pdcp_sn_size::size12bits;
    case asn1::f1ap::pdcp_sn_len_opts::eighteen_bits:
      return pdcp_sn_size::size18bits;
    default:
      return pdcp_sn_size::invalid;
  }
}

asn1::f1ap::pdcp_sn_len_e srsran::pdcp_sn_size_to_f1ap_asn1(pdcp_sn_size sn_size)
{
  switch (sn_size) {
    case pdcp_sn_size::size12bits:
      return asn1::f1ap::pdcp_sn_len_opts::twelve_bits;
    case pdcp_sn_size::size18bits:
      return asn1::f1ap::pdcp_sn_len_opts::eighteen_bits;
    default:
      return asn1::f1ap::pdcp_sn_len_opts::nulltype;
  }
}
