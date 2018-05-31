//
// Created by c1 on 18.01.18.
//

#ifndef NETWORK_SGX_EXAMPLE_CLIENT_CLIENT_H
#define NETWORK_SGX_EXAMPLE_CLIENT_CLIENT_H

#include <network/network_manager.h>
#include <sgx_eid.h>
#include <cstdio>

namespace c1::client {

/** Base peer class */
class Client {
 public:
  /**
   * Access the singleton.
   * @return
   */
  static Client &instance() {
    static Client INSTANCE;
    return INSTANCE;
  }

  /** main loop (infinite) */
  int run();

  void send_msg_to_server(const void *ptr, size_t len);
  /**
   * Used to return the message pairs from traffic_out() (was necessary due to the enclave relationship)
   * @param ptr ptr to the return value
   * @param len its length
   */
  void traffic_out_return(const uint8_t *ptr, size_t len);

 private:
  Client();

  int initialize_enclave();

  /* Global EID shared by multiple threads */
  sgx_enclave_id_t global_eid_ = 0;
  network_manager network_manager_;
};

} // ~namespace

#if defined(__cplusplus)
extern "C" {
#endif

void ocall_print_string(const char *str);
void ocall_send_msg_to_server(const uint8_t *ptr, size_t len);
void ocall_traffic_out_return(const uint8_t *ptr, size_t len);

#if defined(__cplusplus)
}
#endif

#endif //NETWORK_SGX_EXAMPLE_CLIENT_CLIENT_H
