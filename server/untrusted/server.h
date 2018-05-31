//
// Created by c1 on 18.01.18.
//

#ifndef NETWORK_SGX_EXAMPLE_SERVER_SERVER_H
#define NETWORK_SGX_EXAMPLE_SERVER_SERVER_H

#include "network/network_manager_server.h"
#include <sgx_eid.h>
#include <cstdio>

namespace c1::server {

/**
 * Main login server class.
 */
class Server {
 public:
  /**
   * Get singleton of the login server.
   * @return
   */
  static Server &instance() {
    static Server INSTANCE;
    return INSTANCE;

  }

  int run();

  /**
   * Send a message to the peer with uri recipient.
   * @param recipient
   * @param msg
   * @param msg_len
   */
  void send_msg_to_client(const std::string &recipient, const char *msg, size_t msg_len);

 private:
  Server();

  int initialize_enclave();

  /* Global EID shared by multiple threads */
  sgx_enclave_id_t global_eid_ = 0;
  NetworkManagerServer network_manager_;
};

} // ~namespace


#if defined(__cplusplus)
extern "C" {
#endif

void ocall_print_string(const char *str);
void ocall_send_msg_to_client(const char *client_uri, const char *msg, size_t msg_len);

#if defined(__cplusplus)
}
#endif

#endif //NETWORK_SGX_EXAMPLE_SERVER_SERVER_H
