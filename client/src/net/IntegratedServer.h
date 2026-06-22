#pragma once

#include <cw/server/Server.h>

#include <cstdint>
#include <thread>

namespace cw {

// CLIENT-side helper that hosts an in-process authoritative Server on a
// background thread, so singleplayer uses the EXACT same networked path as
// multiplayer (the client always connects over loopback via NetClient).
//
// Threading: the sim still ticks single-threaded — only this background thread
// ever touches the Server/Simulation. The client thread communicates with it
// solely through ENet loopback, so there is no shared mutable state to race on.
class IntegratedServer {
public:
    ~IntegratedServer();

    bool start(uint16_t port); // blocks until the server has bound; returns success
    void stop();
    bool running() const { return m_thread.joinable(); }

private:
    Server      m_server;
    std::thread m_thread;
};

} // namespace cw
