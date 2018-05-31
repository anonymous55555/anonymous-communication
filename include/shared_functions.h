#ifndef NETWORK_SGX_EXAMPLE_SHARED_FUNCTIONS_H
#define NETWORK_SGX_EXAMPLE_SHARED_FUNCTIONS_H

#include <cstdint>
#include <functional>

namespace c1 {

/**
 * Perform func on all onids that are an id of a neighbor of node_id in the overlay network
 * @param node_id
 * @param dimension the dimension of the overlay network
 * @param func the function to be called on onid' where onid' is a neighbor of node_id in the overlay network
 */
void for_all_neighbors(onid_t node_id, int dimension, std::function<void(uint64_t)> func) {
  func(node_id);
  for (int i = 0; i < dimension; ++i) {
    uint64_t node_id_i_th_bit_flipped = node_id ^1UL << i;
    func(node_id_i_th_bit_flipped);
  }
}

} // !namespace

#endif //NETWORK_SGX_EXAMPLE_SHARED_FUNCTIONS_H
