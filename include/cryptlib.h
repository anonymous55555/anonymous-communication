#ifndef NETWORK_SGX_EXAMPLE_CRYPT_LIB_H
#define NETWORK_SGX_EXAMPLE_CRYPT_LIB_H

#include <cstdint>
#include <sgx_tcrypto.h>
#include <vector>
#include "../client/trusted/structures.h"

namespace c1::cryptlib {

std::vector<uint8_t> encrypt(const sgx_aes_gcm_128bit_key_t &sk_enc,
                             const std::vector<uint8_t> &p,
                             const std::vector<uint8_t> &aad);

std::pair<std::vector<uint8_t>, std::vector<uint8_t>> decrypt(const sgx_aes_gcm_128bit_key_t &sk_enc,
                                                              const std::vector<uint8_t> &c);

// key generation for sk_pseud or sk_end
std::array<uint8_t, SGX_AESGCM_KEY_SIZE> keygen();


std::array<uint8_t, SGX_AESGCM_KEY_SIZE> gen_routing_key();

onid_t get_intermediate_target(const sgx_cmac_128bit_key_t &sk_routing, c1::client::Pseudonym bucket_dst, round_t ell_dst, size_t overlay_dimension);

} // !namespace

#endif //NETWORK_SGX_EXAMPLE_CRYPT_LIB_H
