//
// Created by c1 on 18.01.18.
//

#include "client.h"

#include <unistd.h>
#include <pwd.h>
#include <iostream>

#include "enclave_u.h"
#include "sgx_urts.h"
#include "../../include/errors.h"

# define TOKEN_FILENAME   "enclave_client.token"
# define ENCLAVE_FILENAME "enclave_client.signed.so"
# define MAX_PATH FILENAME_MAX

#ifndef TRUE
# define TRUE 1
#endif

#ifndef FALSE
# define FALSE 0
#endif

namespace c1::client {

Client::Client() : global_eid_(0), network_manager_() {}

/* Initialize the enclave:
 *   Step 1: try to retrieve the launch token saved by last transaction
 *   Step 2: call sgx_create_enclave to initialize an enclave instance
 *   Step 3: save the launch token if it is updated
 */
int Client::initialize_enclave() {
  char token_path[MAX_PATH] = {'\0'};
  sgx_launch_token_t token = {0};
  sgx_status_t ret; //= SGX_ERROR_UNEXPECTED;
  int updated = 0;

  /* Step 1: try to retrieve the launch token saved by last transaction
   *         if there is no token, then create a new one.
   */
  /* try to get the token saved in $HOME */
  const char *home_dir = getpwuid(getuid())->pw_dir;

  if (home_dir != nullptr &&
      (strlen(home_dir) + strlen("/") + sizeof(TOKEN_FILENAME) + 1) <= MAX_PATH) {
    /* compose the token path */
    strncpy(token_path, home_dir, strlen(home_dir));
    strncat(token_path, "/", strlen("/"));
    strncat(token_path, TOKEN_FILENAME, sizeof(TOKEN_FILENAME) + 1);
  } else {
    /* if token path is too long or $HOME is NULL */
    strncpy(token_path, TOKEN_FILENAME, sizeof(TOKEN_FILENAME));
  }

  FILE *fp = fopen(token_path, "rb");
  if (fp == nullptr && (fp = fopen(token_path, "wb")) == nullptr) {
    printf("Warning: Failed to create/open the launch token file \"%s\".\n", token_path);
  }

  if (fp != nullptr) {
    /* read the token from saved file */
    size_t read_num = fread(token, 1, sizeof(sgx_launch_token_t), fp);
    if (read_num != 0 && read_num != sizeof(sgx_launch_token_t)) {
      /* if token is invalid, clear the buffer */
      memset(&token, 0x0, sizeof(sgx_launch_token_t));
      printf("Warning: Invalid launch token read from \"%s\".\n", token_path);
    }
  }
  /* Step 2: call sgx_create_enclave to initialize an enclave instance */
  /* Debug Support: set 2nd parameter to 1 */
  ret = sgx_create_enclave(ENCLAVE_FILENAME, SGX_DEBUG_FLAG, &token, &updated, &global_eid_, nullptr);
  if (ret != SGX_SUCCESS) {
    print_error_message(ret);
    if (fp != nullptr) fclose(fp);
    return -1;
  }

  /* Step 3: save the launch token if it is updated */
  if (updated == FALSE || fp == nullptr) {
    /* if the token is not updated, or file handler is invalid, do not perform saving */
    if (fp != nullptr) fclose(fp);
    return 0;
  }

  /* reopen the file with write capability */
  fp = freopen(token_path, "wb", fp);
  if (fp == nullptr) return 0;
  size_t write_num = fwrite(token, 1, sizeof(sgx_launch_token_t), fp);
  if (write_num != sizeof(sgx_launch_token_t))
    printf("Warning: Failed to save launch token to \"%s\".\n", token_path);
  fclose(fp);
  return 0;
}

int Client::run() {
  /* Initialize the enclave */
  if (initialize_enclave() < 0) {
    printf("Enter a character before exit ...\n");
    getchar();
    throw std::runtime_error("");
  }

  ecall_init(global_eid_);

  /* Inform the network manager of the global_eid_ */
  network_manager_.set_global_sgx_eid_and_network_init(global_eid_);

  //main loop
  for (auto start = std::chrono::system_clock::now(); true;) { // infinite (main!) loop
    if (!network_manager_.MainLoop()) {
      break;
    }
    if (!network_manager_.isInitialized()) {
      continue; // no response from the server yet, so don't call traffic_out yet
    }

    std::chrono::duration<double> time_diff = std::chrono::system_clock::now() - start;
    if (time_diff.count() >= static_cast<double>(kDelta) / 2.0) {
      int ret_val;
      ecall_traffic_out(global_eid_, &ret_val);
      start = std::chrono::system_clock::now();
    }

    { //try to receive messages
      int retval;
      UserInterfaceMessageInjectionCommand msg; // slight abuse of notation
      ecall_receive_message(global_eid_, &retval, msg.n_dst, msg.msg, msg.n_src, &msg.t_dst);
      if (retval) {
        std::cout << "Received message: " << reinterpret_cast<char *>(msg.msg) << std::endl;
      }
    }

  }

  /* Destroy the enclave */
  sgx_destroy_enclave(global_eid_);

  printf("Info: SampleEnclave successfully returned.\n");

//    printf("Enter a character before exit ...\n");
//    getchar();
  return 0;
}

void Client::send_msg_to_server(const void *ptr, size_t len) {
  network_manager_.send_msg_to_server(ptr, len);
}

void Client::traffic_out_return(const uint8_t *ptr, size_t len) {
  size_t cur = 0;

  std::vector<uint8_t> working_vec(ptr, ptr + len);

  auto i_c_pairs = deserialize_vec<ReceiverBlobPair>(working_vec, cur);
  for (auto &i_c : i_c_pairs) {
    network_manager_.send_msg_to_peer(i_c.i, i_c.payload.data(), i_c.payload.size());
  }

}

} //~namespace



/* OCall functions sfbsfdf*/
void ocall_print_string(const char *str) {
  /* Proxy/Bridge will check the length and null-terminate
   * the input string to prevent buffer overflow.
   */
  printf("%s", str);
}

void ocall_send_msg_to_server(const uint8_t *ptr, size_t len) {
  //std::cout << "Called send_msg_to_server (outside class)" << std::endl;
  c1::client::Client::instance().send_msg_to_server(ptr, len);
}

void ocall_traffic_out_return(const uint8_t *ptr, size_t len) {
  c1::client::Client::instance().traffic_out_return(ptr, len);
}
