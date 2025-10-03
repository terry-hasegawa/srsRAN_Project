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

namespace srsran {

/// Identification of an HARQ process.
enum harq_id_t : uint8_t { MAX_HARQ_ID = 31, MAX_NOF_HARQS = 32, INVALID_HARQ_ID = 32 };

constexpr harq_id_t to_harq_id(unsigned h_id)
{
  return static_cast<harq_id_t>(h_id);
}

/// Outcomes of a HARQ-ACK report.
enum class mac_harq_ack_report_status : int8_t { nack = 0, ack, dtx };

} // namespace srsran
