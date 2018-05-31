#include "client.h"

int main(int argc, char *argv[]) {
  using namespace c1::client;
  return Client::instance().run();
}