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

#include "srsran/cu_cp/cu_cp_types.h"
#include "srsran/e1ap/common/e1_setup_messages.h"
#include "srsran/e1ap/common/e1ap_common.h"
#include "srsran/e1ap/cu_cp/e1ap_cu_cp_bearer_context_update.h"
#include "srsran/support/async/async_task.h"

namespace srsran {
namespace srs_cu_cp {

/// Handle E1AP interface management procedures as defined in TS 38.463 section 8.2.
class e1ap_connection_manager
{
public:
  virtual ~e1ap_connection_manager() = default;

  /// \brief Initiate the E1 Reset procedure as per TS 38.483 section 8.2.1.
  /// \param[in] reset The E1 Reset message to transmit.
  virtual async_task<void> handle_cu_cp_e1_reset_message(const cu_cp_reset& reset) = 0;

  /// \brief Creates and transmits the E1 Setup outcome to the CU-UP.
  /// \param[in] msg The cu_up_e1_setup_response to transmit.
  /// \remark The CU-CP transmits the E1SetupResponse/E1SetupFailure as per TS 38.463 section 8.2.3.
  virtual void handle_cu_up_e1_setup_response(const cu_up_e1_setup_response& msg) = 0;
};

/// Handle E1AP bearer context management procedures as defined in TS 38.463 section 8.3.
class e1ap_bearer_context_manager
{
public:
  virtual ~e1ap_bearer_context_manager() = default;

  /// \brief Initiates the Bearer Context Setup procedure as per TS 38.463 section 8.3.1.
  /// \param[in] request The Bearer Context Setup Request message to transmit.
  /// \return Returns a e1ap_bearer_context_setup_response struct with the success member set to
  /// 'true' in case of a successful outcome, 'false' otherwise.
  virtual async_task<e1ap_bearer_context_setup_response>
  handle_bearer_context_setup_request(const e1ap_bearer_context_setup_request& request) = 0;

  /// \brief Initiates the Bearer Context Modification procedure as per TS 38.463 section 8.3.2.
  /// \param[in] request The Bearer Context Modification Request message to transmit.
  /// \return Returns a e1ap_bearer_context_modification_response struct with the success member set to
  /// 'true' in case of a successful outcome, 'false' otherwise.
  virtual async_task<e1ap_bearer_context_modification_response>
  handle_bearer_context_modification_request(const e1ap_bearer_context_modification_request& request) = 0;

  /// \brief Initiates the Bearer Context Release procedure as per TS 38.463 section 8.3.4.
  /// \param[in] msg The Bearer Context Release Command message to transmit.
  virtual async_task<void> handle_bearer_context_release_command(const e1ap_bearer_context_release_command& msg) = 0;
};

struct bearer_creation_complete_message {
  ue_index_t ue_index;
};

/// Scheduler of E1AP async tasks using common signalling.
class e1ap_common_cu_up_task_notifier
{
public:
  virtual ~e1ap_common_cu_up_task_notifier() = default;

  /// Schedule common E1AP task.
  virtual bool schedule_async_task(async_task<void> task) = 0;
};

/// Methods used by E1AP to notify the CU-UP processor.
class e1ap_cu_up_processor_notifier : public e1ap_common_cu_up_task_notifier
{
public:
  virtual ~e1ap_cu_up_processor_notifier() = default;

  /// \brief Notifies about the reception of a GNB-CU-UP E1 Setup Request message.
  /// \param[in] msg The received GNB-CU-UP E1 Setup Request message.
  virtual void on_cu_up_e1_setup_request_received(const cu_up_e1_setup_request& msg) = 0;
};

/// Handle bearer context removal
class e1ap_bearer_context_removal_handler
{
public:
  virtual ~e1ap_bearer_context_removal_handler() = default;

  /// \brief Remove the context of an UE.
  /// \param[in] ue_index The index of the UE to remove.
  virtual void remove_bearer_context(ue_index_t ue_index) = 0;
};

/// Methods used by E1AP to notify the NGAP.
class e1ap_ngap_notifier
{
public:
  virtual ~e1ap_ngap_notifier() = default;

  /// \brief Notifies about the reception of a E1 Setup Request message.
  /// \param[in] msg The received E1 Setup Request message.
  virtual void on_e1_setup_request_received(const cu_up_e1_setup_request& msg) = 0;
};

class e1ap_ue_handler
{
public:
  virtual ~e1ap_ue_handler() = default;

  /// \brief Update the context of an UE.
  virtual void update_ue_context(ue_index_t ue_index, ue_index_t old_ue_index) = 0;

  /// Cancel pending tasks for a UE.
  virtual void cancel_ue_tasks(ue_index_t ue_index) = 0;
};

/// \brief Interface to query statistics from the E1AP interface.
class e1ap_statistics_handler
{
public:
  virtual ~e1ap_statistics_handler() = default;

  /// \brief Get the number of UEs registered at the E1AP.
  /// \return The number of UEs.
  virtual size_t get_nof_ues() const = 0;
};

/// Methods used by E1AP to notify the CU-CP.
class e1ap_cu_cp_notifier
{
public:
  virtual ~e1ap_cu_cp_notifier() = default;

  /// \brief Request scheduling a task for a UE.
  /// \param[in] ue_index The index of the UE.
  /// \param[in] task The task to schedule.
  /// \returns True if the task was successfully scheduled, false otherwise.
  virtual bool schedule_async_task(ue_index_t ue_index, async_task<void> task) = 0;

  /// \brief Notifies about the reception of a Bearer Context Release Request message.
  /// \param[in] msg The received Bearer Context Release Request message.
  virtual void on_bearer_context_release_request_received(const cu_cp_bearer_context_release_request& msg) = 0;

  /// \brief Notifies about the reception of a Bearer Context Inactivity Notification message.
  /// \param[in] msg The received Bearer Context Inactivity Notification message.
  virtual void on_bearer_context_inactivity_notification_received(const cu_cp_inactivity_notification& msg) = 0;

  /// \brief Notify the CU-CP to release a UE.
  /// \param[in] request The release request.
  virtual async_task<void> on_ue_release_required(const cu_cp_ue_context_release_request& request) = 0;

  /// \brief Notifies about the reception of an E1 Release Request message.
  /// \param[in] cu_up_index The index of the CU-UP processor.
  virtual void on_e1_release_request_received(cu_up_index_t cu_up_index) = 0;

  /// \brief Indicates that there was some loss of transaction information for some UEs.
  /// \return Asynchronous task that handles the event
  virtual async_task<void> on_transaction_info_loss(const ue_transaction_info_loss_event& ev) = 0;
};

/// Combined entry point for E1AP handling.
class e1ap_cu_cp : public e1ap_message_handler,
                   public e1ap_event_handler,
                   public e1ap_connection_manager,
                   public e1ap_bearer_context_manager,
                   public e1ap_ue_handler,
                   public e1ap_bearer_context_removal_handler,
                   public e1ap_statistics_handler
{
public:
  virtual ~e1ap_cu_cp() = default;

  virtual async_task<void> stop() = 0;

  virtual e1ap_message_handler&                get_e1ap_message_handler()                = 0;
  virtual e1ap_event_handler&                  get_e1ap_event_handler()                  = 0;
  virtual e1ap_connection_manager&             get_e1ap_connection_manager()             = 0;
  virtual e1ap_bearer_context_manager&         get_e1ap_bearer_context_manager()         = 0;
  virtual e1ap_ue_handler&                     get_e1ap_ue_handler()                     = 0;
  virtual e1ap_bearer_context_removal_handler& get_e1ap_bearer_context_removal_handler() = 0;
  virtual e1ap_statistics_handler&             get_e1ap_statistics_handler()             = 0;
};

} // namespace srs_cu_cp
} // namespace srsran
