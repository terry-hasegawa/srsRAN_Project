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

#include "srsran/ran/pci.h"
#include "srsran/scheduler/scheduler_metrics.h"
#include <chrono>
#include <vector>

namespace srsran {

/// Metrics relative to a specific MAC cell.
struct mac_dl_cell_metric_report {
  struct latency_report {
    std::chrono::nanoseconds min;
    std::chrono::nanoseconds max;
    std::chrono::nanoseconds average;
    slot_point               max_slot;
  };

  /// Physical cell id.
  pci_t pci;
  /// First SFN and SLOT of the report.
  slot_point start_slot;
  /// Number of slots considered in this report.
  unsigned nof_slots;
  /// Slot duration.
  std::chrono::nanoseconds slot_duration;
  /// Description of the wall clock latency of the MAC at handling slot indications.
  latency_report wall_clock_latency;
  /// Description of the time spent by the MAC in user mode when handling slot indications.
  latency_report user_time;
  /// Description of the time spent by the MAC in kernel mode when handling slot indications.
  latency_report sys_time;
  /// \brief Description of the wall clock latency between the lower layers signalling a slot indication and the MAC
  /// starting to handle it.
  latency_report slot_ind_handle_latency;
  /// \brief Description of the delay in the MAC scheduler between receiving a slot indication and completing the
  /// scheduling
  latency_report sched_latency;
  /// \brief Description of the delays between the MAC starting the processing of a slot indication and generating a DL
  /// TTI request.
  latency_report dl_tti_req_latency;
  /// \brief Description of the delays between the MAC completing a DL TTI request and completing a TX Data request.
  latency_report tx_data_req_latency;
  /// \brief Description of the delays of the MAC at completing a UL TTI request.
  latency_report ul_tti_req_latency;
  /// \brief Description of the time difference between two consecutive FAPI slot indication messages.
  latency_report slot_ind_msg_time_diff;
  /// Number of voluntary context switches.
  unsigned count_voluntary_context_switches;
  /// Number of involuntary context switches.
  unsigned count_involuntary_context_switches;
  /// Whether the cell was marked for deactivation.
  bool cell_deactivated;
};

struct mac_dl_metric_report {
  std::vector<mac_dl_cell_metric_report> cells;
};

/// \brief Structure holding the metrics of a MAC layer.
struct mac_metric_report {
  /// Metrics of the MAC DL.
  mac_dl_metric_report dl;
  /// Metrics of the MAC scheduler.
  scheduler_metrics_report sched;
};

} // namespace srsran
