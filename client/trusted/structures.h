//
// Created by c1 on 05.05.18.
//

#ifndef NETWORK_SGX_EXAMPLE_STRUCTURES_H
#define NETWORK_SGX_EXAMPLE_STRUCTURES_H

#include <cstdint>
#include <array>
#include <vector>
#include <ostream>
#include "../../include/serialization.h"
#include "../../include/config.h"
#include "helpers.h"
#include "distributed_agreement_scheme.h"

namespace c1::client {

/**
 * Represents a pseudonym (which is actually uint8_t-array of length kPseudonymSize).
 */
class Pseudonym : public Serializable {
  std::array<uint8_t, kPseudonymSize> pseud_;

 private:
  Pseudonym() : pseud_() {};

 public:
  Pseudonym(uint8_t *pseud_array) : pseud_() {
    std::copy(pseud_array, pseud_array + kPseudonymSize, pseud_.begin());
  }

  const std::array<uint8_t, kPseudonymSize> &get() const {
    return pseud_;
  }

  bool operator==(const Pseudonym &rhs) const {
    for (int i = 0; i < pseud_.size(); ++i) {
      if (pseud_[i] != rhs.pseud_[i]) {
        return false;
      }
    }
    return true;
  }

  bool operator!=(const Pseudonym &rhs) const {
    return !(rhs == *this);
  }

  bool operator<(const Pseudonym &rhs) const {
    return pseud_ < rhs.pseud_;
  }
  bool operator>(const Pseudonym &rhs) const {
    return rhs < *this;
  }
  bool operator<=(const Pseudonym &rhs) const {
    return !(rhs < *this);
  }
  bool operator>=(const Pseudonym &rhs) const {
    return !(*this < rhs);
  }

  inline operator std::string const() const {
    std::string result = "pseud_: ";
    for (int i = 0; i < pseud_.size(); ++i) {
      result += std::to_string(pseud_[i]) + " ";
    }
    return result;
  }

  void serialize(std::vector<uint8_t> &working_vec) const override {
    working_vec.insert(std::end(working_vec), std::begin(pseud_), std::end(pseud_));
  }

  static Pseudonym deserialize(const std::vector<uint8_t> &working_vec, size_t &cur) {
    Pseudonym result;
    std::copy(&working_vec[cur], &working_vec[cur] + result.pseud_.size(), result.pseud_.begin());
    return result;
  }

  static Pseudonym create_dummy() {
    return Pseudonym();
  }

};

/**
 * A Pseudonym that additionally stores an id.
 */
class PseudonymWithId : public Pseudonym {
  int id;
 public:
  PseudonymWithId(uint8_t *pseud_array, int id) : Pseudonym(pseud_array), id(id) {}

  int get_id() const {
    return id;
  }

  bool operator==(const PseudonymWithId &rhs) const {
    return static_cast<const c1::client::Pseudonym &>(*this) == static_cast<const c1::client::Pseudonym &>(rhs);
  }

  bool operator==(const Pseudonym &rhs) const {
    return static_cast<const c1::client::Pseudonym &>(*this) == rhs;
  }

  bool operator!=(const PseudonymWithId &rhs) const {
    return !(rhs == *this);
  }
};

class DecryptedPseudonym : public Serializable {
  onid_t onid_repr_;
  PeerInformation peer_information_;
  uint8_t local_num_;

  DecryptedPseudonym() {}

 public:

  DecryptedPseudonym(onid_t onid_repr_, const PeerInformation &peer_information_, uint8_t local_num_) : onid_repr_(
      onid_repr_), peer_information_(peer_information_), local_num_(local_num_) {}

  onid_t get_onid_repr() const {
    return onid_repr_;
  }
  const PeerInformation &get_peer_information() const {
    return peer_information_;
  }
  uint8_t get_local_num() const {
    return local_num_;
  }

  bool operator==(const DecryptedPseudonym &rhs) const {
    return std::tie(onid_repr_, peer_information_, local_num_)
        == std::tie(rhs.onid_repr_, rhs.peer_information_, rhs.local_num_);
  }
  bool operator!=(const DecryptedPseudonym &rhs) const {
    return !(rhs == *this);
  }

  void serialize(std::vector<uint8_t> &working_vec) const override {
    serialize_number(working_vec, onid_repr_);
    peer_information_.serialize(working_vec);
    serialize_number(working_vec, local_num_);
  }

  static DecryptedPseudonym deserialize(const std::vector<uint8_t> &working_vec, size_t &cur) {
    DecryptedPseudonym result;
    result.onid_repr_ = deserialize_number<onid_t>(working_vec, cur);
    result.peer_information_ = PeerInformation::deserialize(working_vec, cur);
    result.local_num_ = deserialize_number<uint8_t>(working_vec, cur);
    return result;
  }
};

/**
 * Message type (which actually is uint8_t-array of size kMessageSize).
 */
class Message : public Serializable {
  std::array<uint8_t, kMessageSize> msg_;

 private:
  Message() {}

 public:
  Message(uint8_t *msg_array) {
    std::copy(msg_array, msg_array + kMessageSize, msg_.begin());
  }

  bool operator==(const Message &rhs) const {
    return msg_ == rhs.msg_;
  }

  bool operator!=(const Message &rhs) const {
    return !(rhs == *this);
  }

  const std::array<uint8_t, kMessageSize> &get() const {
    return msg_;
  }

  void serialize(std::vector<uint8_t> &working_vec) const override {
    working_vec.insert(std::end(working_vec), std::begin(msg_), std::end(msg_));
  }

  static Message deserialize(const std::vector<uint8_t> &working_vec, size_t &cur) {
    Message result;
    std::copy(&working_vec[cur], &working_vec[cur] + result.msg_.size(), result.msg_.begin());
    return result;
  }

  static Message create_dummy() { return Message(); }

};

/**
 * A Message tuple consisting of n_src, m, n_dst and t_dst. (see paper)
 */
class MessageTuple : public Serializable {
 public:
  Pseudonym n_src;
  Message m;
  Pseudonym n_dst;
  round_t t_dst;

  MessageTuple(const Pseudonym &n_src, const Message &m, const Pseudonym &n_dst, round_t t_dst)
      : n_src(n_src), m(m), n_dst(n_dst), t_dst(t_dst) {}

  bool operator<(const MessageTuple &rhs) const {
    return t_dst < rhs.t_dst;
  }

  bool operator>(const MessageTuple &rhs) const {
    return rhs < *this;
  }

  bool operator<=(const MessageTuple &rhs) const {
    return !(rhs < *this);
  }

  bool operator>=(const MessageTuple &rhs) const {
    return !(*this < rhs);
  }

  bool operator==(const MessageTuple &rhs) const {
    return n_src == rhs.n_src &&
        m == rhs.m &&
        n_dst == rhs.n_dst &&
        t_dst == rhs.t_dst;
  }

  bool operator!=(const MessageTuple &rhs) const {
    return !(rhs == *this);
  }

  bool is_due(round_t cur_round, int overlay_dimension) const {
    round_t l_delivered =
        cur_round + calculate_agreement_time(overlay_dimension) + calculate_routing_time(overlay_dimension) + 3;
    return (t_dst >= l_delivered * 4 * kDelta) && (t_dst < (l_delivered + 1) * 4 * kDelta);
  }

  bool is_dummy() const {
    return t_dst == 0; // since no message can arrive at round 0
  }

  bool is_cancel() const {
    return t_dst == 1; // see above
  }

  void serialize(std::vector<uint8_t> &working_vec) const override {
    n_src.serialize(working_vec);
    m.serialize(working_vec);
    n_dst.serialize(working_vec);
    serialize_number(working_vec, t_dst);
  }

  static MessageTuple deserialize(const std::vector<uint8_t> &working_vec, size_t &cur) {
    auto n_src = Pseudonym::deserialize(working_vec, cur);
    auto m = Message::deserialize(working_vec, cur);
    auto n_dst = Pseudonym::deserialize(working_vec, cur);
    auto t_dst = deserialize_number<round_t>(working_vec, cur);
    return MessageTuple{n_src, m, n_dst, t_dst};
  };

  static MessageTuple create_dummy() {
    return MessageTuple{Pseudonym::create_dummy(), Message::create_dummy(), Pseudonym::create_dummy(), 0};
  }

  static MessageTuple create_cancel() {
    return MessageTuple{Pseudonym::create_dummy(), Message::create_dummy(), Pseudonym::create_dummy(), 1};
  }

};

/**
 * A structure used for the agreement scheme internally.
 */
struct AgreementLocalTuple {
  //AgreementLocalTuple() {}
  MessageTuple message;
  /** true means INIT, false means unaware */
  bool aware;
  DistributedAgreementScheme agreement_scheme;
  std::vector<PeerInformation> participating_nodes;
  round_t round;
};

/**
 * A structure used for the agreement scheme (sent between enclaves).
 */
class AgreementTuple : public Serializable {
 public:
  MessageTuple m;
  PeerInformation s;
  round_t l;

  AgreementTuple(const MessageTuple &m,
                 const PeerInformation &s,
                 round_t l) : m(m), s(s), l(l) {}

  void serialize(std::vector<uint8_t> &working_vec) const override {
    m.serialize(working_vec);
    s.serialize(working_vec);
    serialize_number(working_vec, l);
  }

  static AgreementTuple deserialize(const std::vector<uint8_t> &working_vec, size_t &cur) {
    auto m = MessageTuple::deserialize(working_vec, cur);
    auto s = PeerInformation::deserialize(working_vec, cur);
    auto l = deserialize_number<round_t>(working_vec, cur);
    return AgreementTuple{m, s, l};
  };

};

/**
 * A structure used for the routing scheme (sent between enclaves).
 */
struct RoutingSchemeTuple : public Serializable {
  MessageTuple m;
  onid_t onid_dst;
  Pseudonym bucket_dst;
  round_t l_dst;
  onid_t onid_current;

  RoutingSchemeTuple(const MessageTuple &m,
                     onid_t onid_dst,
                     const Pseudonym &bucket_dst,
                     round_t l_dst,
                     onid_t onid_current)
      : m(m), onid_dst(onid_dst), bucket_dst(bucket_dst), l_dst(l_dst), onid_current(onid_current) {}

  bool operator<(const RoutingSchemeTuple &rhs) const {
    return std::tie(onid_dst, bucket_dst, l_dst, onid_current, m)
        < std::tie(rhs.onid_dst, rhs.bucket_dst, rhs.l_dst, rhs.onid_current, rhs.m);
  }
  bool operator>(const RoutingSchemeTuple &rhs) const {
    return rhs < *this;
  }
  bool operator<=(const RoutingSchemeTuple &rhs) const {
    return !(rhs < *this);
  }

  bool operator>=(const RoutingSchemeTuple &rhs) const {
    return !(*this < rhs);
  }

  void serialize(std::vector<uint8_t> &working_vec) const override {
    m.serialize(working_vec);
    serialize_number(working_vec, onid_dst);
    bucket_dst.serialize(working_vec);
    serialize_number(working_vec, l_dst);
    serialize_number(working_vec, onid_current);
  }

  static RoutingSchemeTuple deserialize(const std::vector<uint8_t> &working_vec, size_t &cur) {
    auto m = MessageTuple::deserialize(working_vec, cur);
    auto onid_dst = deserialize_number<onid_t>(working_vec, cur);
    auto bucket_dst = Pseudonym::deserialize(working_vec, cur);
    auto l_dst = deserialize_number<round_t>(working_vec, cur);
    auto onid_current = deserialize_number<onid_t>(working_vec, cur);
    return RoutingSchemeTuple{m, onid_dst, bucket_dst, l_dst, onid_current};
  };

  static RoutingSchemeTuple create_dummy() {
    return RoutingSchemeTuple{MessageTuple::create_dummy(), 0, Pseudonym::create_dummy(), 0, 0};
  }
};

/**
 * A pair of PerInformation and onid.
 */
struct IdOnidPair {
  PeerInformation id;
  onid_t onid;
};

}; // !namespace

#endif //NETWORK_SGX_EXAMPLE_STRUCTURES_H
