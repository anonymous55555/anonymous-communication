// This file contains structs shared between the enclaves of different entities in the system.

#ifndef NETWORK_SGX_EXAMPLE_SHARED_STRUCTS_H
#define NETWORK_SGX_EXAMPLE_SHARED_STRUCTS_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstring>
#include <cassert>
#include <sgx_tcrypto.h>
#include <array>
#include "serialization.h"
#include "config.h"

namespace c1 {

enum MsgTypes { kTypeJoinMessage, kTypeInitMessage };

/**
 * An URI consisting of an IPv4 address (to be accessed byte-wise via ip1, ..., ip4) and a port number.
 */
struct Uri : Serializable {
  uint8_t ip1;
  uint8_t ip2;
  uint8_t ip3;
  uint8_t ip4;
  uint64_t port;

  Uri() {}

  Uri(uint8_t ip1, uint8_t ip2, uint8_t ip3, uint8_t ip4, uint64_t port)
      : ip1(ip1), ip2(ip2), ip3(ip3), ip4(ip4), port(port) {}

  bool operator==(const Uri &rhs) const {
    return std::tie(ip1, ip2, ip3, ip4, port) == std::tie(rhs.ip1, rhs.ip2, rhs.ip3, rhs.ip4, rhs.port);
  }
  bool operator!=(const Uri &rhs) const {
    return !(rhs == *this);
  }

  bool operator<(const Uri &rhs) const {
    return std::tie(ip1, ip2, ip3, ip4, port)
        < std::tie(rhs.ip1, rhs.ip2, rhs.ip3, rhs.ip4, rhs.port);
  }
  bool operator>(const Uri &rhs) const {
    return rhs < *this;
  }
  bool operator<=(const Uri &rhs) const {
    return !(rhs < *this);
  }
  bool operator>=(const Uri &rhs) const {
    return !(*this < rhs);
  }

  void serialize(std::vector<uint8_t> &working_vec) const override {
    serialize_number(working_vec, ip1);
    serialize_number(working_vec, ip2);
    serialize_number(working_vec, ip3);
    serialize_number(working_vec, ip4);
    serialize_number(working_vec, port);
  }

  static Uri deserialize(const std::vector<uint8_t> &working_vec, size_t &cur) {
    Uri result;
    result.ip1 = deserialize_number<uint8_t>(working_vec, cur);
    result.ip2 = deserialize_number<uint8_t>(working_vec, cur);
    result.ip3 = deserialize_number<uint8_t>(working_vec, cur);
    result.ip4 = deserialize_number<uint8_t>(working_vec, cur);
    result.port = deserialize_number<uint64_t>(working_vec, cur);
    return result;
  }

  inline operator std::string const() const {
    std::string result = "";
    result += std::to_string(ip1) + "." + std::to_string(ip2) + "." + std::to_string(ip3) + "." + std::to_string(ip4);
    result += ":" + std::to_string(port);
    return result;
  }

};

/**
 * Basic structure holding the id of a peer and the uri of its socket.
 */
struct PeerInformation : public Serializable {
  uint64_t id;
  Uri uri;

  PeerInformation() : id(0), uri() {}
  PeerInformation(uint64_t id, const Uri &uri) : id(id), uri(uri) {}

  inline bool operator==(const PeerInformation &o) const { return id == o.id && uri == o.uri; }
  inline bool operator!=(const PeerInformation &o) const { return !operator==(o); }
  inline bool operator<(const PeerInformation &rhs) const { return std::tie(id, uri) < std::tie(rhs.id, rhs.uri); }
  inline bool operator>(const PeerInformation &rhs) const { return rhs < *this; }
  inline bool operator<=(const PeerInformation &rhs) const { return !(rhs < *this); }
  inline bool operator>=(const PeerInformation &rhs) const { return !(*this < rhs); }

  void serialize(std::vector<uint8_t> &working_vec) const override {
    serialize_number(working_vec, id);
    uri.serialize(working_vec);
  }

  static PeerInformation deserialize(const std::vector<uint8_t> &working_vec, size_t &cur) {
    PeerInformation result;
    result.id = deserialize_number<uint64_t>(working_vec, cur);
    result.uri = Uri::deserialize(working_vec, cur);
    return result;
  }

  inline operator std::string const() const {
    std::string result = "";
    result += "Id: " + std::to_string(id) + ", ";
    result += "Uri: " + std::string(uri);
    return result;
  }
};

/**
 * Contains a PeerInformation and a blob of data (used to encode the (i,c)-Pairs of trafficOut()).
 */
class ReceiverBlobPair : public Serializable {
 public:

  PeerInformation i;
  std::vector<uint8_t> payload;

  ReceiverBlobPair(const PeerInformation &i, const std::vector<uint8_t> &payload) : i(i), payload(payload) {}

  void serialize(std::vector<uint8_t> &working_vec) const override {
    i.serialize(working_vec);

    serialize_number(working_vec, payload.size());
    working_vec.insert(std::end(working_vec), std::begin(payload), std::end(payload));
  }

  static ReceiverBlobPair deserialize(const std::vector<uint8_t> &working_vec, size_t &cur) {
    auto i = PeerInformation::deserialize(working_vec, cur);
    auto count = deserialize_number<size_t>(working_vec, cur);

    std::vector<uint8_t> payload;
    payload.reserve(count);
    for (int i = 0; i < count; ++i) {
      payload.push_back(deserialize_number<uint8_t>(working_vec, cur));
    }

    return ReceiverBlobPair(i, payload);
  }
};

/**
 * Returns the message type of a properly serialized message (where the type is encoded at the first byte).
 * @param ptr pointer to the message
 * @return
 */
inline MsgTypes get_message_type(const void *ptr) {
  return static_cast<MsgTypes>(reinterpret_cast<const uint64_t *>(ptr)[0]);
}

/**
 * Message used to register at the login server.
 */
struct JoinMessage {
  uint64_t msg_type = kTypeJoinMessage; //used to simplify serialization / deserialization
  //remark: this only works as long as we have a standard-layout class (in particular, all members need to have the same access control!)
  PeerInformation sender;

  explicit JoinMessage(const PeerInformation sender)
      : sender(sender) {};

  inline std::string to_string() const {
    return "\t" + std::string(sender) + "\t (sender) \n";
  }
};

template<typename T>
size_t size_of_vec(const typename std::vector<T> &vec) {
  return sizeof(T) * vec.size();
}

template<typename T, typename U>
size_t size_of_map_keys(const typename std::map<T, U> &map) {
  return sizeof(T) * map.size();
}

/**
 * Copies the contents of the vector into the memory location pointed to by ptr
 * @tparam T type of the vector
 * @param ptr beginning of the location where the contents should be copied to
 * @param vec vector to be copied
 * @return memory location after the contents
 */
template<typename T>
char *copy_vector(char *ptr, std::vector<T> &vec) {
  std::copy(vec.begin(), vec.end(), reinterpret_cast<T *>(ptr));
  return ptr + size_of_vec(vec);
};


template<typename T>
const char *obtain_vector(const char *ptr, size_t size, std::vector<T> &vec) {
  vec = std::vector<T>(reinterpret_cast<const T *>(ptr), reinterpret_cast<const T *>(ptr + size * sizeof(T)));
  return ptr + size * sizeof(T);
};

/**
 * InitMessage sent from the login server to the peers to supply them with the init data.
 */
class InitMessage {
  uint64_t receiver_id_;
  uint64_t num_total_nodes_;
  uint64_t overlay_dimension_;
  uint64_t onid_assoc_;
  uint64_t onid_emul_;
  std::vector<PeerInformation> gamma_send_;
  std::vector<PeerInformation> gamma_receive_;
  std::map<uint64_t, std::vector<PeerInformation>> gamma_route_;
  std::array<uint8_t, SGX_AESGCM_KEY_SIZE> sk_pseud_;
  std::array<uint8_t, SGX_AESGCM_KEY_SIZE> sk_enc_;
  std::array<uint8_t, SGX_CMAC_KEY_SIZE> sk_routing_;

  //Fields containing keys in SGX API form
  sgx_aes_gcm_128bit_key_t sk_pseud_sgx_;
  sgx_aes_gcm_128bit_key_t sk_enc_sgx_;
  sgx_cmac_128bit_key_t sk_routing_sgx_;

 public:
  uint64_t get_receiver_id_() const {
    return receiver_id_;
  }
  uint64_t get_num_total_nodes_() const {
    return num_total_nodes_;
  }

  uint64_t get_overlay_dimension_() const {
    return overlay_dimension_;
  }

  uint64_t get_onid_assoc_() const {
    return onid_assoc_;
  }

  uint64_t get_onid_emul_() const {
    return onid_emul_;
  }

  const std::vector<PeerInformation> &get_gamma_send_() const {
    return gamma_send_;
  }

  const std::vector<PeerInformation> &get_gamma_receive_() const {
    return gamma_receive_;
  }

  const std::map<uint64_t, std::vector<PeerInformation>> &get_gamma_route_() const {
    return gamma_route_;
  }

  sgx_aes_gcm_128bit_key_t &get_sk_pseud_() {
    return sk_pseud_sgx_;
  }

  sgx_aes_gcm_128bit_key_t &get_sk_enc_() {
    return sk_enc_sgx_;
  }

  sgx_cmac_128bit_key_t &get_sk_routing_() {
    return sk_routing_sgx_;
  }

  InitMessage(uint64_t receiver_id_,
              uint64_t num_total_nodes_,
              uint64_t num_quorum_nodes_,
              uint64_t onid_assoc_,
              uint64_t onid_emul_,
              const std::vector<PeerInformation> &gamma_send_, const std::vector<PeerInformation> &gamma_receive_,
              const std::map<uint64_t, std::vector<PeerInformation>> &gamma_route_,
              const std::array<uint8_t, SGX_AESGCM_KEY_SIZE> sk_pseud_, const std::array<uint8_t, SGX_AESGCM_KEY_SIZE> sk_enc_, const std::array<uint8_t, SGX_CMAC_KEY_SIZE> sk_routing_)
      : receiver_id_(receiver_id_), num_total_nodes_(num_total_nodes_),
        overlay_dimension_(num_quorum_nodes_),
        onid_assoc_(onid_assoc_), onid_emul_(onid_emul_),
        gamma_send_(gamma_send_),
        gamma_receive_(gamma_receive_),
        gamma_route_(gamma_route_),
        sk_pseud_(sk_pseud_), sk_enc_(sk_enc_), sk_routing_(sk_routing_){
    copy_crypto_keys();
  }

 private:

  struct InitMessageLowLevelHeader {
    uint64_t msg_type;
    uint64_t receiver_id_;
    uint64_t num_total_nodes_;
    uint64_t overlay_dimension_;
    uint64_t onid_assoc;
    uint64_t onid_emul;
    size_t gamma_send_entry_count;
    size_t gamma_receive_entry_count;
    size_t gamma_route_entry_count;
  };

  void copy_crypto_keys() {
    memcpy(sk_pseud_sgx_, sk_pseud_.data(), sk_pseud_.size());
    memcpy(sk_enc_sgx_, sk_enc_.data(), sk_enc_.size());
    memcpy(sk_routing_sgx_, sk_routing_.data(), sk_routing_.size());
  }

 public:
  /**
   * Serialization constructor.
   * @param ptr pointer to the serialized object
   */
  InitMessage(const char *ptr) {
    InitMessageLowLevelHeader low_level_header;
    memcpy(&low_level_header, ptr, sizeof(low_level_header));
    ptr += sizeof(low_level_header);

    memcpy(sk_pseud_.data(), ptr, SGX_AESGCM_KEY_SIZE);
    ptr += SGX_AESGCM_KEY_SIZE;
    memcpy(sk_enc_.data(), ptr, SGX_AESGCM_KEY_SIZE);
    ptr += SGX_AESGCM_KEY_SIZE;
    memcpy(sk_routing_.data(), ptr, SGX_CMAC_KEY_SIZE);
    ptr += SGX_CMAC_KEY_SIZE;
    copy_crypto_keys();

    receiver_id_ = low_level_header.receiver_id_;
    num_total_nodes_ = low_level_header.num_total_nodes_;
    overlay_dimension_ = low_level_header.overlay_dimension_;
    onid_assoc_ = low_level_header.onid_assoc;
    onid_emul_ = low_level_header.onid_emul;

    //obtain gamma_send
    ptr = obtain_vector(ptr, low_level_header.gamma_send_entry_count, gamma_send_);
    ptr = obtain_vector(ptr, low_level_header.gamma_receive_entry_count, gamma_receive_);
    for (int i = 0; i < low_level_header.gamma_route_entry_count; ++i) {
      uint64_t key = *reinterpret_cast<const uint64_t *>(ptr);
      gamma_route_[key] = std::vector<PeerInformation>();
      ptr += sizeof(uint64_t); // this might be improved by using a different pointer type

      size_t num_entries = *reinterpret_cast<const size_t *>(ptr);
      ptr += sizeof(size_t);
      ptr = obtain_vector(ptr, num_entries, gamma_route_[key]);
    }

  }

  /**
   * serializes the object in memory
   * @return pointer to the serialized object and the size of it
   */
  std::pair<std::shared_ptr<char>, int> serialize() {
    //compute required memory space:
    int size = sizeof(InitMessageLowLevelHeader);
    size += SGX_AESGCM_KEY_SIZE * 2;
    size += SGX_CMAC_KEY_SIZE;
    size += size_of_vec(gamma_send_);
    size += size_of_vec(gamma_receive_);
    size += size_of_map_keys(gamma_route_);
    int gamma_route_entries_size = 0;
    for (auto vec: gamma_route_) {
      gamma_route_entries_size +=
          sizeof(size_t) + size_of_vec(vec.second); // used to store the entry counts and the vectors
    }
    size += gamma_route_entries_size;

    // initialize the low level header and insert the right values:
    InitMessageLowLevelHeader low_level_header;
    low_level_header.msg_type = kTypeInitMessage;
    low_level_header.receiver_id_ = receiver_id_;
    low_level_header.num_total_nodes_ = num_total_nodes_;
    low_level_header.overlay_dimension_ = overlay_dimension_;
    low_level_header.onid_assoc = onid_assoc_;
    low_level_header.onid_emul = onid_emul_;
    low_level_header.gamma_send_entry_count = gamma_send_.size();
    low_level_header.gamma_receive_entry_count = gamma_receive_.size();
    low_level_header.gamma_route_entry_count = gamma_route_.size();

    //auto serialized_object = new char[size];
    std::shared_ptr<char> serialized_object(new char[size], std::default_delete<char[]>());
    auto myptr = serialized_object.get();
    // store low_level_header
    memcpy(myptr, &low_level_header, sizeof(low_level_header));
    myptr += sizeof(low_level_header);
    // store crypto keys
    memcpy(myptr, &sk_pseud_, SGX_AESGCM_KEY_SIZE);
    myptr += SGX_AESGCM_KEY_SIZE;
    memcpy(myptr, &sk_enc_, SGX_AESGCM_KEY_SIZE);
    myptr += SGX_AESGCM_KEY_SIZE;
    memcpy(myptr, &sk_routing_, SGX_CMAC_KEY_SIZE);
    myptr += SGX_CMAC_KEY_SIZE;
    // store gamma_send
    myptr = copy_vector(myptr, gamma_send_);
    // store gamma_receive
    myptr = copy_vector(myptr, gamma_receive_);
    // store gamma_route keys:
    auto myptr2 = myptr;
    for (auto elem : gamma_route_) {
      //for(auto myptr2 = myptr; myptr2 < myptr + size_of_map_keys(gamma_route_); ) {
      *reinterpret_cast<uint64_t *>(myptr2) = elem.first;
      myptr2 += sizeof(uint64_t);
      *reinterpret_cast<size_t *>(myptr2) = elem.second.size();
      myptr2 += sizeof(size_t);
      myptr2 = copy_vector(myptr2, elem.second);

    }
    assert(myptr2 = myptr + size_of_map_keys(gamma_route_) + gamma_route_entries_size);

    return std::pair<std::shared_ptr<char>, int>{serialized_object, size};
  }

};

/**
 * Used by the peer interface to obtain the relevant information for a sendMessage.
 */
struct UserInterfaceMessageInjectionCommand {
  uint8_t n_src[kPseudonymSize];
  uint8_t msg[kMessageSize];
  uint8_t n_dst[kPseudonymSize];
  uint64_t t_dst;
};

}

#endif //NETWORK_SGX_EXAMPLE_SHARED_STRUCTS_H
