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

#include "../../phy_test_utils.h"
#include "srsran/phy/upper/signal_processors/pdcch/factories.h"

namespace srsran {

class dmrs_pdcch_processor_spy : public dmrs_pdcch_processor
{
private:
  struct entry_t {
    config_t              config;
    resource_grid_writer* grid;
  };
  std::vector<entry_t> entries;

public:
  void map(resource_grid_writer& grid, const config_t& config) override
  {
    entry_t entry = {};
    entry.config  = config;
    entry.grid    = &grid;
    entries.emplace_back(entry);
  }
  void                        reset() { entries.clear(); }
  unsigned                    get_nof_entries() const { return entries.size(); }
  const std::vector<entry_t>& get_entries() const { return entries; }
};

PHY_SPY_FACTORY_TEMPLATE(dmrs_pdcch_processor);

} // namespace srsran
