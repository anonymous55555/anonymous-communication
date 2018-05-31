//
// Created by c1 on 25.05.18.
//

#include <sgx_trts.h>
#include "cryptlib.h"
#include "enclave_t.h"

#define ASSERT(x) \
  if (!(x)) { \
    ocall_print_string("assert x; failed"); \
  } \
  assert (x);

namespace c1 {

std::vector<uint8_t> cryptlib::encrypt(const sgx_aes_gcm_128bit_key_t &sk_enc,
                                       const std::vector<uint8_t> &p,
                                       const std::vector<uint8_t> &aad) { //authenticated encryption done via randomized counter GCM

  std::vector<uint8_t> result; //will contain data in the format lct(sizeof(uint32_t)) || laad(sizeof(uint32_t)) || iv(SGX_AESGCM_IV_SIZE) || aad(laad) || mac(SGX_AESGCM_MAC_SIZE)  || ciphertext(lct)
  uint32_t result_size = sizeof(uint32_t) + sizeof(uint32_t) + SGX_AESGCM_IV_SIZE + aad.size() + SGX_AESGCM_MAC_SIZE + p.size();
  result.reserve(result_size);

  //Write length lct of the ciphertext to the result
  uint32_t lct = p.size();
  serialize_number(result, lct);

  //Write length laad of the aad to the result
  uint32_t laad = aad.size();
  serialize_number(result, laad);

  //Generate random IV and write it to result
  uint8_t iv[SGX_AESGCM_IV_SIZE];
  assert(sgx_read_rand(iv, SGX_AESGCM_IV_SIZE) == SGX_SUCCESS);
  result.insert(result.end(), &iv[0], &iv[SGX_AESGCM_IV_SIZE]);

  //Copy aad to result
  result.insert(result.end(), aad.begin(), aad.end());

  //Encrypt p(lct) with additional authenticated data: lct(sizeof(uint32_t)) || laad(sizeof(uint32_t)) || iv(SGX_AESGCM_IV_SIZE) || aad(laad)
  uint8_t ciphertext_out[lct];
  uint8_t mac_out[SGX_AESGCM_MAC_SIZE];
  auto status = sgx_rijndael128GCM_encrypt(&sk_enc,
          p.data(), lct,
          ciphertext_out,
          iv, SGX_AESGCM_IV_SIZE, //todo iv length can be chosen more cleverly knowing how much data we encrypt at most with each IV
          result.data(), sizeof(uint32_t) + sizeof(uint32_t) + SGX_AESGCM_IV_SIZE + laad, //aad for GCM
          &mac_out);
  assert(status == SGX_SUCCESS);

  //Append MAC and ciphertext to the result
  result.insert(result.end(), &mac_out[0], &mac_out[SGX_AESGCM_MAC_SIZE]);
  result.insert(result.end(), &ciphertext_out[0], &ciphertext_out[lct]);

  return result;
}

std::pair<std::vector<uint8_t>, std::vector<uint8_t>> cryptlib::decrypt(const sgx_aes_gcm_128bit_key_t &sk_enc,
                                                                        const std::vector<uint8_t> &c) {
  size_t cur = 0;
  auto lct = deserialize_number<uint32_t>(c, cur);
  auto laad = deserialize_number<uint32_t>(c, cur);
  const uint8_t* iv = &c[sizeof(uint32_t) + sizeof(uint32_t)];
  const uint8_t* aad = &c[sizeof(uint32_t) + sizeof(uint32_t) + SGX_AESGCM_IV_SIZE];
  const uint8_t* mac = &c[sizeof(uint32_t) + sizeof(uint32_t) + SGX_AESGCM_IV_SIZE + laad];
  const uint8_t* ct = &c[sizeof(uint32_t) + sizeof(uint32_t) + SGX_AESGCM_IV_SIZE + laad + SGX_AESGCM_MAC_SIZE];
  const uint8_t* gcmaad = &c[0];
  assert(c.size() == sizeof(uint32_t) + sizeof(uint32_t) + SGX_AESGCM_IV_SIZE + laad + SGX_AESGCM_MAC_SIZE + lct);

  //Copy MAC to please sgx_rijndael128GCM_decrypt's input requirements
  const uint8_t mac_tag[SGX_AESGCM_MAC_SIZE] = {mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], mac[6], mac[7],
                                                mac[8], mac[9], mac[10], mac[11], mac[12], mac[13], mac[14], mac[15]};

  //Decrypt
  uint8_t plaintext[lct];
  auto status = sgx_rijndael128GCM_decrypt(&sk_enc,
          ct, lct,
          plaintext,
          iv, SGX_AESGCM_IV_SIZE,
          gcmaad, sizeof(uint32_t) + sizeof(uint32_t) + SGX_AESGCM_IV_SIZE + laad,
          &mac_tag);
  assert(status == SGX_SUCCESS);

  std::vector<uint8_t> resultPlaintext(plaintext, plaintext+lct);
  std::vector<uint8_t> resultAad(aad, aad+laad);

  return std::pair<std::vector<uint8_t>, std::vector<uint8_t>>(resultPlaintext, resultAad);
}

std::array<uint8_t, SGX_AESGCM_KEY_SIZE> cryptlib::keygen() {
  std::array<uint8_t, SGX_AESGCM_KEY_SIZE> result;
  auto res = sgx_read_rand(result.data(), SGX_AESGCM_KEY_SIZE);
  assert(res == SGX_SUCCESS);
  return result;
}


std::array<uint8_t, SGX_AESGCM_KEY_SIZE> cryptlib::gen_routing_key() {
  std::array<uint8_t, SGX_CMAC_KEY_SIZE> result;
  auto res = sgx_read_rand(result.data(), SGX_CMAC_KEY_SIZE);
  assert(res == SGX_SUCCESS);
  return result;
}

onid_t cryptlib::get_intermediate_target(const sgx_cmac_128bit_key_t &sk_routing, c1::client::Pseudonym bucket_dst, round_t ell_dst, size_t overlay_dimension) {
  //Prepare input (bucket_dst, ell_dst) to the PRF
  std::vector<uint8_t> preimage;
  preimage.reserve(kPseudonymSize + sizeof(round_t));
  auto bucket_dst_bytes = bucket_dst.get();
  preimage.insert(preimage.end(), bucket_dst_bytes.begin(), bucket_dst_bytes.end());
  serialize_number(preimage, ell_dst);

  //Evaluate PRF
  sgx_cmac_128bit_tag_t image;
  auto status = sgx_rijndael128_cmac_msg(&sk_routing, preimage.data(), preimage.size(), &image);
  assert(status == SGX_SUCCESS);

  //Interpret PRF image as overlay node id
  onid_t result = (image[3] << 24) | (image[2] << 16) | (image[1] << 8) | (image[0]);
  result = result % (1UL << overlay_dimension); //reduce to desired dimension
  return result;
}

} // !namespace