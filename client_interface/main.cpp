
#include <zmq.hpp>
#include <iostream>
#include <regex>
#include "../include/shared_structs.h"

/**
 * The user inputs a pseudonym byte-wise with spaces ("1 2 3 4 5 255 0 72 ...").
 * This function generates a pseudonym from that input.
 * @param user_string
 * @return the pseudonym or {} if input could not be parsed
 */
std::optional<std::array<uint8_t, kPseudonymSize>> obtain_pseudonym_from_user_string(const std::string &user_string) {
  std::regex regex{"\\s+"};
  std::vector<std::string> container{
      std::sregex_token_iterator(user_string.begin(), user_string.end(), regex, -1),
      std::sregex_token_iterator()
  };

  if (container.size() != kPseudonymSize) {
    return {};
  }

  std::array<uint8_t, kPseudonymSize> result;

  for (int i = 0; i < kPseudonymSize; ++i) {
    result[i] = std::stoi(container[i]);
  }

  return result;
};

int main(int argc, char *argv[]) {
  using namespace c1;

  zmq::context_t context(1);

  zmq::socket_t socket_out(context, ZMQ_PUSH);
  socket_out.setsockopt(ZMQ_LINGER, 0);

  std::string port;
  std::cout << "Please enter the port number of the client: ";
  if (!std::getline(std::cin, port)) { return -1; }
  socket_out.connect("tcp://localhost:" + port);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
  while (true) {
    std::cout << "Please choose: \n(1) Create pseudonym \n(2) Send Message\nYour choice: ";
    std::string answer_str;
    std::getline(std::cin, answer_str);
    auto answer_int = std::stoi(answer_str);

    if (answer_int == 1) {
      zmq::message_t message(1);
      memset(message.data(), 0, 1);
      bool rc = socket_out.send(message);
      if (!rc) { return -1; };
    }
    if (answer_int == 2) {
      UserInterfaceMessageInjectionCommand injection_message;

      // n_src
      std::cout << "Please enter the source pseudonym (n_src): ";
      std::string pseud_src_string;
      std::getline(std::cin, pseud_src_string);
      auto pseud_src_vec = obtain_pseudonym_from_user_string(pseud_src_string);
      if (!pseud_src_vec.has_value()) {
        continue;
      }
      std::copy(std::begin(pseud_src_vec.value()), std::end(pseud_src_vec.value()), injection_message.n_src);

      // msg
      std::cout << "Please enter the message (msg): ";
      std::string message_string;
      std::getline(std::cin, message_string);
      if (message_string.size() == 0) {
        continue;
      }
      memcpy(injection_message.msg, message_string.c_str(), std::min(kMessageSize, message_string.length()));

      // n_dst
      std::cout << "Please enter the destination pseudonym (n_dst): ";
      std::string pseud_dst_string;
      std::getline(std::cin, pseud_dst_string);
      auto pseud_dst_vec = obtain_pseudonym_from_user_string(pseud_dst_string);
      if (!pseud_dst_vec.has_value()) {
        continue;
      }
      std::copy(std::begin(pseud_dst_vec.value()), std::end(pseud_dst_vec.value()), injection_message.n_dst);

      // t_dst
      std::cout << "Please enter the destination time (t_dst): ";
      std::string t_dst_string;
      std::getline(std::cin, t_dst_string);
      injection_message.t_dst = std::stoull(t_dst_string);

      // send message
      zmq::message_t message(sizeof(injection_message) + 1);
      static_cast<char *>(message.data())[0] = 1;
      memcpy(static_cast<char *>(message.data()) + 1, &injection_message, sizeof(injection_message));
      bool rc = socket_out.send(message);
      if (!rc) { return -1; };
    }


    /*
struct UserInterfaceMessageInjectionCommand {
 uint8_t n_src[kPseudonymSize];
 uint8_t msg[kMessageSize];
 uint8_t n_dst[kPseudonymSize];
 uint64_t t_dst;
}; */

  }
#pragma clang diagnostic pop

}