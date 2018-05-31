//
// Created by c1 on 15.01.18.
//

#include "network_manager.h"
#include <enclave_u.h>
#include <iostream>

namespace c1::client {

static constexpr auto HEARTBEAT_INTERVAL = 500;
static constexpr auto HEARTBEAT_LIVENESS = 3;

network_manager::network_manager() : context_(1), server_socket_out_(context_, ZMQ_DEALER),
                                     server_and_peer_socket_in_(context_, ZMQ_DEALER),
                                     global_sgx_eid_(0),
                                     pollitems_{server_and_peer_socket_in_, 0, ZMQ_POLLIN, 0},
                                     user_socket_in_{context_, ZMQ_PULL},
                                     pollitems_user_{user_socket_in_, 0, ZMQ_POLLIN, 0} {
  server_socket_out_.connect("tcp://localhost:5671");
  std::string id("client"
                     + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
  server_socket_out_.setsockopt(ZMQ_IDENTITY, id.c_str(), id.length()); //  Set a printable identity
  server_socket_out_.setsockopt(ZMQ_LINGER, 0);
  server_and_peer_socket_in_.setsockopt(ZMQ_LINGER, 0);
  user_socket_in_.setsockopt(ZMQ_LINGER, 0);

  char uri_chars[1024]; //make this sufficiently large.
  //otherwise an error will be thrown because of invalid argument.
  size_t size = sizeof(uri_chars);
  try {
    server_and_peer_socket_in_.bind("tcp://*:*");
  }
  catch (zmq::error_t &e) {
    std::cerr << "couldn't bind to socket: " << e.what();
    abort();
  }
  server_and_peer_socket_in_.getsockopt(ZMQ_LAST_ENDPOINT, &uri_chars, &size);
  in_port_ = get_port_from_uri(uri_chars);
  hostname_ = "127.0.0.1";


  // establish the user interface socket:
  try {
    user_socket_in_.bind("tcp://*:*");
  }
  catch (zmq::error_t &e) {
    std::cerr << "couldn't bind to socket (for user interface): " << e.what();
    abort();
  }
  user_socket_in_.getsockopt(ZMQ_LAST_ENDPOINT, &uri_chars, &size);
  auto user_port = get_port_from_uri(uri_chars);

  std::cout << "user socket is bound at port: " << user_port << std::endl;

}
int network_manager::get_port_from_uri(const char *uri_chars) {
  auto uri_str = std::string(uri_chars);
  //std::cout << "socket is bound at URI " << uri_str << std::endl;
  auto colon_index = uri_str.find_last_of(':');
  auto host = uri_str.substr(0, colon_index);
  return stoi(uri_str.substr(colon_index + 1));
}

bool network_manager::MainLoop() {

  zmq::poll(&pollitems_[0], 1, HEARTBEAT_INTERVAL);

  // check in_socket
  if (pollitems_[0].revents & ZMQ_POLLIN) {
    // receive message
    zmq::message_t msg_content;
    bool rc = server_and_peer_socket_in_.recv(&msg_content);
    assert(!msg_content.more());

    if (!initialized) { // message was sent from server
      ecall_received_msg_from_server(global_sgx_eid_, static_cast<uint8_t *>(msg_content.data()), msg_content.size());
      initialized = true;
    } else { // message was sent from other peer
      std::cout << "Received message of length " << msg_content.size() << std::endl;
      std::cout << "\trc: " << rc << std::endl;
      ecall_traffic_in(global_sgx_eid_, static_cast<uint8_t *>(msg_content.data()), msg_content.size());
    }
  }

  // check user socket
  zmq::poll(&pollitems_user_[0], 1, 0);
  if (pollitems_user_[0].revents & ZMQ_POLLIN) {
    // received message from user
    zmq::message_t msg_content;
    bool rc = user_socket_in_.recv(&msg_content);
    assert(!msg_content.more());
    //assert(msg_content.size() == 1);
    char message_type = *static_cast<char *>(msg_content.data());
    switch (message_type) {
      case 0: assert(msg_content.size() == 1);
        uint8_t pseud[kPseudonymSize];
        ecall_generate_pseudonym(global_sgx_eid_, pseud);
        std::cout << "Pseudonym is: ";
        for (int i = 0; i < kPseudonymSize; ++i) {
          std::cout << std::to_string(pseud[i]) << " ";
        }
        std::cout << std::endl;
        break;
      case 1:
        auto injection =
            *reinterpret_cast<UserInterfaceMessageInjectionCommand *>(static_cast<char *>(msg_content.data()) + 1);
        ecall_send_message(global_sgx_eid_, injection.n_src, injection.msg, injection.n_dst, injection.t_dst);
        break;
    }
  }


//server_socket_out_.close();

  return true;
}

void network_manager::set_global_sgx_eid_and_network_init(sgx_enclave_id_t global_sgx_eid_) {
  network_manager::global_sgx_eid_ = global_sgx_eid_;
  ecall_network_init(global_sgx_eid_, 127, 0, 0, 1, in_port_);
}

void network_manager::send_msg_to_server(const void *ptr, size_t len) {
  zmq::message_t message(len);
  memcpy(message.data(), ptr, len);

  bool rc = server_socket_out_.send(message);
  //return (rc);
}

void network_manager::send_msg_to_peer(const PeerInformation &peer, const uint8_t *ptr, size_t len) {
  // if connection to recipient does not yet exist, establish it
  if (peers_.count(peer.id) < 1) {
    peers_.emplace(peer.id, Peer{zmq::socket_t(context_, ZMQ_DEALER)});
    peers_.at(peer.id).socket.connect("tcp://" + std::string(peer.uri));
  }
  // send message to recipient

  zmq::message_t message(len);
  memcpy(message.data(), ptr, len);

  bool rc = peers_.at(peer.id).socket.send(message);

  //return (rc);
}

network_manager::~network_manager() {
  server_socket_out_.close();
}
bool network_manager::isInitialized() const {
  return initialized;
}

} // ~namespace
