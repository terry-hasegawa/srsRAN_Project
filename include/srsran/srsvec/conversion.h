/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

/// \file
/// \brief Conversion functions.

#pragma once

#include "srsran/adt/complex.h"
#include "srsran/srsvec/types.h"

namespace srsran {
namespace srsvec {

/// \brief Converts a sequence of numbers from complex float to int16 applying the given scaling and rounding the result
/// to the nearest integer.
///
/// \param [out] z       Converted sequence.
/// \param [in]  x       Input sequence.
/// \param [in]  scale   Scaling factor.
void convert(span<int16_t> z, span<const cf_t> x, float scale);

/// \brief Converts from int16 to complex float applying the given scaling.
///
/// \param [out] z       Converted sequence.
/// \param [in]  x       Input sequence.
/// \param [in]  scale   Scaling factor.
void convert(span<cf_t> z, span<const int16_t> x, float scale);

/// \brief Converts a sequence of numbers from float to int16 applying the given scaling and rounding the result to the
/// nearest integer.
///
/// \param [out] z       Converted sequence.
/// \param [in]  x       Input sequence.
/// \param [in]  scale   Scaling factor.
void convert(span<int16_t> z, span<const float> x, float scale);

/// Converts from int16 to float applying the given scaling
///
/// \param [out] z       Converted sequence.
/// \param [in]  x       Input sequence.
/// \param [in]  scale   Scaling factor.
void convert(span<float> z, span<const int16_t> x, float scale);

/// \brief Converts a sequence of numbers from complex float to complex brain float.
///
/// \param[out] out Resultant data.
/// \param[in]  in  Data to convert.
void convert(span<cbf16_t> out, span<const cf_t> in);

/// \brief Converts a sequence of numbers from single precision float to brain float.
///
/// \param[out] out Resultant data.
/// \param[in]  in  Data to convert.
void convert(span<bf16_t> out, span<const float> in);

/// \brief Converts a sequence of numbers from complex brain float to complex float.
///
/// \param[out] out Resultant data.
/// \param[in]  in  Data to convert.
void convert(span<cf_t> out, span<const cbf16_t> in);

/// \brief Converts a sequence of numbers from brain float to single precision float.
///
/// \param[out] out Resultant data.
/// \param[in]  in  Data to convert.
void convert(span<float> out, span<const bf16_t> in);

/// \brief Converts a sequence of numbers from complex brain float to int16 applying the given scaling and rounding the
/// result to the nearest integer.
///
/// \param [out] z     Resultant data.
/// \param [in]  x     Data to convert.
/// \param [in]  scale Input data scaling prior conversion.
void convert(span<int16_t> z, span<const cbf16_t> x, float scale);

/// \brief Converts a sequence of numbers from complex brain float to 16-bit complex integer applying the given scaling
/// and rounding the result to the nearest integer.
///
/// \param [out] z     Resultant data.
/// \param [in]  x     Data to convert.
/// \param [in]  scale Input data scaling prior conversion.
void convert(span<ci16_t> z, span<const cbf16_t> x, float scale);

/// \brief Converts from int16 to complex brain float applying the given scaling.
///
/// \param [out] z     Resultant data.
/// \param [in]  x     Data to convert.
/// \param [in]  scale Input data scaling after conversion.
void convert(span<cbf16_t> z, span<const int16_t> x, float scale);

/// \brief Converts from int16 to complex brain float applying the given scaling per input element.
///
/// \param [out] z     Resultant data.
/// \param [in]  x     Data to convert.
/// \param [in]  scale Vector of scaling factors to be applied after conversion.
void convert(span<cbf16_t> z, span<const int16_t> x, span<const float> scale);

/// \brief Converts a sequence of numbers from brain float to int16 applying the given scaling and rounding the result
/// to the nearest integer.
///
/// \param [out] z     Resultant data.
/// \param [in]  x     Data to convert.
/// \param [in]  scale Input data scaling prior conversion.
void convert(span<int16_t> z, span<const bf16_t> x, float scale);

/// Converts from int16 to brain float applying the given scaling.
///
/// \param [out] z     Resultant data.
/// \param [in]  x     Data to convert.
/// \param [in]  scale Input data scaling after conversion.
void convert(span<bf16_t> z, span<const int16_t> x, float scale);

} // namespace srsvec
} // namespace srsran
