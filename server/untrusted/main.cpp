#include "server.h"


int main(int argc, char *argv[]) {
    using namespace c1::server;
    return Server::instance().run();
}