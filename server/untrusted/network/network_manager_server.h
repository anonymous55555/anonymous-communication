#ifndef NETWORK_SGX_EXAMPLE_SERVER_NETWORKMANAGER_H
#define NETWORK_SGX_EXAMPLE_SERVER_NETWORKMANAGER_H

#include <zmq.h>
#include <zmq.hpp>
#include <unordered_map>
#include <sgx_eid.h>

namespace c1::server {

/**
 * Struct encapsulating the socket of a peer.
 */
struct Client {
  zmq::socket_t socket;
  Client(zmq::socket_t &&socket) : socket(std::move(socket)) {}
};

/**
 * Class to manage all network stuff.
 */
class NetworkManagerServer {
 public:
  /** Constructor. */
  NetworkManagerServer();

  /**
 * Called once the global_sgx_eid is available.
 * @param global_sgx_eid_
 */
  void setGlobal_sgx_eid_(sgx_enclave_id_t global_sgx_eid_);

  /**
 * Called regularly.
 * @return true
 */
  bool main_loop();

  /**
   * Send message at ptr of length len to the peer with uri recipient.
   * @param recipient
   * @param ptr
   * @param len
   */
  void send_msg_to_client(const std::string &recipient, const char *ptr, size_t len);

 private:
  /** zeromq context */
  zmq::context_t context_;
  /** the incoming router socket */
  zmq::socket_t socket_in_;
  /** maps from URI to clients */
  std::unordered_map<std::string, Client> clients_;
  /** global sgx eid, to be able to make ecalls */
  sgx_enclave_id_t global_sgx_eid_;
  /** poller for the incoming socket */
  zmq::pollitem_t pollitems_[1];
};

} // ~namespace

#endif //NETWORK_SGX_EXAMPLE_SERVER_NETWORKMANAGER_H
