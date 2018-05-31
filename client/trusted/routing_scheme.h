#ifndef NETWORK_SGX_EXAMPLE_ROUTING_SCHEME_H
#define NETWORK_SGX_EXAMPLE_ROUTING_SCHEME_H

#include <sgx_tcrypto.h>
#include "structures.h"

namespace c1::client {

/**
 * The routing scheme (see paper).
 */
class RoutingScheme {
 public:
  /**
   * see paper
   * @param set_s
   * @param cur_round
   * @param overlay_dimension
   * @param sk_routing
   * @return
   */
  static std::vector<RoutingSchemeTuple> route(std::vector<RoutingSchemeTuple> &set_s,
                                               round_t cur_round,
                                               size_t overlay_dimension,
                                               const sgx_cmac_128bit_key_t &sk_routing
  );

 private:
  /** determine all messages that have to be cancelled in the current call of route()
   *  (messages that exceed k_receive for one target bucket are dropped and replaced by M_cancel, see paper)
   */
  static std::set<RoutingSchemeTuple> determine_elements_to_be_cancelled(std::vector<RoutingSchemeTuple> &set_s);
};

} // !namespace


#endif //NETWORK_SGX_EXAMPLE_ROUTING_SCHEME_H
