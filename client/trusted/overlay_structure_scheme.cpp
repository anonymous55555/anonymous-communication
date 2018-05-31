//
// Created by c1 on 04.05.18.
//

#include <sgx_trts.h>
#include <cmath>
#include "overlay_structure_scheme.h"
#include "../../include/shared_functions.h"

namespace c1::client {
void OverlayStructureScheme::init(uint64_t onid_assoc,
                                  uint64_t onid_emul,
                                  const std::vector<PeerInformation> &gamma_send,
                                  const std::vector<PeerInformation> &gamma_receive,
                                  const std::map<uint64_t, std::vector<PeerInformation>> &gamma_route,
                                  uint64_t overlay_dimension,
                                  uint64_t reconfiguration_time,
                                  PeerInformation own_id) {
  onid_assoc_ = onid_assoc;
  onid_emul_ = onid_emul;
  onid_emul_prev_ = onid_emul;
  gamma_agree_ = gamma_route.at(onid_emul);
  gamma_send_ = gamma_send;
  gamma_receive_ = gamma_receive;
  gamma_route_ = gamma_route;
  overlay_dimension_ = overlay_dimension;
  reconfiguration_time_ = reconfiguration_time;
  own_id_ = own_id;
}

OverlayReturnTuple OverlayStructureScheme::update(round_t round,
                                                  const std::vector<OverlayStructureSchemeMessage> &set_s) {

  // the following is the simple code used so far...
//  auto gamma_route_result = gamma_route_;
//  if (gamma_route_result.count(onid_emul_prev_) == 0) { //
//    gamma_route_result[onid_emul_prev_] = nodes_emulating_previous_q_e_;
//  }
//  std::map<PeerInformation, std::vector<OverlayStructureSchemeMessage>> s_prime;
//  return OverlayReturnTuple{onid_emul_, gamma_agree_, gamma_send_, gamma_route_result, gamma_receive_, s_prime};

  // (1)
  std::map<PeerInformation, std::vector<OverlayStructureSchemeMessage>> s_prime;

  // (2)
  std::map<onid_t, std::vector<OverlayStructureSchemeMessage>> msg_out_q;

  // (3)
  if (round % (reconfiguration_time_ / 2) == 1) {
    // (a)
    onid_emul_prev_ = onid_emul_;

    // (b)
    onid_emul_ = onid_emul_new_;
    gamma_agree_.clear();
    for (auto &i_prime : gamma_route_new_[onid_emul_]) {
      gamma_agree_.push_back(i_prime);
    }
    gamma_send_ = std::move(gamma_send_new_);
    gamma_route_ = std::move(gamma_route_new_);

    // (c)
    uint64_t random_number = 12345;
#ifndef OUTSIDE_ENCLAVE
    sgx_read_rand((unsigned char *) &random_number, 8);
#endif
    auto num_quorums = static_cast<uint64_t>(std::pow(2, overlay_dimension_));
    uint64_t random_quorum = random_number % num_quorums;
    onid_emul_new_ = random_quorum;

    // (d)
    gamma_send_new_.clear();
    gamma_route_new_.clear();

    // (e)
    for (auto &i_prime : gamma_route_succ_[onid_emul_prev_]) {
      gamma_agree_prev_[onid_emul_prev_].push_back(i_prime);
    }

    // (f)
    gamma_route_prev_.clear();
    for (auto &i_prime : gamma_route_succ_[onid_emul_prev_]) {
      gamma_route_prev_[onid_emul_prev_].push_back(i_prime);
    }

    // (g)
    gamma_route_succ_.clear();

    // (h)
    msg_out_q[onid_emul_new_].push_back(OverlayStructureSchemeMessage::createEmulateRequestMsg(onid_emul_new_,
                                                                                               own_id_));
  }

  // (4)
  if (round % (reconfiguration_time_ / 2) == 2) {
    gamma_receive_ == std::move(gamma_receive_new_);
    gamma_receive_new_.clear();
    for (auto &i_prime : gamma_route_[onid_emul_]) {
      gamma_route_prev_[onid_emul_].push_back(i_prime);
    }
  }

  // (5)
  if (round % (reconfiguration_time_ / 2) == calculate_agreement_time(overlay_dimension_) + 1) {
    gamma_agree_prev_.clear();
    for (auto &i_prime : gamma_route_[onid_emul_]) {
      gamma_agree_prev_[onid_emul_].push_back(i_prime);
    }
  }

  // (6)
  for (const auto &s : set_s) {
    // (a)
    if (s.t == OverlayStructureSchemeMessage::OverlayStructureSchemeMessageType::tEmulateRequestMsg) {
      // (i)
      if (s.onid != onid_emul_) {
        msg_out_q[s.onid].push_back(s);
      }

      // (ii)
      for_all_neighbors(onid_emul_, overlay_dimension_, [&msg_out_q,
          &s](onid_t
              neighbor) {
        msg_out_q[neighbor].emplace_back(OverlayStructureSchemeMessage::createEmulateRequestReceivedMsg(s.onid,
                                                                                                        s.peer_information));
      });
    }

    // (b)
    if (s.t == OverlayStructureSchemeMessage::OverlayStructureSchemeMessageType::tEmulateRequestReceivedMsg) {
      gamma_route_succ_[s.onid].push_back(s.peer_information);
    }

    // (c)
    if (s.t == OverlayStructureSchemeMessage::OverlayStructureSchemeMessageType::tHandOverMsg) {
      for (auto&[onid, ids] : s.gamma_route) {
        for (auto &elem : ids) {
          if (std::find(gamma_route_new_[onid].begin(), gamma_route_new_[onid].end(), elem)
              == gamma_route_new_[onid].end()) { // do not add elements twice
            gamma_route_new_[onid].push_back(elem);
          }
        }
      }
      for (auto &elem : s.gamma_receive) {
        if (std::find(gamma_receive_new_.begin(), gamma_receive_new_.end(), elem)
            == gamma_receive_new_.end()) { // do not add elements twice
          gamma_receive_new_.push_back(elem);
        }
      }
    }

    // (d)
    if (s.t == OverlayStructureSchemeMessage::OverlayStructureSchemeMessageType::tSelfIntroduceMsg) {
      gamma_send_new_.push_back(s.peer_information);
    }
  }

  // (7)
  if (round % (reconfiguration_time_ / 2) == overlay_dimension_ + 2) {
    for (auto &i_prime: gamma_route_succ_[onid_emul_]) {
      s_prime[i_prime].push_back(OverlayStructureSchemeMessage::createHandOverMessage(gamma_route_succ_,
                                                                                      gamma_receive_));
    }
  }

  // (8)
  if (round % (reconfiguration_time_ / 2) == overlay_dimension_ + 3) {
    for (auto &i_prime: gamma_receive_new_) {
      s_prime[i_prime].push_back(OverlayStructureSchemeMessage::createSelfIntroduceMessage(own_id_));
    }
  }

  // (9)
  for (auto&[onid, msgs]: msg_out_q) {
    // determine onid'
    onid_t onid_prime = onid_emul_;
    for (int i = 0; i < overlay_dimension_; ++i) {
      unsigned long i_th_bit_of_own_onid = (onid_emul_ >> i) & 1U;
      unsigned long i_th_bit_of_onid = (onid >> i) & 1U;
      if (i_th_bit_of_onid != i_th_bit_of_own_onid) {
        onid_prime ^=
            (-i_th_bit_of_onid ^ onid_prime) & (1UL << i); // changes the i-th bit of onid_prime to i_th_bit_of_onid
        break;
      }
    }
    for (const auto &msg: msgs) {
      for (const auto &i_prime : gamma_route_[onid_prime]) {
        s_prime[i_prime].push_back(msg);
      }
    }
  }

  // (10)
  auto gamma_route_result = gamma_route_;
  for (const auto&[onid, ids] : gamma_agree_prev_) {
    for (auto &id: ids) {
      if (std::find(gamma_route_result[onid].begin(), gamma_route_result[onid].end(), id)
          == gamma_route_result[onid].end()) { // I know this is inefficient ...
        gamma_route_result[onid].push_back(id);
      }
    }
  }
  for (const auto&[onid, ids] : gamma_route_prev_) {
    for (auto &id: ids) {
      if (std::find(gamma_route_result[onid].begin(), gamma_route_result[onid].end(), id)
          == gamma_route_result[onid].end()) { // I know this is inefficient ...
        gamma_route_result[onid].push_back(id);
      }
    }
  }

  return OverlayReturnTuple{onid_emul_, gamma_agree_, gamma_send_, gamma_route_result, gamma_receive_, s_prime};
}

void OverlayStructureSchemeMessage::serialize(std::vector<uint8_t> &working_vec) const {
  serialize_number(working_vec, static_cast<uint8_t>(t));
  serialize_number(working_vec, onid);
  peer_information.serialize(working_vec);
  serialize_number(working_vec, gamma_route.size());
  for (auto&[onid, peer_information_vec] : gamma_route) {
    serialize_number(working_vec, onid);
    serialize_vec(working_vec, peer_information_vec);
  }
  serialize_vec(working_vec, gamma_receive);
}

OverlayStructureSchemeMessage OverlayStructureSchemeMessage::deserialize(const std::vector<uint8_t> &working_vec,
                                                                         size_t &cur) {
  OverlayStructureSchemeMessage result;
  result.t = static_cast<OverlayStructureSchemeMessageType>(deserialize_number<uint8_t>(working_vec, cur));
  result.onid = deserialize_number<decltype(result.onid)>(working_vec, cur);
  result.peer_information = PeerInformation::deserialize(working_vec, cur);
  auto gamma_route_size = deserialize_number<size_t>(working_vec, cur);
  for (int i = 0; i < gamma_route_size; ++i) {
    auto onid = deserialize_number<onid_t>(working_vec, cur);
    result.gamma_route[onid] = deserialize_vec<PeerInformation>(working_vec, cur);
  }
  result.gamma_receive = deserialize_vec<PeerInformation>(working_vec, cur);

  return result;
}
OverlayStructureSchemeMessage::OverlayStructureSchemeMessage(OverlayStructureSchemeMessage::OverlayStructureSchemeMessageType t,
                                                             onid_t onid,
                                                             const PeerInformation &peer_information,
                                                             const std::map<onid_t,
                                                                            std::vector<PeerInformation>> &gamma_route,
                                                             const std::vector<PeerInformation> &gamma_receive_or_send)
    : t(t),
      onid(onid),
      peer_information(peer_information),
      gamma_route(gamma_route),
      gamma_receive(gamma_receive_or_send) {}

} // !namespace