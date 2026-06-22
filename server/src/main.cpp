#include <cw/proto/Protocol.h>
#include <cw/server/Server.h>
#include <net/Host.h>

#include <cstdlib>
#include <iostream>

int main(int argc, char** argv) {
    std::cout << std::unitbuf; // flush logs immediately (and survive a hard stop)

    const uint16_t port = (argc > 1) ? static_cast<uint16_t>(std::atoi(argv[1]))
                                     : cw::kDefaultPort;

    if (!net::initialize()) {
        std::cerr << "[server] failed to initialize ENet\n";
        return 1;
    }

    cw::Server server;
    int code = 0;
    if (server.start(port)) {
        std::cout << "[server] running. Close this window (Ctrl+C) to stop.\n";
        server.run();
    } else {
        code = 1;
    }

    net::shutdown();
    return code;
}
