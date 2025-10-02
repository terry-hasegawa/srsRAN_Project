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

#include "srsran/ran/du_types.h"
#include "srsran/ran/plmn_identity.h"
#include "srsran/ran/resource_block.h"
#include "srsran/ran/s_nssai.h"

namespace srsran {

/// Identifier of a RRM Policy Member.
struct rrm_policy_member {
  plmn_identity plmn_id = plmn_identity::test_value();
  /// Single Network Slice Selection Assistance Information (S-NSSAI).
  s_nssai_t s_nssai;

  bool operator==(const rrm_policy_member& other) const
  {
    return plmn_id == other.plmn_id and s_nssai == other.s_nssai;
  }
};

struct rrm_policy_ratio_group {
  /// The resource type of interest for an RRM Policy.
  /// \remark See 3GPP TS 28.541, Section 4.4.1 Attribute properties.
  enum class resource_type_t { prb, prb_ul, prb_dl };
  resource_type_t resource_type = resource_type_t::prb;
  /// List of RRM policy members (PLMN + S-NSSAI combinations).
  std::vector<rrm_policy_member> policy_members_list;
  /// Sets the minimum percentage of PRBs to be allocated to this group.
  std::optional<unsigned> min_prb_policy_ratio;
  /// Sets the maximum percentage of PRBs to be allocated to this group.
  std::optional<unsigned> max_prb_policy_ratio;
  /// Sets the percentage of PRBs to be allocated to this group.
  std::optional<unsigned> ded_prb_policy_ratio;
};

/// Number of dedicated, prioritized and shared PRBs associated with a given RAN slice.
struct rrm_policy_radio_block_limits {
  rrm_policy_radio_block_limits() = default;
  rrm_policy_radio_block_limits(unsigned prio_rbs_, unsigned shared_rbs_) :
    rrm_policy_radio_block_limits(0, prio_rbs_, shared_rbs_)
  {
  }
  rrm_policy_radio_block_limits(unsigned ded_rbs_, unsigned prio_rbs_, unsigned shared_rbs_) :
    ded_rbs(ded_rbs_), min_rbs(ded_rbs_ + prio_rbs_), max_rbs(shared_rbs_ + min_rbs)
  {
  }

  /// Sum of dedicated and prioritized RBs.
  unsigned min() const { return min_rbs; }

  /// Sum of dedicated, prioritized and shared RBs.
  unsigned max() const { return max_rbs; }

  /// Dedicated PRBs.
  unsigned dedicated() const { return ded_rbs; }

  /// Prioritized PRBs.
  unsigned prioritized() const { return min_rbs - ded_rbs; }

  /// Shared PRBs.
  unsigned shared() const { return max_rbs - min_rbs; }

private:
  unsigned ded_rbs = 0;
  unsigned min_rbs = 0;
  unsigned max_rbs = MAX_NOF_PRBS;
};

constexpr unsigned MAX_SLICE_RECONF_POLICIES = 16;

/// Request to reconfigure slicing policies for a given DU cell.
struct du_cell_slice_reconfig_request {
  du_cell_index_t cell_index;
  struct rrm_policy_config {
    rrm_policy_member rrc_member;
    /// RB limits for the RRM policy member.
    rrm_policy_radio_block_limits rbs;
  };
  static_vector<rrm_policy_config, MAX_SLICE_RECONF_POLICIES> rrm_policies;
};

} // namespace srsran

namespace fmt {
template <>
struct formatter<srsran::rrm_policy_member> {
  template <typename ParseContext>
  auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const srsran::rrm_policy_member& member, FormatContext& ctx) const
  {
    return format_to(ctx.out(),
                     "{{PLMN={}, SST={}, SD={}}}",
                     member.plmn_id.to_string(),
                     member.s_nssai.sst.value(),
                     member.s_nssai.sd.value());
  }
};

} // namespace fmt
