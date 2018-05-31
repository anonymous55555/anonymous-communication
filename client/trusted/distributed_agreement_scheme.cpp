//
// Created by c1 on 06.05.18.
//

#include "distributed_agreement_scheme.h"

namespace c1::client {

DistributedAgreementScheme::DistributedAgreementScheme() : set_s_i_() {

}

std::vector<DistributedAgreementSchemePair> DistributedAgreementScheme::update(bool aware,
                                                                               const std::vector<PeerInformation> &participating_nodes,
                                                                               PeerInformation own_id,
                                                                               std::vector<PeerInformation> &s_m,
                                                                               round_t l) {
  std::vector<PeerInformation> s_out;
  if (aware) { // init
    set_s_i_.insert(own_id);
    s_out.push_back(own_id);
  }
  if (set_s_i_.empty()) {
    set_s_i_.insert(own_id);
    std::copy(set_s_i_.begin(), set_s_i_.end(), std::back_inserter(s_out));
  } else if (!s_m.empty()) {
    set_s_i_.insert(std::begin(s_m), std::end(s_m));
  }

  std::vector<DistributedAgreementSchemePair> result;
  for (auto &id : participating_nodes) {
    for (auto &s : s_out) {
      result.emplace_back(DistributedAgreementSchemePair{id, s});
    }
  }

  return result;
}

bool DistributedAgreementScheme::finalize(PeerInformation own_id,
                                          std::vector<PeerInformation> &s_m,
                                          size_t t) {
  set_s_i_.insert(std::begin(s_m), std::end(s_m));
  set_s_i_.insert(own_id);
  return set_s_i_.size() >= t + 1;
}

} // !namespace