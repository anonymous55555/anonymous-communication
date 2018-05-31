//
// Created by c1 on 04.05.18.
//

#ifndef NETWORK_SGX_EXAMPLE_OVERLAYSTRUCTURESCHEME_H
#define NETWORK_SGX_EXAMPLE_OVERLAYSTRUCTURESCHEME_H

#include <cstdint>
#include "structures.h"
#include "../../include/shared_structs.h"

namespace c1::client {

// the following is not the nicest way of doing this, but it's keeping things simple for now
class OverlayStructureSchemeMessage : public Serializable {
 private:
  OverlayStructureSchemeMessage() {}

 public:
  enum class OverlayStructureSchemeMessageType : uint8_t {
    tEmulateRequestMsg,
    tEmulateRequestReceivedMsg,
    tHandOverMsg,
    tSelfIntroduceMsg
  } t;
  onid_t onid;
  PeerInformation peer_information;
  std::map<onid_t, std::vector<PeerInformation>> gamma_route;
  std::vector<PeerInformation> gamma_receive;

  OverlayStructureSchemeMessage(OverlayStructureSchemeMessageType t,
                                onid_t onid,
                                const PeerInformation &peer_information,
                                const std::map<onid_t, std::vector<PeerInformation>> &gamma_route,
                                const std::vector<PeerInformation> &gamma_receive_or_send);

  static OverlayStructureSchemeMessage createEmulateRequestMsg(onid_t onid_prime, PeerInformation i) {
    return OverlayStructureSchemeMessage{OverlayStructureSchemeMessageType::tEmulateRequestMsg, onid_prime, i,
                                         std::map<onid_t, std::vector<PeerInformation>>(),
                                         std::vector<PeerInformation>()};
  }
  static OverlayStructureSchemeMessage createEmulateRequestReceivedMsg(onid_t onid_prime, PeerInformation i) {
    return OverlayStructureSchemeMessage{OverlayStructureSchemeMessageType::tEmulateRequestReceivedMsg, onid_prime, i,
                                         std::map<onid_t, std::vector<PeerInformation>>(),
                                         std::vector<PeerInformation>()};
  }
  static OverlayStructureSchemeMessage createHandOverMessage(std::map<onid_t, std::vector<PeerInformation>> gamma_route,
                                                             std::vector<PeerInformation> gamma_receive) {
    return OverlayStructureSchemeMessage{OverlayStructureSchemeMessageType::tHandOverMsg, 0, PeerInformation(),
                                         gamma_route, gamma_receive};
  }
  static OverlayStructureSchemeMessage createSelfIntroduceMessage(PeerInformation i) {
    return OverlayStructureSchemeMessage{OverlayStructureSchemeMessageType::tHandOverMsg, 0, i,
                                         std::map<onid_t, std::vector<PeerInformation>>(),
                                         std::vector<PeerInformation>()};
  }

  void serialize(std::vector<uint8_t> &working_vec) const override;
  static OverlayStructureSchemeMessage deserialize(const std::vector<uint8_t> &working_vec, size_t &cur);

  bool operator==(const OverlayStructureSchemeMessage &rhs) const {
    return std::tie(t, onid, peer_information, gamma_route, gamma_receive)
        == std::tie(rhs.t,
                    rhs.onid,
                    rhs.peer_information,
                    rhs.gamma_route,
                    rhs.gamma_receive);
  }
  bool operator!=(const OverlayStructureSchemeMessage &rhs) const {
    return !(rhs == *this);
  }

};

struct OverlayReturnTuple {
  onid_t onid_emul;
  std::vector<PeerInformation> gamma_agree;
  std::vector<PeerInformation> gamma_send;
  std::map<onid_t, std::vector<PeerInformation>> gamma_route;
  std::vector<PeerInformation> gamma_receive;
  std::map<PeerInformation, std::vector<OverlayStructureSchemeMessage>> s_overlay_prime;
};

class OverlayStructureScheme {
  onid_t onid_assoc_;
  onid_t onid_emul_;
  std::vector<PeerInformation> gamma_agree_;
  std::vector<PeerInformation> gamma_send_;
  std::vector<PeerInformation> gamma_receive_;
  std::map<uint64_t, std::vector<PeerInformation>> gamma_route_;
  uint64_t overlay_dimension_;
  uint64_t reconfiguration_time_;
  PeerInformation own_id_;

  onid_t onid_emul_prev_;
  std::vector<PeerInformation> nodes_emulating_previous_q_e_;
  onid_t onid_emul_new_;
  std::map<uint64_t, std::vector<PeerInformation>> gamma_agree_prev_;
  std::vector<PeerInformation> gamma_send_new_;
  std::map<uint64_t, std::vector<PeerInformation>> gamma_route_new_;
  std::map<uint64_t, std::vector<PeerInformation>> gamma_route_succ_;
  std::map<uint64_t, std::vector<PeerInformation>> gamma_route_prev_;
  std::vector<PeerInformation> gamma_receive_new_;
  std::map<uint64_t, std::vector<PeerInformation>> new_neighbors_for_q_e_;

 public:
  void init(uint64_t onid_assoc,
            uint64_t onid_emul,
            const std::vector<PeerInformation> &gamma_send,
            const std::vector<PeerInformation> &gamma_receive,
            const std::map<uint64_t, std::vector<PeerInformation>> &gamma_route,
            uint64_t overlay_dimension,
            uint64_t reconfiguration_time,
            PeerInformation own_id);

  OverlayReturnTuple update(round_t round, const std::vector<OverlayStructureSchemeMessage> &set_s);

};

} // !namespace

#endif //NETWORK_SGX_EXAMPLE_OVERLAYSTRUCTURESCHEME_H

