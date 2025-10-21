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

#include "srsran/adt/complex.h"
#include "srsran/adt/span.h"
#include "srsran/phy/support/interpolator.h"
#include "srsran/phy/support/re_buffer.h"
#include "srsran/phy/support/resource_grid_reader.h"
#include "srsran/phy/support/time_alignment_estimator/time_alignment_estimator.h"
#include "srsran/phy/upper/signal_processors/channel_estimator/port_channel_estimator.h"
#include "srsran/phy/upper/signal_processors/channel_estimator/port_channel_estimator_parameters.h"

namespace srsran {

/// \brief Extracts channel observations corresponding to DM-RS pilots from the resource grid for one layer, one hop
/// and for the selected port.
/// \param[out] rx_symbols  Symbol buffer destination.
/// \param[in]  grid        Resource grid.
/// \param[in]  port        Port index.
/// \param[in]  cfg         Configuration parameters of the current context.
/// \param[in]  hop         Intra-slot frequency hopping index: 0 for first position (before hopping), 1 for second
///                         position (after hopping).
/// \param[in] i_layer      Layer index.
/// \return The number of OFDM symbols containing DM-RS for the given layer and hop.
unsigned extract_layer_hop_rx_pilots(dmrs_symbol_list&                            rx_symbols,
                                     const resource_grid_reader&                  grid,
                                     unsigned                                     port,
                                     const port_channel_estimator::configuration& cfg,
                                     unsigned                                     hop,
                                     unsigned                                     i_layer = 0);

/// \brief Applies frequency domain smoothing strategy.
/// \param[out] enlarged_filtered_pilots_out   Smoothed pilots estimates.
/// \param[in]  enlarged_pilots_in             Pilots estimates.
/// \param[in]  nof_rb                         Number of resource blocks.
/// \param[in]  stride                         Reference signals stride in frequency domain.
/// \param[in]  fd_smoothing_strategy          Frequency domain smoothing strategy.
void apply_fd_smoothing(span<cf_t>                                   enlarged_filtered_pilots_out,
                        span<cf_t>                                   enlarged_pilots_in,
                        unsigned                                     nof_rb,
                        unsigned                                     stride,
                        port_channel_estimator_fd_smoothing_strategy fd_smoothing_strategy);

/// \brief Estimates the time alignment based on one hop.
///
/// \param[in] pilots_lse   The estimated channel (only for REs carrying DM-RS).
/// \param[in] pattern      DM-RS pattern for the current layer.
/// \param[in] hop          Intra-slot frequency hopping index: 0 for first position (before hopping), 1 for second
///                         position (after hopping).
/// \param[in] scs          Subcarrier spacing.
/// \param[in] ta_estimator Time alignment estimator.
/// \return The estimated time alignment as a number of samples (the sampling frequency is given by the DFT processor).
float estimate_time_alignment(const re_measurement<cf_t>&                       pilots_lse,
                              const port_channel_estimator::layer_dmrs_pattern& pattern,
                              unsigned                                          hop,
                              subcarrier_spacing                                scs,
                              time_alignment_estimator&                         ta_estimator);

// Returns the interpolator configuration for the given RE pattern.
interpolator::configuration configure_interpolator(const bounded_bitset<NRE>& re_mask);

/// \brief Extract resource elements from an OFDM symbol view.
/// \param[out] dmrs_symbols     Extracted resource elements.
/// \param[in]  ofdm_symbol_view Resource grid OFDM symbol view.
/// \param[in]  hop_rb_mask      Resource block selector mask.
/// \param[in]  re_pattern       Resource element pattern within resource blocks.
void extract_dmrs_grid(span<cf_t>                    dmrs_symbols,
                       span<const cbf16_t>           ofdm_symbol_view,
                       const bounded_bitset<MAX_RB>& hop_rb_mask,
                       const bounded_bitset<NRE>&    re_pattern);

} // namespace srsran
