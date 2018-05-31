#ifndef NETWORK_SGX_EXAMPLE_HELPERS_H
#define NETWORK_SGX_EXAMPLE_HELPERS_H

#include <cstdint>
#include "structures.h"

namespace c1::client {

/**
 * calculate L_agreement (see paper) based on the dimension of the overlay network
 * @param dim
 * @return
 */
inline uint64_t calculate_agreement_time(int dim) {
  return dim + 2;
}

/**
 * calculate L_routing (see paper) based on the dimension of the overlay network
 * @param dim
 * @return
 */
inline uint64_t calculate_routing_time(int dim) {
  return 2 * dim + 2;
}

/**
 * returns the l such that t in [4lDelta,(l+1)4Delta)
 * @param t
 * @return
 */
inline round_t calculate_round_from_t(round_t t) {
  return t / (4 * kDelta);
}

/**
 * returns the l such that t in [lDelta,(l+1)Delta)
 * @param t
 * @return
 */
inline round_t calculate_subround_from_t(round_t t) {
  return t / kDelta;
}

} //!namespace

#endif //NETWORK_SGX_EXAMPLE_HELPERS_H
