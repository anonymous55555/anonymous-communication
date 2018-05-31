//
// Created by c1 on 06.05.18.
//

#include "routing_scheme.h"
#include "../../include/cryptlib.h"

namespace c1::client {

std::vector<RoutingSchemeTuple> RoutingScheme::route(std::vector<RoutingSchemeTuple> &set_s,
                                                     round_t cur_round,
                                                     size_t overlay_dimension,
                                                     const sgx_cmac_128bit_key_t &sk_routing) {
  auto cancel_set = determine_elements_to_be_cancelled(set_s);
  std::vector<RoutingSchemeTuple> result;

  for (auto &s : set_s) {
    if (cancel_set.count(RoutingSchemeTuple{MessageTuple::create_cancel(), s.onid_dst,
                                            s.bucket_dst, s.l_dst, s.onid_current})) {
      continue;
    }
    auto i = 1 + 2 * overlay_dimension - (s.l_dst - cur_round);
    auto onid_itm = cryptlib::get_intermediate_target(sk_routing, s.bucket_dst, s.l_dst, overlay_dimension);
    assert (i >= 1);
    assert (i <= 2 * overlay_dimension);
    if (i <= overlay_dimension) {
      unsigned long itm_bit = (onid_itm >> i) & 1U; // determines the i-th bit of onid_itm
      s.onid_current ^= (-itm_bit ^ s.onid_current) & (1UL << i); // changes the i-th bit of s.onid_current to itm_bit
    } else if (i <= 2 * overlay_dimension) {
      unsigned long itm_bit = (s.onid_dst >> (i - overlay_dimension)) & 1U; // determines the i-th bit of s.onid_dst
      s.onid_current ^= (-itm_bit ^ s.onid_current) & (1UL << i); // changes the i-th bit of s.onid_current to itm_bit
    } else if (i == 2 * overlay_dimension + 1) {
      assert(false); // actually, if this happens, something bad happened..
    }
    result.push_back(s);
  }

  return result;
}

std::set<RoutingSchemeTuple> RoutingScheme::determine_elements_to_be_cancelled(std::vector<RoutingSchemeTuple> &set_s) {
  std::set<RoutingSchemeTuple> result;

  if (set_s.size() == 0) {
    return result;
  }

  std::sort(set_s.begin(), set_s.end());
  {
    auto &prev_element = set_s.at(0);
    auto prev_element_index = 0;
    for (int i = 0; i < set_s.size(); ++i) {
      if (set_s[i].m.is_cancel()) {
        result.emplace(RoutingSchemeTuple{MessageTuple::create_cancel(), prev_element.onid_dst, prev_element.bucket_dst,
                                          prev_element.l_dst, prev_element.onid_current});
      }

      if (std::tie(prev_element.onid_dst, prev_element.bucket_dst, prev_element.l_dst, prev_element.onid_current) !=
          std::tie(set_s[i].onid_dst, set_s[i].bucket_dst, set_s[i].l_dst, set_s[i].onid_current)) {
        if (i - prev_element_index > kRecv) {
          result.emplace(RoutingSchemeTuple{MessageTuple::create_cancel(), prev_element.onid_dst,
                                            prev_element.bucket_dst, prev_element.l_dst, prev_element.onid_current});
        }
        prev_element = set_s.at(i);
        prev_element_index = i;
      }
    }
  }

  return result;
}

} // !namespace