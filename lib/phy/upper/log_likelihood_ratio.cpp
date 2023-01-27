/*
 *
 * Copyright 2013-2022 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "srsgnb/phy/upper/log_likelihood_ratio.h"
#include "srsgnb/adt/optional.h"
#include <cmath>

#ifdef __AVX2__
#include <immintrin.h>
#endif // __AVX2__
#ifdef HAVE_NEON
#include <arm_neon.h>
#endif // HAVE_NEON

using namespace srsgnb;

// Computes the sum when at least one of the summands is plus/minus infinity.
// Note that also the indeterminate case +LLR_INFTY + (-LLR_INFTY) is set to zero.
static optional<log_likelihood_ratio> tackle_special_sums(log_likelihood_ratio val_a, log_likelihood_ratio val_b)
{
  if (val_a == -val_b) {
    return log_likelihood_ratio(0);
  }

  // When at least one of the summands is infinity, the sum is also infinity (with sign).
  // The indeterminate case LLR_INFTY + (-LLR_INFTY) has already been dealt with in the previous case.
  if (log_likelihood_ratio::isinf(val_a)) {
    return val_a;
  }
  if (log_likelihood_ratio::isinf(val_b)) {
    return val_b;
  }
  return {};
}

log_likelihood_ratio log_likelihood_ratio::operator+=(log_likelihood_ratio rhs)
{
  if (auto special = tackle_special_sums(*this, rhs)) {
    *this = special.value();
    return *this;
  }

  // When not dealing with special cases, classic saturated sum.
  int tmp = value + static_cast<int>(rhs.value);
  if (std::abs(tmp) > LLR_MAX) {
    *this = ((tmp > 0) ? LLR_MAX : -LLR_MAX);
    return *this;
  }
  *this = tmp;
  return *this;
}

log_likelihood_ratio log_likelihood_ratio::promotion_sum(log_likelihood_ratio a, log_likelihood_ratio b)
{
  if (auto special = tackle_special_sums(a, b)) {
    return special.value();
  }

  // When not dealing with special cases, promotion sum: if the sum exceeds LLR_MAX (in absolute value), then return
  // LLR_INFTY (with the proper sign).
  int tmp = a.value + static_cast<int>(b.value);
  if (std::abs(tmp) > log_likelihood_ratio::LLR_MAX) {
    return ((tmp > 0) ? log_likelihood_ratio::LLR_INFTY : -log_likelihood_ratio::LLR_INFTY);
  }
  return tmp;
}

log_likelihood_ratio log_likelihood_ratio::quantize(float value, float range_limit)
{
  srsgnb_assert(range_limit > 0, "Second input must be positive.");

  float clipped = value;
  if (std::abs(value) > range_limit) {
    clipped = std::copysign(range_limit, value);
  }
  return static_cast<value_type>(std::round(clipped / range_limit * LLR_MAX));
}

#ifdef __AVX2__
// Hard decision function with bit packing.
static void hard_decision_simd(bit_buffer& hard_bits, const int8_t* soft_bits, unsigned len)
{
  // Number of bits processed on each loop cycle.
  static constexpr unsigned AVX2_B_SIZE = 32;

  unsigned i_bit = 0;

  // Destination buffer.
  span<uint8_t> packed_buffer = hard_bits.get_buffer();

  for (; i_bit != (len / AVX2_B_SIZE) * AVX2_B_SIZE; i_bit += AVX2_B_SIZE) {
    // Generate a mask from the LLR.
    __m256i mask = _mm256_loadu_si256((__m256i*)(&soft_bits[i_bit]));

    // Shuffle mask. The position of each byte is maintained, but its bits are reversed.
    mask = _mm256_shuffle_epi8(mask,
                               _mm256_set_epi8(24,
                                               25,
                                               26,
                                               27,
                                               28,
                                               29,
                                               30,
                                               31,
                                               16,
                                               17,
                                               18,
                                               19,
                                               20,
                                               21,
                                               22,
                                               23,
                                               8,
                                               9,
                                               10,
                                               11,
                                               12,
                                               13,
                                               14,
                                               15,
                                               0,
                                               1,
                                               2,
                                               3,
                                               4,
                                               5,
                                               6,
                                               7));

    // Obtain 32 packed hard bits from the mask.
    uint32_t bytes = _mm256_movemask_epi8(mask);

    // Write the packed bits into 4 bytes of the internal buffer.
    *(reinterpret_cast<uint32_t*>(packed_buffer.begin())) = bytes;

    // Advance buffer.
    packed_buffer = packed_buffer.last(packed_buffer.size() - (AVX2_B_SIZE / 8));
  }

  for (; i_bit != len; ++i_bit) {
    uint8_t hard_bit = soft_bits[i_bit] < 0 ? 1 : 0;

    hard_bits.insert(hard_bit, i_bit, 1);
  }
}
#endif // __AVX2__

#ifdef HAVE_NEON
static void hard_decision_simd(bit_buffer& hard_bits, const int8_t* soft_bits, unsigned len)
{
  const uint8x16_t mask_msb_u8    = vdupq_n_u8(0x80);
  const int8_t     shift_mask[16] = {-7, -6, -5, -4, -3, -2, -1, 0, -7, -6, -5, -4, -3, -2, -1, 0};
  const int8x16_t  shift_mask_s8 =
      vcombine_s8(vcreate_s8(*((uint64_t*)shift_mask)), vcreate_s8(*((uint64_t*)&shift_mask[8])));

  // Number of bits processed on each loop cycle.
  static constexpr unsigned NEON_B_SIZE = 16;

  unsigned i_bit = 0;

  // Destination buffer.
  span<uint8_t> packed_buffer = hard_bits.get_buffer();

  for (; i_bit != (len / NEON_B_SIZE) * NEON_B_SIZE; i_bit += NEON_B_SIZE) {
    // Read soft bits.
    uint8x16_t soft_bits_u8 = vld1q_u8(reinterpret_cast<const uint8_t*>(&soft_bits[i_bit]));

    // Reverse 8 bytes in every double-word, MSBs of each byte will form a mask.
    soft_bits_u8 = vrev64q_u8(soft_bits_u8);

    // Generate masks of MSB bits shifted to appropriate position.
    uint8x16_t msb_bits_u8 = vandq_u8(soft_bits_u8, mask_msb_u8);
    uint8x16_t mask_u8     = vshlq_u8(msb_bits_u8, shift_mask_s8);

    // Obtain 16 packed hard bits from the mask.
    mask_u8 = vpaddq_u8(mask_u8, mask_u8);
    mask_u8 = vpaddq_u8(mask_u8, mask_u8);
    mask_u8 = vpaddq_u8(mask_u8, mask_u8);

    // Write the packed bits into 2 bytes of the internal buffer.
    vst1q_lane_u16(reinterpret_cast<uint16_t*>(packed_buffer.begin()), vreinterpretq_u16_u8(mask_u8), 0);

    // Advance buffer.
    packed_buffer = packed_buffer.last(packed_buffer.size() - (NEON_B_SIZE / 8));
  }

  for (; i_bit != len; ++i_bit) {
    uint8_t hard_bit = soft_bits[i_bit] < 0 ? 1 : 0;

    hard_bits.insert(hard_bit, i_bit, 1);
  }
}
#endif // HAVE_NEON

void srsgnb::hard_decision(bit_buffer& hard_bits, span<const log_likelihood_ratio> soft_bits)
{
  // Make sure that there is enough space in the output to accommodate the hard bits.
  srsgnb_assert(soft_bits.size() <= hard_bits.size(),
                "Input size (i.e., {}) does not fit into the output buffer with size {}",
                soft_bits.size(),
                hard_bits.size());

  unsigned nof_bits = soft_bits.size();

#if defined(__AVX2__) || defined(HAVE_NEON)

  hard_decision_simd(hard_bits, reinterpret_cast<const int8_t*>(soft_bits.data()), nof_bits);

#else
  for (unsigned i_bit = 0; i_bit != nof_bits; ++i_bit) {
    // Compute hard bit.
    uint8_t hard_bit = soft_bits[i_bit] >= 0 ? 0 : 1;

    // Insert into the bit buffer.
    hard_bits.insert(hard_bit, i_bit, 1);
  }
#endif // __AVX2__ or HAVE_NEON
}
