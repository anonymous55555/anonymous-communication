#ifndef NETWORK_SGX_EXAMPLE_DISTRIBUTEDAGREEMENTSCHEME_H
#define NETWORK_SGX_EXAMPLE_DISTRIBUTEDAGREEMENTSCHEME_H

#include <vector>
#include <set>
#include "../../include/config.h"
#include "../../include/shared_structs.h"

namespace c1::client {

struct DistributedAgreementSchemePair {
  PeerInformation receiver;
  PeerInformation aware_node;
};

/**
 * The distributed agreement scheme.
 */
class DistributedAgreementScheme {
 private:
  /** see paper */
  std::set<PeerInformation> set_s_i_;

 public:
  DistributedAgreementScheme();

  /**
   * see paper
   * @param aware
   * @param participating_nodes
   * @param own_id
   * @param s_m
   * @param l
   * @return
   */
  std::vector<DistributedAgreementSchemePair> update(bool aware,
                                                     const std::vector<PeerInformation> &participating_nodes,
                                                     PeerInformation own_id,
                                                     std::vector<PeerInformation> &s_m,
                                                     round_t l);

  /** see paper */
  bool finalize(PeerInformation own_id,
                std::vector<PeerInformation> &s_m,
                size_t t);

};

} // !namespace


#endif //NETWORK_SGX_EXAMPLE_DISTRIBUTEDAGREEMENTSCHEME_H
