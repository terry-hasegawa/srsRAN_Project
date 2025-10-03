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

#include "srsran/support/async/coroutine.h"
#include "srsran/support/async/detail/event_impl.h"
#include <utility>

namespace srsran {
namespace detail {

/// \brief Base class implementation with functionality that is common to event_signal<R> and event_signal_flag.
///
/// Awaiters for the event are stored in a linked list of awaiters, where each awaiter is stored in the embedded memory
/// buffer of the coroutine frame.
class signal_event_common
{
public:
  struct awaiter_common {
    explicit awaiter_common(signal_event_common& event_) : event(&event_) {}

    bool await_ready() const { return false; }

    void await_suspend(coro_handle<> c)
    {
      // Store suspending coroutine.
      suspended_handle = c;

      // Enqueue awaiter in linked list.
      next = static_cast<awaiter_common*>(std::exchange(event->awaiters, static_cast<void*>(this)));
    }

    signal_event_common* event;
    awaiter_common*      next;
    coro_handle<>        suspended_handle;
  };

  signal_event_common() = default;

  signal_event_common(const signal_event_common& event_)            = delete;
  signal_event_common& operator=(const signal_event_common& event_) = delete;

  ~signal_event_common() = default;

protected:
  /// Linked list of awaiters.
  void* awaiters = nullptr;

  /// Triggers all awaiting coroutines after event is set.
  void flush()
  {
    auto* old_state = std::exchange(awaiters, nullptr);
    flush_awaiter_list(static_cast<awaiter_common*>(old_state));
  }
};

} // namespace detail

/// \brief Awaitable type that can be manually set to one of the states: "set" or "unset".
///
/// Coroutines will automatically resume when they await on a manual_event_flag that is in "set" state.
class event_signal_flag : public detail::signal_event_common
{
public:
  using result_type = void;

  struct awaiter_type : public awaiter_common {
    using awaiter_common::awaiter_common;
    void await_resume() {}
  };

  void set() { flush(); }

  /// Awaiter interface.
  awaiter_type get_awaiter() { return awaiter_type{*this}; }
};

/// Similar to "manual_event_flag", but the data passed to .set() is propagated to awaiting coroutine.
/// \tparam Data type of data passed to awaiting coroutine.
template <typename Data>
class event_signal : public detail::signal_event_common
{
public:
  /// Stored value type.
  using result_type = Data;

  /// Awaiter object type.
  struct awaiter_type : public awaiter_common {
    using awaiter_common::awaiter_common;
    Data await_resume() const { return static_cast<event_signal<Data>*>(event)->data; }
  };

  /// Called to set event value, and trigger all awaiting coroutines.
  template <typename U>
  void set(U&& u)
  {
    data = std::forward<U>(u);
    flush();
  }

  /// Awaiter interface.
  awaiter_type get_awaiter() { return awaiter_type{*this}; }

private:
  Data data;
};

} // namespace srsran
