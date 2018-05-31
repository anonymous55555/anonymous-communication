//
// Created by c1 on 18.05.18.
//

#ifndef NETWORK_SGX_EXAMPLE_AAD_TUPLE_H
#define NETWORK_SGX_EXAMPLE_AAD_TUPLE_H

#include <utility>
#include "../../../include/config.h"
#include "../overlay_structure_scheme.h"
#include "../../../include/serialization.h"
#include "../../../include/shared_structs.h"

namespace c1::client {

/**
 * Structure used to store the additional authenticated data (sent via an untrusted path)
 */
struct AadTuple : Serializable {
  PeerInformation sender;
  PeerInformation receiver;
  round_t round;
  std::vector<OverlayStructureSchemeMessage> p_structure;

  AadTuple(PeerInformation sender,
           PeerInformation receiver,
           round_t round,
           std::vector<OverlayStructureSchemeMessage> p_structure)
      : sender(std::move(sender)), receiver(std::move(receiver)), round(round), p_structure(std::move(p_structure)) {}

  void serialize(std::vector<uint8_t> &working_vec) const override {
    //std::vector<uint8_t> aad;
    sender.serialize(working_vec);
    receiver.serialize(working_vec);
    serialize_number(working_vec, round);
    serialize_vec(working_vec, p_structure);
  }

  static AadTuple deserialize(const std::vector<uint8_t> &working_vec, size_t &cur) {
    auto sender = PeerInformation::deserialize(working_vec, cur);
    auto receiver = PeerInformation::deserialize(working_vec, cur);
    auto round = deserialize_number<round_t>(working_vec, cur);
    auto structure_msg = deserialize_vec<OverlayStructureSchemeMessage>(working_vec, cur);

    return AadTuple{sender, receiver, round, structure_msg};
  }

  bool operator==(const AadTuple &rhs) const {
    return std::tie(sender, receiver, round, p_structure)
        == std::tie(rhs.sender, rhs.receiver, rhs.round, rhs.p_structure);
  }
  bool operator!=(const AadTuple &rhs) const {
    return !(rhs == *this);
  }

  inline operator std::string const() const {
    std::string result = "Aad_Tuple: ";
    result += "Sender: " + std::string(sender) + ", ";
    result += "Receiver: " + std::string(receiver) + ", ";
    result += "Round: " + std::to_string(round) + ", ";
    result += "p_structure.size(): " + std::to_string(p_structure.size()) + "\n";
    return result;
  }
};

}

#endif //NETWORK_SGX_EXAMPLE_AAD_TUPLE_H
