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

#include "srsran/phy/lower/modulation/ofdm_demodulator.h"
#include "srsran/phy/support/resource_grid_writer.h"
#include "srsran/srslog/srslog.h"
#include "srsran/srsvec/zero.h"
#include "srsran/support/memory_pool/bounded_object_pool.h"

namespace srsran {

/// Implements an OFDM demodulator concurrent pool.
class ofdm_symbol_demodulator_pool : public ofdm_symbol_demodulator
{
public:
  /// Internal pool of demodulators.
  using demodulator_pool = bounded_unique_object_pool<ofdm_symbol_demodulator>;

  /// \brief Constructs an OFDM symbol demodulator concurrent pool.
  /// \param[in] base_         Base instance for getting the symbol size.
  /// \param[in] demodulators_ Shared pool of OFDM demodulators.
  ofdm_symbol_demodulator_pool(std::unique_ptr<ofdm_symbol_demodulator> base_,
                               std::shared_ptr<demodulator_pool>        demodulators_) :
    logger(srslog::fetch_basic_logger("PHY")), base(std::move(base_)), demodulators(std::move(demodulators_))
  {
  }

  // See the interface for documentation.
  unsigned get_symbol_size(unsigned symbol_index) const override { return base->get_symbol_size(symbol_index); }

  // See the interface for documentation.
  void set_center_frequency(double center_frequency_Hz_) override
  {
    center_frequency_Hz.store(center_frequency_Hz_, std::memory_order_relaxed);
  }

  // See the interface for documentation.
  void
  demodulate(resource_grid_writer& grid, span<const cf_t> input, unsigned port_index, unsigned symbol_index) override
  {
    auto demodulator = demodulators->get();
    if (!demodulator) {
      srsvec::zero(grid.get_view(port_index, symbol_index));
      logger.error("Insufficient number of OFDM demodulator instances.");
      return;
    }

    double current_center_frequency_Hz = center_frequency_Hz.load(std::memory_order_relaxed);
    if (std::isnormal(current_center_frequency_Hz)) {
      demodulator->set_center_frequency(current_center_frequency_Hz);
    }

    demodulator->demodulate(grid, input, port_index, symbol_index);
  }

private:
  std::atomic<double>                      center_frequency_Hz = {};
  srslog::basic_logger&                    logger;
  std::unique_ptr<ofdm_symbol_demodulator> base;
  std::shared_ptr<demodulator_pool>        demodulators;
};

} // namespace srsran
