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

#include "mac_dl_configurator.h"
#include "srsran/adt/noop_functor.h"
#include "srsran/adt/spsc_queue.h"
#include "srsran/mac/mac_metrics.h"
#include "srsran/ran/pci.h"
#include "srsran/ran/slot_point.h"
#include "srsran/ran/subcarrier_spacing.h"
#include "srsran/support/tracing/resource_usage.h"
#include <memory>

namespace srsran {

class mac_metrics_notifier;
class task_executor;
class timer_manager;

/// Clock time point type used for metric capture.
using metric_clock = std::chrono::steady_clock;

/// Handling of MAC DL cell metrics.
class mac_dl_cell_metric_handler
{
public:
  struct slot_measurement {
    slot_measurement() = default;
    slot_measurement(mac_dl_cell_metric_handler& parent_,
                     slot_point                  sl_tx_,
                     metric_clock::time_point    slot_ind_enqueue_tp_) :
      parent(&parent_), sl_tx(sl_tx_), slot_ind_enqueue_tp(slot_ind_enqueue_tp_)
    {
      if (enabled()) {
        start_tp   = metric_clock::now();
        start_rusg = resource_usage::now();
        if (not parent->active()) {
          parent->on_cell_activation(sl_tx);
        }
      }
    }
    ~slot_measurement()
    {
      if (enabled()) {
        parent->handle_slot_completion(*this);
      }
    }

    bool enabled() const { return parent != nullptr; }

    auto start_time_point() const { return start_tp; }

    void on_sched()
    {
      if (enabled()) {
        sched_tp = metric_clock::now();
      }
    }

    void on_dl_tti_req()
    {
      if (enabled()) {
        dl_tti_req_tp = metric_clock::now();
      }
    }

    void on_tx_data_req()
    {
      if (enabled()) {
        tx_data_req_tp = metric_clock::now();
      }
    }

    void on_ul_tti_req()
    {
      if (enabled()) {
        ul_tti_req_tp = metric_clock::now();
      }
    }

  private:
    friend class mac_dl_cell_metric_handler;

    std::unique_ptr<mac_dl_cell_metric_handler, noop_operation> parent;
    slot_point                                                  sl_tx;
    metric_clock::time_point                                    slot_ind_enqueue_tp;
    metric_clock::time_point                                    start_tp;
    metric_clock::time_point                                    sched_tp;
    metric_clock::time_point                                    dl_tti_req_tp;
    metric_clock::time_point                                    tx_data_req_tp;
    metric_clock::time_point                                    ul_tti_req_tp;
    expected<resource_usage::snapshot, int>                     start_rusg;
  };

  mac_dl_cell_metric_handler(pci_t cell_pci, subcarrier_spacing scs, mac_cell_metric_notifier* notifier);

  /// Called when the MAC cell is deactivated.
  void on_cell_deactivation();

  /// Initiate new slot measurements.
  /// \param[in] sl_tx Tx slot.
  /// \param[in] slot_ind_trigger_tp Time point when the slot_indication was signalled by the lower layers.
  auto start_slot(slot_point sl_tx, metric_clock::time_point slot_ind_trigger_tp)
  {
    if (not enabled()) {
      return slot_measurement{};
    }
    return slot_measurement{*this, sl_tx, slot_ind_trigger_tp};
  }

private:
  /// Called when the MAC cell is activated.
  void on_cell_activation(slot_point sl_tx);

  bool active() const { return last_sl_tx.valid(); }

  bool enabled() const { return notifier != nullptr; }

  struct non_persistent_data {
    struct latency_data {
      unsigned                 count{0};
      std::chrono::nanoseconds min{std::chrono::nanoseconds::max()};
      std::chrono::nanoseconds max{0};
      std::chrono::nanoseconds sum{0};
      slot_point               max_slot;

      mac_dl_cell_metric_report::latency_report get_report() const;

      void save_sample(slot_point sl_tx, std::chrono::nanoseconds tdiff);
    };

    unsigned     nof_slots = 0;
    slot_point   start_slot;
    latency_data wall;
    latency_data user;
    latency_data sys;
    latency_data slot_enqueue;
    latency_data sched;
    latency_data dl_tti_req;
    latency_data tx_data_req;
    latency_data ul_tti_req;
    latency_data slot_distance;
    unsigned     count_vol_context_switches{0};
    unsigned     count_invol_context_switches{0};
    /// \brief Whether the cell was marked for deactivation and this is the last report.
    bool last_report = false;
  };

  void handle_slot_completion(const slot_measurement& meas);

  void send_new_report();

  const pci_t                    cell_pci;
  mac_cell_metric_notifier*      notifier;
  const std::chrono::nanoseconds slot_duration;

  // Last slot indication slot point. If not set, the cell is inactive.
  slot_point last_sl_tx;

  // Metrics tracked
  /// Data that is reset on every report.
  non_persistent_data data;
  /// Time point when the last slot indication was sent.
  metric_clock::time_point last_slot_ind_enqueue_tp{};
};

} // namespace srsran
