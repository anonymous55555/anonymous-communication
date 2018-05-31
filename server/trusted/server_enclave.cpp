#include <sgx_trts.h>
#include "server_enclave.h"
#include "enclave_t.h"
#include "../../include/errors.h"
#include "../../include/shared_functions.h"
#include "../../include/cryptlib.h"

namespace c1::server {

void ServerEnclave::init() {
  ocall_print_string("ServerEnclave initialized!\n");
}

void ServerEnclave::received_msg_from_client(const void *ptr, size_t len) {
  if (get_message_type(ptr) == kTypeJoinMessage) {

    assert(sizeof(JoinMessage) == len);
    const JoinMessage received_message = *reinterpret_cast<const JoinMessage *>(ptr);
    ocall_print_string("ServerEnclave received join message from: \n");
    ocall_print_string(received_message.to_string().c_str());

    if (clients_.size() < kNumRequiredClients) {
      clients_.emplace_back(received_message.sender);
      ocall_print_string(
          ("Current number of registered peers: " + std::to_string(clients_.size()) + "\n").c_str());
      if (clients_.size() >= kNumRequiredClients) {
        initialize_system();
      }
    }
  }
}

int ServerEnclave::main_loop() {
  return !initialized;
}

void ServerEnclave::try_and_send_msg(std::string client_uri, char *msg_raw, int msg_len, std::string msg_desc) const {
  sgx_status_t ret = ocall_send_msg_to_client(client_uri.c_str(), msg_raw, msg_len);
  if (ret != SGX_SUCCESS) {
    std::string error_text = "ocall for sending " + msg_desc + " failed......\n";
    ocall_print_string(error_text.c_str());
    for (int idx = 0; idx < sizeof sgx_errlist / sizeof sgx_errlist[0]; idx++) {
      if (ret == sgx_errlist[idx].err) {
        ocall_print_string(sgx_errlist[idx].msg);
      }
    }
  }
}

void ServerEnclave::initialize_system() {
  ocall_print_string("Ready to initialize the system.\n");

  //Generate keys
  std::array<uint8_t, SGX_AESGCM_KEY_SIZE> sk_pseud = c1::cryptlib::keygen();
  std::array<uint8_t, SGX_AESGCM_KEY_SIZE> sk_enc = c1::cryptlib::keygen();
  std::array<uint8_t, SGX_CMAC_KEY_SIZE> sk_routing = c1::cryptlib::gen_routing_key();

  //Initialize overlay schemes
  for (int i = 0; i < clients_.size(); ++i) {
    clients_[i].id = i;
  }

  std::vector<std::vector<PeerInformation>> associated_quorums; // stores, for each quorum, the associated nodes
  std::vector<std::vector<PeerInformation>>
      emulated_quorums(kNumQuorumNodes); // stores, for each quorum, the nodes emulating this quorum
  std::vector<uint64_t> clients_associated_quorums
      (kNumRequiredClients); // stores redundant data, used for efficiency, maps each client to its associated quorum
  std::vector<uint64_t> clients_emulated_quorums
      (kNumRequiredClients); // stores redundant data, used for efficiency, maps each client to the quorum it emulates

  for (int i = 0; i < kNumQuorumNodes; ++i) {
    associated_quorums.push_back(std::vector<PeerInformation>());

    for (int j = i * kNumNodesPerQuorum;
         j < (i + 1) * kNumNodesPerQuorum || (i == kNumQuorumNodes - 1 && j < kNumRequiredClients); ++j) {
      associated_quorums.at(i).push_back(clients_.at(j));
      clients_associated_quorums[j] = i;
    }
  }

  for (int i = 0; i < kNumRequiredClients; ++i) {
    uint64_t random_number;
    sgx_read_rand((unsigned char *) &random_number, 8);
    uint64_t random_quorum = random_number % kNumQuorumNodes;
    emulated_quorums.at(random_quorum).push_back(clients_.at(i));
    clients_emulated_quorums[i] = random_quorum;
  }

  for (int i = 0; i < kNumQuorumNodes; ++i) {
    assert(emulated_quorums.at(i).size() > 0);
    ocall_print_string((std::string("Quorum ") + std::to_string(i) + " is emulated by: ").c_str());
    for (auto client : emulated_quorums.at(i)) {
      ocall_print_string((std::to_string(client.id) + ", ").c_str());
    }
    ocall_print_string("\n");
  }

  // send the init message to every client:
  for (int i = 0; i < kNumRequiredClients; ++i) {

    // gamma_send : all nodes that emulate the quorum node that i is associated with
    std::vector<PeerInformation> gamma_send = emulated_quorums.at(clients_associated_quorums[i]);
    // gamma_receive: all nodes that are associated with the quorum node that i emulates
    std::vector<PeerInformation> gamma_receive = associated_quorums.at(clients_emulated_quorums[i]);
    std::map<uint64_t, std::vector<PeerInformation>> gamma_route;

    for_all_neighbors(clients_emulated_quorums[i],
                      kDimension,
                      [&gamma_route, &emulated_quorums](uint64_t neighbor_quorum) {
                        gamma_route[neighbor_quorum] = emulated_quorums.at(neighbor_quorum);
                      });

    InitMessage init_message(clients_[i].id, kNumRequiredClients, kDimension,
                             clients_associated_quorums[i], clients_emulated_quorums[i],
                             gamma_send, gamma_receive, gamma_route, sk_pseud, sk_enc, sk_routing);

    auto init_message_serialized = init_message.serialize();
    try_and_send_msg(clients_[i].uri,
                     init_message_serialized.first.get(),
                     init_message_serialized.second,
                     "init msg to client " + std::to_string(i));
  }
  initialized = true;
}

} // ~namespace