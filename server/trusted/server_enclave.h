#ifndef _ENCLAVE_H_
#define _ENCLAVE_H_

#include <cstdlib>
#include <cassert>
#include <vector>
#include <string>
#include "../../include/shared_structs.h"

namespace c1::server {

/** currently hardcodes the number of clients */
constexpr int kNumRequiredClients{81};
/** currently hardcodes the dimension of the overlay network */
constexpr int kDimension{3};
/** currently hardcodes the desired number of nodes associated to each quorum */
constexpr int kNumNodesPerQuorum{10};
/** the total number of nodes in the overlay network */
constexpr int kNumQuorumNodes = 1 << kDimension;

/**
 * The enclave of the login server.
 * Methods are very similar to those of the client_enclave.
 */
class ServerEnclave {
private:
    ServerEnclave() = default;

public:
  /**
   * retrieve the singleton
   * @return singleton reference
   */
  static ServerEnclave &instance() {
      static ServerEnclave INSTANCE;
      return INSTANCE;
  }

    void init();
    void received_msg_from_client(const void *ptr, size_t len);
    int main_loop();


private:
    void try_and_send_msg(std::string client_uri, char *msg_raw, int msg_len, std::string msg_desc) const;
    void initialize_system();

    std::vector<PeerInformation> clients_; //very simple: each client gets added with its uri and id
  bool initialized = false;


};

} //~namespace

#if defined(__cplusplus)
extern "C" {
#endif

void ecall_init() { c1::server::ServerEnclave::instance().init(); }
void ecall_received_msg_from_client(const char *ptr, size_t len) {c1::server::ServerEnclave::instance().received_msg_from_client(ptr, len);}
int ecall_main_loop() { return c1::server::ServerEnclave::instance().main_loop(); }

#if defined(__cplusplus)
}
#endif

#endif /* !_ENCLAVE_H_ */
