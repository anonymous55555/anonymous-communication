#include <sgx_trts.h>
#include <sgx_tae_service.h>
#include "../../include/config.h"
#include <cmath>
#include <structures/aad_tuple.h>
#include "client_enclave.h"
#include "enclave_t.h"  /* print_string */
#include "../../include/errors.h"
#include "routing_scheme.h"
#include "../../include/cryptlib.h"

#define ASSERT(x) \
  if (!(x)) { \
    ocall_print_string("assert " #x "; failed\n"); \
  } \
  assert (x);

namespace c1::client {

std::string create_uri(std::string &ip, int port) {
  return std::string("tcp://" + std::string(ip) + ":" + std::to_string(port));
}

void ClientEnclave::init() {
  ASSERT(sgx_create_pse_session() == SGX_SUCCESS);

}

void ClientEnclave::network_init(uint8_t ip1, uint8_t ip2, uint8_t ip3, uint8_t ip4, uint64_t port) {

  own_id_.uri = Uri{ip1, ip2, ip3, ip4, port};

  JoinMessage join_msg(own_id_);
  try_and_send_msg_to_server(reinterpret_cast<void *>(&join_msg), sizeof(join_msg), "join message");
}

void ClientEnclave::received_msg_from_server(const void *msg, size_t msg_len) {
  if (get_message_type(msg) == kTypeInitMessage) {
    sgx_time_t current_time;
    sgx_time_source_nonce_t time_source_nonce;
    ASSERT(sgx_get_trusted_time(&current_time, &time_source_nonce) == SGX_SUCCESS);
    //init_time_ = current_time + 2 * kDelta; // to make it fit to the paper
    init_time_ = current_time;
    std::copy(std::begin(time_source_nonce), std::end(time_source_nonce), std::begin(init_time_nonce_));

    InitMessage init_message(reinterpret_cast<const char *>(msg));

    memcpy(sk_pseud_, init_message.get_sk_pseud_(), SGX_AESGCM_KEY_SIZE);
    memcpy(sk_enc_, init_message.get_sk_enc_(), SGX_AESGCM_KEY_SIZE);
    memcpy(sk_routing_, init_message.get_sk_routing_(), SGX_CMAC_KEY_SIZE);

    overlay_dimension_ = init_message.get_overlay_dimension_();
    onid_repr_ = init_message.get_onid_assoc_();

    overlay_structure_scheme_.init(init_message.get_onid_assoc_(),
                                   init_message.get_onid_emul_(),
                                   init_message.get_gamma_send_(),
                                   init_message.get_gamma_receive_(),
                                   init_message.get_gamma_route_(),
                                   overlay_dimension_,
                                   overlay_dimension_ + 7,
                                   own_id_);

    max_quorum_size_ = static_cast<size_t>(std::ceil(
        (1 + 1.0 / (kX * kX)) * std::pow(log2(init_message.get_num_total_nodes_()), 1 + kEpsilon)));

    auto b_max = max_quorum_size_ * kAMax;
    max_routing_msg_out_ =
        static_cast<size_t>(std::ceil(8 * std::pow(overlay_dimension_, kEpsilon + 2) * kRecv * b_max));

    own_id_.id = init_message.get_receiver_id_();

    initialized_ = true;

    m_corrupt_ = max_quorum_size_ / 2 - 1;

    ocall_print_string("ClientEnclave initialized Overlay Structure Scheme. \n");
  }
}

void ClientEnclave::try_and_send_msg_to_server(void *msg_raw, int msg_len, std::string msg_desc) const {
  sgx_status_t ret = ocall_send_msg_to_server(reinterpret_cast<uint8_t *>(msg_raw), msg_len);
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

void ClientEnclave::generate_pseudonym(uint8_t pseudonym[kPseudonymSize]) {
  memset(pseudonym, 0, kPseudonymSize);
  if (pseudonyms_.size() >= kAMax) { // too many pseudonyms already
    return;
  }

  DecryptedPseudonym pseud{onid_repr_, own_id_, pseudonyms_.size()};

  std::vector<uint8_t> pseud_vec;
  pseud.serialize(pseud_vec);
  std::vector<uint8_t> aad_vec;
  auto encrypted_pseudonym = cryptlib::encrypt(sk_pseud_, pseud_vec, aad_vec);

  for (int i = 0; i < std::min(kPseudonymSize, encrypted_pseudonym.size()); ++i) {
    pseudonym[i] = encrypted_pseudonym[i];
  }

  pseudonyms_.push_back(pseud);
  num_q_out_entries_for_round_for_pseudonym_.resize(num_q_out_entries_for_round_for_pseudonym_.size() + 1);
  q_in_for_pseudonyms_.resize(q_in_for_pseudonyms_.size() + 1);
}

void ClientEnclave::send_message(uint8_t n_src[kPseudonymSize],
                                 uint8_t msg[kMessageSize],
                                 uint8_t n_dst[kPseudonymSize],
                                 uint64_t t_dst) {
  if (!initialized_) {
    return;
  }

  auto pseud_n_src_decr = decrypt_pseudonym(Pseudonym{n_src});
  auto pseud_n_src = Pseudonym{n_src};
  auto msg_msg = Message{msg};
  auto pseud_n_dst = Pseudonym{n_dst};

  if (std::find(pseudonyms_.begin(), pseudonyms_.end(), pseud_n_src_decr) == pseudonyms_.end()) {
    // this node does not have pseudonym n_src, abort
    ocall_print_string("Source pseudonym does not exist at this node!\n");
    return;
  }

  ocall_print_string("TEMP\n");

  if (get_time() > t_dst
      - (calculate_agreement_time(overlay_dimension_) + calculate_routing_time(overlay_dimension_) + 4) * 4 * kDelta) {
    // message is too late, abort
    ocall_print_string("Message is too late! Canceled ...\n");
    return;
  }

  int l_dst = calculate_round_from_t(t_dst);
  auto &num_entries_for_round =
      num_q_out_entries_for_round_for_pseudonym_.at(pseud_n_src_decr.get_local_num());
  if (num_entries_for_round.count(l_dst) != 0 && num_entries_for_round[l_dst] >= kSend) {
    // too many message for that round already sent
    ocall_print_string("Message limit for that round was exceeded ...\n");
    return;
  }
  MessageTuple m{pseud_n_src, msg_msg, pseud_n_dst, t_dst};
  q_out_.push(m);
  if (num_entries_for_round.count(l_dst) == 0) {
    num_entries_for_round[l_dst] = 1;
  } else {
    num_entries_for_round[l_dst] += 1;
  }
}

int ClientEnclave::receive_message(uint8_t *n_dst,
                                   uint8_t *msg,
                                   uint8_t *n_src,
                                   uint64_t *t_dst) {
  if (!initialized_) {
    return false;
  }

  auto pseud_n_dst = decrypt_pseudonym(Pseudonym{n_dst});
  if (std::find(pseudonyms_.begin(), pseudonyms_.end(), pseud_n_dst) == pseudonyms_.end()) {
    // this node does not have pseudonym n_dst, abort
    ocall_print_string("This pseudonym does not exist!\n");
    return false;
  }
  auto &message_tuple = q_in_for_pseudonyms_.at(pseud_n_dst.get_local_num()).top();
  if (message_tuple.t_dst > get_time()) {
    // even the message with lowest t_dst is not due yet, abort
    ocall_print_string("No message ready!\n");
    return false;
  }
  std::copy(message_tuple.m.get().data(), message_tuple.m.get().data() + kMessageSize, msg);
  std::copy(message_tuple.n_dst.get().data(), message_tuple.n_dst.get().data() + kMessageSize, n_dst);
  *t_dst = message_tuple.t_dst;

  q_in_for_pseudonyms_.at(pseud_n_dst.get_local_num()).pop();
  return true;
}

int ClientEnclave::traffic_out() {
  if (!initialized_) {
    return false;
  }

  typedef std::map<PeerInformation, std::vector<MessageTuple>> default_out_t;
  default_out_t out_announce;
  std::map<PeerInformation, std::vector<AgreementTuple>> out_agreement;
  default_out_t out_inject;
  std::map<PeerInformation, std::vector<RoutingSchemeTuple>> out_routing;
  default_out_t out_predeliver;
  default_out_t out_deliver;
  std::vector<RoutingSchemeTuple> s_routing;


  // establish a round model
  auto subround = calculate_subround_from_t(get_time());
  if (subround % 4 != 0) {
    return false;
  }
  if (subround / 4 == cur_round_) {
    return false; // we already had a call of traffic_out this round...
  }
  ASSERT(cur_round_ == (subround / 4) - 1); // otherwise, I am corrupted - so quit
  cur_round_++;

  ocall_print_string(("traffic_out called ... in round " + std::to_string(cur_round_) + " (time "
      + std::to_string(get_time()) + "). " +
      "Message can be sent for t = " + std::to_string(get_time() + (calculate_agreement_time(overlay_dimension_)
      + calculate_routing_time(overlay_dimension_) + 4) * 4 * kDelta) + "\n").c_str());

  // auto& in  // in must be treated differently, given our implementation

  //run overlay maintenance
  auto overlay_result = overlay_structure_scheme_.update(cur_round_, in_structure_.at(0));
  gamma_agree_for_round_.push_front(overlay_result.gamma_agree);
  if (gamma_agree_for_round_.size() > calculate_agreement_time(overlay_dimension_)) {
    gamma_agree_for_round_.pop_back(); // do store more than L_agreement entries
  }
  auto &onid_emul_l_now = overlay_result.onid_emul;
  auto &onid_emul_l_prev = onid_emul_;
  onid_emul_ = onid_emul_l_now;
  auto &out_structure = overlay_result.s_overlay_prime;
  auto &gamma_route = overlay_result.gamma_route;

  //announce outgoing messages
  while (!q_out_.empty() && q_out_.top().is_due(cur_round_, overlay_dimension_)) {
    ocall_print_string("Announcing a message!\n");
    for (auto &peer : overlay_result.gamma_send) {
      out_announce[peer].emplace_back(q_out_.top());
    }
    q_out_.pop();
  }

  // start agreement for (other nodes') outgoing messages
  for (auto &announce: in_announce_[0]) {
    ocall_print_string("I received an announce message\n");
    agreement_tuples_.emplace_back(AgreementLocalTuple{announce, true, DistributedAgreementScheme(),
                                                       gamma_agree_for_round_.at(1), 0});
  }

  // update running agreement schemes
  for (auto &agreement: in_agreement_[0]) {
    if (agreement.l < calculate_agreement_time(overlay_dimension_) - 1) {
      bool found = false;
      for (auto &elem: agreement_tuples_) {
        if (elem.message == agreement.m) {
          found = true;
          break;
        }
      }
      if (!found) {
        agreement_tuples_.emplace_back(AgreementLocalTuple{agreement.m, false, DistributedAgreementScheme(),
                                                           gamma_agree_for_round_.at(agreement.l), agreement.l - 1});
      }
    }
  }
  for (auto &agreement_tuple: agreement_tuples_) {
    // compute s_m
    std::vector<PeerInformation> s_m;
    for (auto &agreement_in: in_agreement_[0]) {
      if (agreement_tuple.message == agreement_in.m) {
        s_m.push_back(agreement_in.s);
      }
    }
    // call update
    auto new_agreement_messages = agreement_tuple.agreement_scheme.update(agreement_tuple.aware,
                                                                          agreement_tuple.participating_nodes,
                                                                          own_id_,
                                                                          s_m,
                                                                          agreement_tuple.round + 1);
    // update tuple
    agreement_tuple.round++;
    // reset state
    agreement_tuple.aware = false;
    // add messages to out
    for (auto &new_agreement_message : new_agreement_messages) {
      out_agreement[new_agreement_message.receiver].emplace_back(AgreementTuple{agreement_tuple.message,
                                                                                new_agreement_message.aware_node,
                                                                                agreement_tuple.round + 1});
    }
  }

  // finalize finished agreement scheme runs
  for (auto &agreement_tuple : agreement_tuples_) {
    if (agreement_tuple.round == calculate_agreement_time(overlay_dimension_) - 1) {
      // compute s_m
      std::vector<PeerInformation> s_m;
      for (auto &agreement_in: in_agreement_[0]) {
        if (agreement_tuple.message == agreement_in.m) {
          s_m.push_back(agreement_in.s);
        }
      }
      // call finalize
      bool v = agreement_tuple.agreement_scheme.finalize(own_id_, s_m, 0);
      if (!v) {
        continue; // v = 0, do nothing
      }
      // deletion see below
      // decrypt
      auto onid_src = decrypt_pseudonym(agreement_tuple.message.n_src).get_onid_repr();
      for (auto &id: gamma_route.at(onid_src)) {
        out_inject[id].emplace_back(agreement_tuple.message);
      }
    }
  }
  // remove the tuples of the current round (did not do this during the loop for performance reasons)
  auto it = std::remove_if(agreement_tuples_.begin(), agreement_tuples_.end(),
                           [this](const AgreementLocalTuple &agreement_tuple) {
                             return agreement_tuple.round == calculate_agreement_time(overlay_dimension_);
                           });

  agreement_tuples_.erase(it, agreement_tuples_.end());


  // inject (other nodes') message into routing
  auto in_inject_filtered = obtain_elements_that_exceed_m_corrupt<MessageTuple>(in_inject_[0]);

  for (const auto &inject_message : in_inject_filtered) {
    if (inject_message.is_dummy()) {
      continue; // ignore this message
    }
    auto onid_dst = decrypt_pseudonym(inject_message.n_dst).get_onid_repr();
    auto onid_src = decrypt_pseudonym(inject_message.n_src).get_onid_repr();
    s_routing.emplace_back(RoutingSchemeTuple{inject_message,
                                              onid_dst,
                                              inject_message.n_dst,
                                              cur_round_ + calculate_routing_time(overlay_dimension_),
                                              onid_src
    });
  }

  // route messages
  auto s_primes =
      obtain_elements_that_exceed_m_corrupt<std::vector<RoutingSchemeTuple>>(
          in_routing_[0]);
  std::vector<RoutingSchemeTuple> v_set;
  for (const auto &s_uppercase : s_primes) {
    for (const auto &s_lowercase : s_uppercase) {
      v_set.push_back(s_lowercase);
    }
  }
  for (const auto &v : v_set) {
    if (v.l_dst < cur_round_) {
      s_routing.push_back(v);
    }
  }
  auto s_routing_prime = RoutingScheme::route(s_routing, cur_round_, overlay_dimension_, sk_routing_);
  for (const auto &[onid, peers] : gamma_route) {
    for (const auto &i : peers) {
      std::vector<RoutingSchemeTuple> s_prime_onid_current;
      auto it = std::copy_if(s_routing_prime.begin(), s_routing_prime.end(), std::back_inserter(s_prime_onid_current),
                             [&](const auto elem) { return elem.onid_current == onid; });
      ASSERT (out_routing.count(i) == 0);
      out_routing[i] = s_prime_onid_current;
    }
  }

  // run pre-delivery majority vote
  for (const auto &v : v_set) {
    if (v.l_dst == cur_round_) {
      for (const auto &i : gamma_route[onid_emul_l_prev])
        out_predeliver[i].emplace_back(v.m);
    }
  }

  // deliver messages to final destination
  auto set_of_predeliver_messages = obtain_elements_that_exceed_m_corrupt<MessageTuple>(
      in_predeliver_[0]);
  for (const auto &message : set_of_predeliver_messages) {
    if (!message.is_dummy()) {
      out_deliver[decrypt_pseudonym(message.n_dst).get_peer_information()].emplace_back(message);
    }
  }

  // add dummy messages
  for (const auto &i : overlay_result.gamma_send) { // announce type
    ASSERT (out_announce[i].size() <= kSend * kAMax);
    out_announce[i].reserve(kSend * kAMax);
    while (out_announce[i].size() < kSend * kAMax) {
      out_announce[i].emplace_back(MessageTuple::create_dummy());
    }
  }

  for (const auto&[onid, ids] : overlay_result.gamma_route) { // routing type
    for (const auto &i : ids) {
      ASSERT (out_routing[i].size() <= max_routing_msg_out_);
      out_routing[i].reserve(max_routing_msg_out_);
      while (out_routing[i].size() < max_routing_msg_out_) {
        out_routing[i].emplace_back(RoutingSchemeTuple::create_dummy());
      }
    }
  }

  for (const auto &i : overlay_result.gamma_route[onid_emul_l_prev]) { // predeliver type
    ASSERT (out_predeliver[i].size() <= kRecv * kAMax * overlay_result.gamma_receive.size());
    out_predeliver[i].reserve(kRecv * kAMax * overlay_result.gamma_receive.size());
    while (out_predeliver[i].size() < kRecv * kAMax * overlay_result.gamma_receive.size()) {
      out_predeliver[i].emplace_back(MessageTuple::create_dummy());
    }
  }

  for (const auto &i : overlay_result.gamma_receive) { // deliver type
    ASSERT (out_deliver[i].size() <= kRecv * kAMax);
    out_deliver[i].reserve(kRecv * kAMax);
    while (out_deliver[i].size() < kRecv * kAMax) {
      out_deliver[i].emplace_back(MessageTuple::create_dummy());
    }
  }

  // encrypt and authenticate outgoing data
  std::vector<PeerInformation>
      all_i; // since there was a bug whose source I could not determine, we have to use vector instead of set here.. set simply didn't guarantee uniqueness
  add_peer_information_to_set_if_not_present(all_i, out_announce);
  add_peer_information_to_set_if_not_present(all_i, out_agreement);
  add_peer_information_to_set_if_not_present(all_i, out_inject);
  add_peer_information_to_set_if_not_present(all_i, out_routing);
  add_peer_information_to_set_if_not_present(all_i, out_predeliver);
  add_peer_information_to_set_if_not_present(all_i, out_deliver);
  add_peer_information_to_set_if_not_present(all_i, out_structure);

  std::vector<ReceiverBlobPair> i_c_pairs;
  for (auto &i : all_i) {
    // compute p_i
    std::vector<uint8_t> p_i_serialized;

    serialize_vec(p_i_serialized, out_announce[i]);
    serialize_vec(p_i_serialized, out_agreement[i]);
    serialize_vec(p_i_serialized, out_inject[i]);
    serialize_vec(p_i_serialized, out_routing[i]);
    serialize_vec(p_i_serialized, out_predeliver[i]);
    serialize_vec(p_i_serialized, out_deliver[i]);

    // compute aad_i
    auto aad_i = AadTuple{own_id_, i, cur_round_ + 1, out_structure[i]};
    std::vector<uint8_t> aad_i_serialized;
    aad_i.serialize(aad_i_serialized);

    // compute c_i
    i_c_pairs.emplace_back(ReceiverBlobPair{i, cryptlib::encrypt(sk_enc_, p_i_serialized, aad_i_serialized)});
  }

  std::vector<uint8_t> output;
  serialize_vec(output, i_c_pairs);

  // move all in[1] to in[0]
  in_structure_[0] = std::move(in_structure_[1]);
  in_structure_[1].clear();
  in_announce_[0] = std::move(in_announce_[1]);
  in_announce_[1].clear();
  in_agreement_[0] = std::move(in_agreement_[1]);
  in_agreement_[1].clear();
  in_inject_[0] = std::move(in_inject_[1]);
  in_inject_[1].clear();
  in_routing_[0] = std::move(in_routing_[1]);
  in_routing_[1].clear();
  in_predeliver_[0] = std::move(in_predeliver_[1]);
  in_predeliver_[1].clear();
  in_deliver_[0] = std::move(in_deliver_[1]);
  in_deliver_[1].clear();
  traffic_in_received_from_[0] = std::move(traffic_in_received_from_[1]);
  traffic_in_received_from_[1].clear();

  // actually return the output
  sgx_status_t ret = ocall_traffic_out_return(output.data(), output.size());
  if (ret != SGX_SUCCESS) {
    std::string error_text = "ocall traffic_out_return failed......\n";
    ocall_print_string(error_text.c_str());
    for (int idx = 0; idx < sizeof sgx_errlist / sizeof sgx_errlist[0]; idx++) {
      if (ret == sgx_errlist[idx].err) {
        ocall_print_string(sgx_errlist[idx].msg);
      }
    }
  }

  ocall_print_string("\n");

  return true;
}

void ClientEnclave::traffic_in(const uint8_t *ptr, size_t len) {
  if (!initialized_) {
    return;
  }

  // decrypt data
  std::vector<uint8_t> data;
  data.insert(data.end(), ptr, ptr + len);
  auto[p_decrypted, aad_decrypted] = cryptlib::decrypt(sk_enc_, data);
  if (p_decrypted.size() == 0 && aad_decrypted.size() == 0) {
    ocall_print_string("Decrypted message is empty!\n");
    return;
  }

  // deserialize aad
  size_t cur_aad = 0;
  auto aad = AadTuple::deserialize(aad_decrypted, cur_aad);

  // deserialize p
  size_t cur_p = 0;
  auto p_announce = deserialize_vec<MessageTuple>(p_decrypted, cur_p);
  auto p_agreement = deserialize_vec<AgreementTuple>(p_decrypted, cur_p);
  auto p_inject = deserialize_vec<MessageTuple>(p_decrypted, cur_p);
  auto p_routing = deserialize_vec<RoutingSchemeTuple>(p_decrypted, cur_p);
  auto p_predeliver = deserialize_vec<MessageTuple>(p_decrypted, cur_p);
  auto p_deliver = deserialize_vec<MessageTuple>(p_decrypted, cur_p);

  if (aad.receiver != own_id_) {
    ocall_print_string("Received a misguided message ...\n");
    return; // message was misguided
  }

  if (aad.round < cur_round_) {
    ocall_print_string("Received a message that was sent too late ...\n");
    return;
  }

  auto cur_or_next = aad.round - cur_round_; // compute whether in[0] or in[1] needs to be used
  ASSERT(cur_or_next < 2);

  if (traffic_in_received_from_[cur_or_next][aad.sender]) {
    ocall_print_string("Received a message a second time ...\n");
    // possible replay attack (message was already received)
    return;
  }

  traffic_in_received_from_[cur_or_next][aad.sender] = true;

  in_announce_[cur_or_next].insert(std::end(in_announce_[cur_or_next]), std::begin(p_announce), std::end(p_announce));
  in_agreement_[cur_or_next].insert(std::end(in_agreement_[cur_or_next]),
                                    std::begin(p_agreement),
                                    std::end(p_agreement));
  in_inject_[cur_or_next].insert(std::end(in_inject_[cur_or_next]), std::begin(p_inject), std::end(p_inject));
  in_routing_[cur_or_next].push_back(std::move(p_routing));
  in_predeliver_[cur_or_next].insert(std::end(in_predeliver_[cur_or_next]),
                                     std::begin(p_predeliver),
                                     std::end(p_predeliver));
  in_structure_[cur_or_next].insert(std::end(in_structure_[cur_or_next]),
                                    std::begin(aad.p_structure),
                                    std::end(aad.p_structure));
  in_deliver_[cur_or_next].insert(std::begin(p_deliver),
                                  std::end(p_deliver));

  // remove dummys from routing
  auto it = std::remove_if(in_routing_[cur_or_next].begin(), in_routing_[cur_or_next].end(),
                           [this](const std::vector<RoutingSchemeTuple> &routing_vec) {
                             return routing_vec.size() == 0;
                           });

  in_routing_[cur_or_next].erase(it, in_routing_[cur_or_next].end());

  // deliver message
  if (traffic_in_received_from_[cur_or_next].size() > m_corrupt_) {
    for (auto &message : in_deliver_[cur_or_next]) {
      if (!message.is_dummy()) {
        q_in_for_pseudonyms_.at(decrypt_pseudonym(message.n_dst).get_local_num()).push(std::move(message));
      }
    }
    // empty the sets - the if condition will never be fulfilled for this round again
    in_deliver_[cur_or_next].clear();
    traffic_in_received_from_[cur_or_next].clear();
  }
  ocall_print_string("\n");
}

uint64_t ClientEnclave::get_time() const {
  if (!initialized_) {
    return 0;
  }

  sgx_time_t current_time;
  sgx_time_source_nonce_t time_source_nonce;
  ASSERT(sgx_get_trusted_time(&current_time, &time_source_nonce) == SGX_SUCCESS);
  ASSERT(std::equal(std::begin(time_source_nonce), std::end(time_source_nonce), std::begin(init_time_nonce_)));
  return current_time - init_time_;
}

DecryptedPseudonym ClientEnclave::decrypt_pseudonym(const c1::client::Pseudonym &pseudonym) const {
  std::vector<uint8_t> pseud_vec(pseudonym.get().data(), pseudonym.get().data() + pseudonym.get().size());
  auto pseud_decr = cryptlib::decrypt(sk_pseud_, pseud_vec);

  size_t cur = 0;
  return DecryptedPseudonym::deserialize(pseud_decr.first, cur);
}

template<typename T>
std::vector<T> ClientEnclave::obtain_elements_that_exceed_m_corrupt(const std::vector<T> &vec) const {
  std::map<T, int> elems_u_counter;
  for (const auto &elem_t : vec) {
    if (elems_u_counter.count(elem_t) == 0) {
      elems_u_counter[elem_t] = 0;
    }
    elems_u_counter[elem_t]++;
  }

  std::vector<T> result;

  for (const auto &elem_u : elems_u_counter) {
    if (elem_u.second > m_corrupt_) {
      result.push_back(elem_u.first);
    }
  }

  return result;
}

template<typename T>
void ClientEnclave::add_peer_information_to_set_if_not_present(std::vector<PeerInformation> &result,
                                                               const std::map<PeerInformation, T> &map) const {
  for (const auto &elem : map) {
    if (std::find(result.begin(), result.end(), elem.first) != result.end()) {
      //ocall_print_string(("Trying to insert " + std::string(elem.first) + " again... I won't do this.\n").c_str());
    } else {
      result.push_back(elem.first);
    }
  }

}

} // ~namespace

#if defined(__cplusplus)
extern "C" {
#endif

void ecall_init() {
  c1::client::ClientEnclave::instance().init();
} ;

void ecall_network_init(uint8_t ip1, uint8_t ip2, uint8_t ip3, uint8_t ip4, uint64_t port) {
  c1::client::ClientEnclave::instance().network_init(ip1, ip2, ip3, ip4, port);
} ;

void ecall_received_msg_from_server(const uint8_t *ptr, size_t len) {
  c1::client::ClientEnclave::instance().received_msg_from_server(ptr, len);
}

void ecall_generate_pseudonym(uint8_t pseudonym[kPseudonymSize]) {
  c1::client::ClientEnclave::instance().generate_pseudonym(pseudonym);
}

void ecall_send_message(uint8_t n_src[kPseudonymSize],
                        uint8_t msg[kMessageSize],
                        uint8_t n_dst[kPseudonymSize],
                        uint64_t t_dst) {
  c1::client::ClientEnclave::instance().send_message(n_src, msg, n_dst, t_dst);
}

int ecall_receive_message(uint8_t n_dst[kPseudonymSize],
                          uint8_t msg[kMessageSize],
                          uint8_t n_src[kPseudonymSize],
                          uint64_t *t_dst) {
  return c1::client::ClientEnclave::instance().receive_message(n_dst, msg, n_src, t_dst);
}

int ecall_traffic_out() {
  return c1::client::ClientEnclave::instance().traffic_out();
}

void ecall_traffic_in(const uint8_t *ptr, size_t len) {
  c1::client::ClientEnclave::instance().traffic_in(ptr, len);
}

uint64_t ecall_get_time() {
  return c1::client::ClientEnclave::instance().get_time();
}

#if defined(__cplusplus)
}
#endif