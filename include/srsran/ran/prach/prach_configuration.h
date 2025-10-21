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
#include "srsran/ran/duplex_mode.h"
#include "srsran/ran/frequency_range.h"
#include "srsran/ran/prach/prach_format_type.h"
#include "srsran/ran/slot_point.h"
#include <cstdint>

namespace srsran {

/// Indicates the number of SSBs per RACH occasion (L1 parameter 'SSB-per-rach-occasion'). See TS 38.331, \c
/// ssb-perRACH-OccasionAndCB-PreamblesPerSSB. Values {1/8, 1/4, 1/2, 1, 2, 4, 8, 16}.
/// Value 1/8 corresponds to one SSB associated with 8 RACH occasions and so on so forth.
enum class ssb_per_rach_occasions : uint8_t { one_eighth = 0, one_forth, one_half, one, two, four, eight, sixteen };

inline float ssb_per_rach_occ_to_float(ssb_per_rach_occasions value)
{
  return static_cast<float>(1U << (static_cast<unsigned>(value))) / 8.0f;
}

/// \brief Contains the PRACH configuration parameters.
///
/// The parameters are used in TS38.211 Section 6.3.3.2 and they are derived from TS38.211 Tables 6.3.3.2-2, 6.3.3.2-3
/// and 6.3.3.2-4.
///
/// The PRACH transmission occasions are in the system frames \f$n_{SFN}\f$ that satisfy \f$n_{SFN} \bmod x = y\f$.
struct prach_configuration {
  /// Maximum number of system frame offsets.
  static constexpr unsigned max_nof_sfn_offsets = 2;
  /// Maximum number of PRACH slots per radio frame assuming a subcarrier spacing of 60kHz.
  static constexpr unsigned max_nof_slots_60kHz_frame =
      NOF_SUBFRAMES_PER_FRAME * (1U << to_numerology_value(subcarrier_spacing::kHz60));

  /// Preamble format (see [here](\ref preamble_format) for more information).
  prach_format_type format;
  /// SFN period, \f$x\f$.
  unsigned x;
  /// SFN offset \f$y\f$.
  static_vector<uint8_t, max_nof_sfn_offsets> y;
  /// \brief Slots within a radio frame that contain PRACH occasions.
  ///
  /// The slot numbering assumes the subcarrier spacing:
  /// - 15kHz for FR1; and
  /// - 60kHz for FR2.
  static_vector<uint8_t, max_nof_slots_60kHz_frame> slots;
  /// Starting OFDM symbol index.
  uint8_t starting_symbol;
  /// \brief Number of PRACH slots. Set zero for reserved.
  ///
  /// Depending on the frequency range:
  /// - FR1: within a subframe (15 kHz slot); or
  /// - FR2: within a 60 kHz slot.
  uint8_t nof_prach_slots_within_subframe;
  /// Number of time-domain PRACH occasions within a PRACH slot. Set zero for reserved.
  uint8_t nof_occasions_within_slot;
  /// PRACH duration in symbols. Only valid for short preamble formats.
  uint8_t duration;
};

/// Reserved PRACH configuration. Indicates the configuration parameters are invalid.
const prach_configuration PRACH_CONFIG_RESERVED = {prach_format_type::invalid, UINT32_MAX, {}, {}, 0, 0, 0, 0};

/// \brief Gets a PRACH configuration.
///
/// Arguments are considered valid if:
/// - They are consistent with the TS38.211 Tables 6.3.3.2-2, 6.3.3.2-3 and 6.3.3.2-4, and
/// - the combination is implemented.
///
/// Currently, only long preamble formats for FR1 in FDD mode are implemented.
///
/// \param[in] fr                 Frequency range (see [here](\ref frequency_range) for more information).
/// \param[in] dm                 Duplex mode (see [here](\ref duplex_mode) for more information).
/// \param[in] prach_config_index PRACH configuration index with range {0, ..., 255}.
/// \return A valid PRACH configuration if the provided arguments are valid. Otherwise, \ref PRACH_CONFIG_RESERVED.
prach_configuration prach_configuration_get(frequency_range fr, duplex_mode dm, uint8_t prach_config_index);

} // namespace srsran
