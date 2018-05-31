Requirements:
  * zeromq (http://zeromq.org/)
  * cppzmq (https://github.com/zeromq/cppzmq)
  * boost test (https://www.boost.org/doc/libs/1_67_0/libs/test/doc/html/index.html)
  * the intel sgx sdk (https://github.com/intel/linux-sgx)
  * g++ >= 7.2

Howto:
  * use cmake to build
  * run login_server (the login server)
  * start 81 clients
  * use the client\_interface binary for user input to the clients (generate_pseudonym, etc.)


Known Limitations:
  * currently hardcoded for n = 81 nodes (overlay network with 8 (quorum) nodes, i.e., dimension 3)
  * as for now, all processes run on the same node only (localhost is hard-coded)
  * has been tested in the Intel SGX Simulation Mode only
