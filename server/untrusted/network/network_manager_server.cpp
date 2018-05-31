//
// Created by c1 on 15.01.18.
//

#include "network_manager_server.h"
#include "../enclave_u.h"
#include <thread>

namespace c1::server {

static constexpr auto HEARTBEAT_INTERVAL = 1000;
static constexpr auto HEARTBEAT_LIVENESS = 3;

NetworkManagerServer::NetworkManagerServer()
    : context_(1), socket_in_(context_, ZMQ_ROUTER), clients_{}, global_sgx_eid_(0),
      pollitems_{socket_in_, 0, ZMQ_POLLIN, 0} {
    socket_in_.bind("tcp://*:5671"); // todo: move somewhere else...
}

bool NetworkManagerServer::main_loop() {
    //static int run{0};

    zmq::poll(&pollitems_[0], 1, 0);

    if (pollitems_[0].revents & ZMQ_POLLIN) {
        // receive identity of client
        zmq::message_t msg_client_identity;
        socket_in_.recv(&msg_client_identity);
        assert(msg_client_identity.more());
        //client_id_ = std::string(static_cast<char *>(msg_client_identity.data()), msg_client_identity.size());
        //std::cout << "UNTRUSTED(server): Received message from client " << client_id_ << std::endl;

        // receive actual message content
        zmq::message_t msg_content;
        socket_in_.recv(&msg_content);
        assert(!msg_content.more());

        ecall_received_msg_from_client(global_sgx_eid_, reinterpret_cast<char*>(msg_content.data()), msg_content.size());
    }

    return true;
}

void NetworkManagerServer::setGlobal_sgx_eid_(sgx_enclave_id_t global_sgx_eid_) {
    NetworkManagerServer::global_sgx_eid_ = global_sgx_eid_;
}

void NetworkManagerServer::send_msg_to_client(const std::string &recipient, const char *ptr, size_t len) {
    // if connection to recipient does not yet exist, establish it
    if (clients_.count(recipient) < 1) {
//        std::cout << "Establishing connection to client " << recipient << std::endl;
        clients_.emplace(recipient, Client{zmq::socket_t(context_, ZMQ_DEALER)});
        clients_.at(recipient).socket.connect("tcp://" + recipient);
    }
    // send message to recipient

    zmq::message_t message(len);
    memcpy(message.data(), ptr, len);

    bool rc = clients_.at(recipient).socket.send(message);
    //return (rc);
//    std::cout << "(UNTRUSTED) Server: Sent message to client..." << std::endl;
}

} // ~namespace