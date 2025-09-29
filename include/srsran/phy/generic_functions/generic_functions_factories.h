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

#include "srsran/phy/generic_functions/dft_processor.h"
#include "srsran/phy/generic_functions/dft_processor_ci16.h"

namespace srsran {

/// Discrete Fourier Transform (DFT) processor factory.
class dft_processor_factory
{
public:
  /// Default destructor.
  virtual ~dft_processor_factory() = default;

  /// \brief Creates a DFT processor.
  /// \param[in] config Provides the DFT processor parameters.
  /// \return A unique pointer of a DFT processor if the configuration is valid. Otherwise, \c nullptr.
  virtual std::unique_ptr<dft_processor> create(const dft_processor::configuration& config) = 0;
};

/// \brief Creates a DFT processor factory based on a generic Radix-2 DFT implementation.
std::shared_ptr<dft_processor_factory> create_dft_processor_factory_generic();

/// \brief Creates a DFT processor factory based on FFTW library.
/// \param[in] optimization_flag       FFTW planning optimization mode. Leave empty for default value. Available modes:
///                                      - \c estimate picks a sub-optimal plan quickly, using heuristics.
///                                      - \c measure finds an optimized FFT plan by computing several FFTs and
///                                        measuring their execution time.
///                                      - \c exhaustive is like \c measure but it tries to achieve an even higher
///                                        optimization.
/// \param[in] plan_creation_timeout_s FFTW plan creation maximum duration in seconds. Set to zero or lower for
///                                    infinite.
/// \param[in] avoid_wisdom            Set to true to avoid loading FFTW wisdom from a file.
/// \param[in] wisdom_filename         FFTW wisdom file name. Leave empty for default value. It is ignored if the option
///                                    \c avoid_wisdom is true.
/// \return A valid pointer to a DFT processor factory if FFTW is available. Otherwise, \c nullptr.
/// \note If the plan creation timeout is reached, the selected plan is sub-optimal. However, it will always be more
///       optimal than \c estimate.
std::shared_ptr<dft_processor_factory> create_dft_processor_factory_fftw(const std::string& optimization_flag,
                                                                         double             plan_creation_timeout_s,
                                                                         bool               avoid_wisdom    = false,
                                                                         const std::string& wisdom_filename = "");

/// \brief Creates a sub-optimal DFT processor factory based on FFTW library.
/// \param[in] avoid_wisdom    Set to true to avoid loading FFTW wisdom from a file.
/// \param[in] wisdom_filename FFTW wisdom file name. Leave empty for default value. It is ignored if the option
///                            \c avoid_wisdom is true.
/// \return A valid pointer to a DFT processor factory if FFTW is available. Otherwise, \c nullptr.
inline std::shared_ptr<dft_processor_factory>
create_dft_processor_factory_fftw_slow(bool avoid_wisdom = false, const std::string& wisdom_filename = "")
{
  return create_dft_processor_factory_fftw("estimate", 1.0, avoid_wisdom, wisdom_filename);
}

/// \brief Creates an optimal DFT processor factory based on FFTW library.
/// \param[in] avoid_wisdom    Set to true to avoid loading FFTW wisdom from a file.
/// \param[in] wisdom_filename FFTW wisdom file name. Leave empty for default value. It is ignored if the option
///                            \c avoid_wisdom is true.
/// \return A valid pointer to a DFT processor factory if FFTW is available. Otherwise, \c nullptr.
inline std::shared_ptr<dft_processor_factory>
create_dft_processor_factory_fftw_fast(bool avoid_wisdom = false, const std::string& wisdom_filename = "")
{
  std::string optimization_flag;
#if defined(__SANITIZE_THREAD__) || defined(__SANITIZE_MEMORY__)
  return create_dft_processor_factory_fftw_slow(avoid_wisdom, wisdom_filename);
#else
  return create_dft_processor_factory_fftw("measure", 5.0, avoid_wisdom, wisdom_filename);
#endif
}

/// Factory for the Discrete Fourier Transform (DFT) processor for 16-bit complex integer values.
class dft_processor_ci16_factory
{
public:
  /// Default destructor.
  virtual ~dft_processor_ci16_factory() = default;

  /// \brief Creates a DFT processor.
  /// \param[in] config Configuration parameters.
  /// \return A unique pointer to a DFT processor if the configuration is valid. Otherwise, \c nullptr.
  virtual std::unique_ptr<dft_processor_ci16> create(const dft_processor_ci16::configuration& config) = 0;
};

/// \brief Creates a factory for DFT processors for 16-bit complex integer values.
///
/// \return A DFT processor factory.
std::shared_ptr<dft_processor_ci16_factory> create_dft_processor_ci16_factory_avx2();

} // namespace srsran
