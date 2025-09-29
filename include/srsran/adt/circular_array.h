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

#include <array>
#include <cstddef>

namespace srsran {

/// \brief Helper class to safely access elements of a std::array.
///
/// Protects from out-of-bounds access by applying modulo of it's length when using the [] operator for element access.
template <typename T, std::size_t N>
class circular_array
{
  std::array<T, N> data;

public:
  using iterator       = T*;
  using const_iterator = const T*;

  T&       operator[](std::size_t pos) { return data[pos % N]; }
  const T& operator[](std::size_t pos) const { return data[pos % N]; }

  T*       begin() { return data.begin(); }
  const T* begin() const { return data.begin(); }

  T*       end() { return data.end(); }
  const T* end() const { return data.end(); }

  size_t size() const { return N; }
};

} // namespace srsran
