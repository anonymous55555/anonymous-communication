#ifndef NETWORK_SGX_EXAMPLE_CLIENT_NETWORKMANAGER_H
#define NETWORK_SGX_EXAMPLE_CLIENT_NETWORKMANAGER_H

#include <zmq.h>
#include <zmq.hpp>
#include <unordered_map>
#include <sgx_eid.h>
#include "../../../include/shared_structs.h"


namespace c1::client {

/**
 * Struct encapsulating the socket of a peer.
 */
struct Peer {
  zmq::socket_t socket;
  Peer(zmq::socket_t &&socket) : socket(std::move(socket)) {}
};

/**
 * Class to manage all network stuff.
 */
class network_manager {
 public:
  /**
   * Constructor.
   */
  network_manager();
  /**
   * Destructor.
   */
  virtual ~network_manager();

  /**
   * Called once the global_sgx_eid is available.
   * @param global_sgx_eid_
   */
  void set_global_sgx_eid_and_network_init(sgx_enclave_id_t global_sgx_eid_);

  /**
   * Called regularly.
   * @return true
   */
  bool MainLoop();

  /**
   * Send message at ptr of length len to the login_server.
   * @param ptr
   * @param len
   */
  void send_msg_to_server(const void *ptr, size_t len);
  /**
   * Send message at ptr of length len to the peer given by peer.
   * @param peer
   * @param ptr
   * @param len
   */
  void send_msg_to_peer(const PeerInformation &peer, const uint8_t *ptr, size_t len);

 private:
  /** zeromq context */
  zmq::context_t context_;
  /** outgoing socket to the login server */
  zmq::socket_t server_socket_out_;
  /** the incoming socket for all messages from server (prior to initialization) and other peers (after initialization) */
  zmq::socket_t server_and_peer_socket_in_;
  /** the incoming socket for messages from the peer interface */
  zmq::socket_t user_socket_in_;
  /** global sgx eid, to be able to make ecalls */
  sgx_enclave_id_t global_sgx_eid_;
  /** poller for the server and peer in-socket */
  zmq::pollitem_t pollitems_[1];
  /** poller for the peer interface in-socket */
  zmq::pollitem_t pollitems_user_[1];
  /** port of server_and_peer_socket_in_ */
  int in_port_;
  std::string hostname_;
  /** maps PeerInformation to peers */
  std::unordered_map<uint64_t, Peer> peers_;
  /** whether the system has already been initialized (login server's work is done, all peers have joined the system) */
  bool initialized = false;

 public:
  /**
   *  returns whether the system has already been initialized (login server's work is done, all peers have joined the system)
   * @return
   */
  bool isInitialized() const;
 private:

  int get_port_from_uri(const char *uri_chars);
};

} //!namespace


#endif //NETWORK_SGX_EXAMPLE_CLIENT_NETWORKMANAGER_H
