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

#include "srsran/phy/upper/sequence_generators/pseudo_random_generator.h"
#include "srsran/phy/upper/signal_processors/channel_estimator/port_channel_estimator.h"
#include "srsran/phy/upper/signal_processors/pucch/dmrs_pucch_estimator.h"

namespace srsran {

/// Generic implementation of a DM-RS channel estimator for PUCCH Format 2.
class dmrs_pucch_estimator_format2
{
public:
  using layer_dmrs_pattern = port_channel_estimator::layer_dmrs_pattern;

  dmrs_pucch_estimator_format2(std::unique_ptr<pseudo_random_generator> prg_,
                               std::unique_ptr<port_channel_estimator>  ch_est_) :
    prg(std::move(prg_)), ch_estimator(std::move(ch_est_))
  {
    srsran_assert(prg, "Invalid pseudo random generator.");
    srsran_assert(ch_estimator, "Invalid port channel estimator.");
  }

  /// Estimates the channel of a PUCCH Format 2 transmission. See \ref dmrs_pucch_estimator for more details.
  void estimate(channel_estimate&                                  ce_,
                const resource_grid_reader&                        grid,
                const dmrs_pucch_estimator::format2_configuration& config);

private:
  /// Number of DM-RS RE per Resource Block.
  static constexpr unsigned NOF_DMRS_PER_RB = 4;
  /// Pseudo-random sequence generator instance.
  std::unique_ptr<pseudo_random_generator> prg;
  /// Antenna port channel estimator.
  std::unique_ptr<port_channel_estimator> ch_estimator;
  /// Buffer for DM-RS symbols.
  dmrs_symbol_list temp_symbols;

  /// \brief Computes the initial pseudo-random state.
  /// \remark Implemented according to TS38.211 Section 6.4.1.3.2.1.
  ///
  /// \param[in] symbol OFDM symbol index.
  /// \param[in] config PUCCH configuration parameters.
  /// \return    The initial pseudo-random state.
  static unsigned c_init(unsigned symbol, const dmrs_pucch_estimator::format2_configuration& config);

  /// \brief Generates a DM-RS sequence according to TS38.211 Section 6.4.1.3.2.1.
  ///
  /// \param[out] sequence  Sequence destination.
  /// \param[in]  symbol    Symbol index.
  /// \param[in]  start_prb Index of first PRB allocated for DM-RS in the current symbol.
  /// \param[in]  config    Required parameters to calculate the sequences.
  void generate_sequence(span<cf_t>                                         sequence,
                         unsigned                                           symbol,
                         unsigned                                           start_prb,
                         const dmrs_pucch_estimator::format2_configuration& config);

  /// \brief Generates the PUCCH DM-RS allocation pattern.
  ///
  /// Implements the PUCCH DM-RS mapping, as described in TS38.211 Section 6.4.1.3.2.2.
  /// \param[in] config Configuration parameters.
  /// \return The DM-RS allocation pattern.
  static layer_dmrs_pattern generate_dmrs_pattern(const dmrs_pucch_estimator::format2_configuration& config);
};

} // namespace srsran
