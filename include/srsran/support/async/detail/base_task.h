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

#include "srsran/support/async/detail/promise_data.h"
#include "srsran/support/async/detail/unique_coroutine.h"

namespace srsran {
namespace detail {

struct task_promise_base {
  /// True if the result of task has been stored.
  bool has_value = false;
  /// Pending continuation task.
  coro_handle<> continuation;

  /// Check if return has been stored.
  bool ready() const { return has_value; }
};

/// Common functionality to lazy and eager async tasks.
template <typename Derived, typename Result>
class common_task_crtp
{
public:
  using result_type = Result;

  /// Checks if async task has an handle.
  bool empty() const { return derived().handle.empty(); }

  /// Returns true if base_task is complete.
  bool ready() const { return empty() or derived().handle.promise().ready(); }

  /// Called to get result of task once it is complete.
  template <typename Res = std::decay_t<result_type>, std::enable_if_t<not std::is_same_v<Res, void>, bool> = true>
  const Res& get() const&
  {
    srsran_assert(not empty() and derived().handle.promise().ready(), "Called task::get() for task that is not ready");
    return derived().handle.promise().get();
  }

  /// Called to get result of task once it is complete.
  template <typename Res = std::decay_t<result_type>, std::enable_if_t<not std::is_same_v<Res, void>, bool> = true>
  Res get() &&
  {
    srsran_assert(not empty() and derived().handle.promise().ready(), "Called task::get() for task that is not ready");
    return std::move(derived().handle.promise()).get();
  }

  /// Awaiter interface.
  bool await_ready() const noexcept { return derived().handle.empty() or derived().handle.promise().ready(); }
  template <typename Res = result_type>
  detail::enable_if_nonvoid<Res> await_resume()
  {
    srsran_sanity_check(not empty(), "Resuming an empty base_task");
    return derived().handle.promise().get();
  }
  template <typename Res = result_type>
  detail::enable_if_void<Res> await_resume()
  {
    srsran_sanity_check(not empty(), "Resuming an empty base_task");
  }

private:
  Derived&       derived() { return static_cast<Derived&>(*this); }
  const Derived& derived() const { return static_cast<const Derived&>(*this); }
};

} // namespace detail
} // namespace srsran
