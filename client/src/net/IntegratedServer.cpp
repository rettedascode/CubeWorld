#include "net/IntegratedServer.h"

#include <ce/util/Log.h>

#include <future>
#include <utility>

namespace cw {

IntegratedServer::~IntegratedServer() { stop(); }

bool IntegratedServer::start(uint16_t port) {
    // All ENet host calls must happen on the server thread, so bind there too and
    // hand the result back via a promise — start() returns only once the server
    // is actually listening (or has failed), avoiding a connect-before-bind race.
    std::promise<bool> ready;
    auto future = ready.get_future();

    m_thread = std::thread([this, port, ready = std::move(ready)]() mutable {
        const bool ok = m_server.start(port);
        ready.set_value(ok);
        if (ok) m_server.run(); // blocks on this thread until stop()
    });

    const bool ok = future.get();
    if (ok) {
        CE_LOG_INFO("IntegratedServer: hosting on loopback port ", port);
    } else {
        if (m_thread.joinable()) m_thread.join();
        CE_LOG_ERROR("IntegratedServer: failed to start on port ", port);
    }
    return ok;
}

void IntegratedServer::stop() {
    if (!m_thread.joinable()) return;
    m_server.stop();
    m_thread.join();
}

} // namespace cw
