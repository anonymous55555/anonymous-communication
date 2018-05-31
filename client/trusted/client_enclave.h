#ifndef _CLIENTENCLAVE_H_
#define _CLIENTENCLAVE_H_


#include <string>
#include <queue>
#include <set>
#include <sgx_tcrypto.h>
#include "../../include/shared_structs.h"
#include "overlay_structure_scheme.h"
#include "structures.h"

namespace c1::client {

/**
 * THE main class of the peer enclave
 */
class ClientEnclave {
 private:
  /**
   * Constructor. Not to be called directly (thus private). Use instance() instead.
   */
  ClientEnclave() : overlay_structure_scheme_(), cur_round_(static_cast<round_t>(-1)) {
  }

 public:
  /**
   * Returns the client singleton.
   * @return
   */
  static ClientEnclave &instance() {
    static ClientEnclave INSTANCE;
    return INSTANCE;
  }

  /**
   * Used to set up some variables.
   */
  void init();
  /**
   * Called once the network is ready (and ip address and port of the incoming socket are known)
   * @param ip1 first byte of the ipv4 address
   * @param ip2 second byte of the ipv4 address
   * @param ip3 third byte of the ipv4 address
   * @param ip4 fourth byte of the ipv4 address
   * @param port port of the incoming socket
   */
  void network_init(uint8_t ip1, uint8_t ip2, uint8_t ip3, uint8_t ip4, uint64_t port);
  /**
   * Called when a message from the login_server has arrived.
   * @param msg
   * @param msg_len
   */
  void received_msg_from_server(const void *msg, size_t msg_len);

 private:
  /** Whether the login server has initiated the whole anonymous system yet */
  bool initialized_ = false;
  /** Time of initialization */
  sgx_time_t init_time_;
  /** Init time nonce */
  sgx_time_source_nonce_t init_time_nonce_;
  /** Dimension of the overlay */
  uint64_t overlay_dimension_;

  /**
   * Send a message to the login server
   * @param msg_raw
   * @param msg_len
   * @param msg_desc a description of this message for error messages (in case the sending has failed)
   */
  void try_and_send_msg_to_server(void *msg_raw, int msg_len, std::string msg_desc) const;

 public:
  /**
   * see paper
   * @param pseudonym
   */
  void generate_pseudonym(uint8_t pseudonym[kPseudonymSize]);
  /**
   * see paper
   * @param n_src
   * @param msg
   * @param n_dst
   * @param t_dst
   */
  void send_message(uint8_t n_src[kPseudonymSize],
                    uint8_t msg[kMessageSize],
                    uint8_t n_dst[kPseudonymSize],
                    uint64_t t_dst);
  /**
   * see paper
   * @param n_dst
   * @param msg
   * @param n_src
   * @param t_dst
   * @return
   */
  int receive_message(uint8_t *n_dst,
                      uint8_t *msg,
                      uint8_t *n_src,
                      uint64_t *t_dst);
  /**
   * see paper
   * @return true iff this call of traffic_out was not superfluous (it is necessary only once every round) and ran correctly
   */
  int traffic_out();
  /**
   * see paper
   * @param ptr
   * @param len
   */
  void traffic_in(const uint8_t *ptr, size_t len);
  /**
   * retrieves the current time, relative to the initialization time
   * @return
   */
  uint64_t get_time() const;

 private:
  /** see paper */
  size_t m_corrupt_ = 1;
  /** see paper */
  size_t max_quorum_size_;
  /** see paper */
  size_t max_routing_msg_out_;
  /** see paper */
  onid_t onid_repr_;
  /** actually, this is stored to have access to previous round's value */
  onid_t onid_emul_;
  /** the pi of the peeritself */
  PeerInformation own_id_;
  /** see paper */
  sgx_aes_gcm_128bit_key_t sk_pseud_;
  /** see paper */
  sgx_aes_gcm_128bit_key_t sk_enc_;
  /** see paper */
  sgx_cmac_128bit_key_t sk_routing_;
  /** set q_in (see paper), one for each (of the local) pseudonym(s) */
  std::vector<std::priority_queue<MessageTuple, std::vector<MessageTuple>, std::greater<MessageTuple>>>
      q_in_for_pseudonyms_;
  /** see paper */
  std::priority_queue<MessageTuple, std::vector<MessageTuple>, std::greater<MessageTuple>> q_out_;
  /** used to count the messages sent from each of the local pseudonyms for each round (to check if we respected the k_send limit) */
  std::vector<std::map<uint64_t, int>>
      num_q_out_entries_for_round_for_pseudonym_;
  /** see paper (called N there) */
  std::vector<DecryptedPseudonym> pseudonyms_;
  /** basically, this is st_overlay */
  OverlayStructureScheme overlay_structure_scheme_;
  /** set in for this tag - twice because we may receive messages for the next round and the round after (due to delay) */
  std::array<std::vector<OverlayStructureSchemeMessage>, 2>
      in_structure_;
  /** set in for this tag - twice because we may receive messages for the next round and the round after (due to delay) */
  std::array<std::vector<MessageTuple>, 2> in_announce_;
  /** set in for this tag - twice because we may receive messages for the next round and the round after (due to delay) */
  std::array<std::vector<AgreementTuple>, 2> in_agreement_;
  /** set in for this tag - twice because we may receive messages for the next round and the round after (due to delay) */
  std::array<std::vector<MessageTuple>, 2> in_inject_;
  /** set in for this tag - twice because we may receive messages for the next round and the round after (due to delay) */
  std::array<std::vector<std::vector<RoutingSchemeTuple>>, 2>
      in_routing_;
  /** set in for this tag - twice because we may receive messages for the next round and the round after (due to delay) */
  std::array<std::vector<MessageTuple>, 2> in_predeliver_;
  /** set in for this tag - twice because we may receive messages for the next round and the round after (due to delay) */
  std::array<std::set<MessageTuple>, 2> in_deliver_;
  /** the tuples to be stored for messages whose agreement protocol this TEE is currently part of */
  std::vector<AgreementLocalTuple> agreement_tuples_;
  /** see paper (called l_now there) */
  round_t cur_round_;
  /** used to store gamma_{agree, l}, where entry 0 corresponds to l_now and entry l corresponds to l_now - l */
  std::deque<std::vector<PeerInformation>>
      gamma_agree_for_round_;
  /** Used to ignore messages sent twice (to prevent replay attacks) */
  std::array<std::map<PeerInformation, bool>, 2> traffic_in_received_from_;

  /**
   * Decrypt a pseudonym to obtain the id of the node with that pseudonym and the onid of its associated quorum
   * @param pseudonym
   * @return
   */
  DecryptedPseudonym decrypt_pseudonym(const Pseudonym &pseudonym) const;

  /**
   * For a given vector v of elements of type T, return those elements that occur more than m_corrupt times in v
   * @tparam T Type of the elements in the vector
   * @param vec The input vector
   * @return a vector containing only the desired elements (and only once)
   */
  template<typename T>
  std::vector<T> obtain_elements_that_exceed_m_corrupt(const std::vector<T> &vec) const;
  template<typename T>
  /**
   * Traverses map and adds all keys to result that aren't in there already (used to be a set, but there seems to be a bug in the stdlib causing elements not to be unique)
   * @tparam T
   * @param result
   * @param map
   */
  void add_peer_information_to_set_if_not_present(std::vector<PeerInformation>
                                                  &result,
                                                  const std::map<PeerInformation, T> &map
  ) const;
};

} // !namespace



#endif /* !_CLIENTENCLAVE_H_ */
