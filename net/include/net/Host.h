#pragma once

#include "net/ByteBuffer.h"

#include <cstdint>
#include <string>
#include <unordered_map>

// Forward-declared so this public header never leaks <enet/enet.h>; ENet stays
// an implementation detail of net.
struct _ENetHost;
struct _ENetPeer;

namespace net {

// Library lifetime (wraps enet_initialize / enet_deinitialize). Call once each.
bool initialize();
void shutdown();

enum class EventType { None, Connect, Disconnect, Receive };

struct Event {
    EventType  type    = EventType::None;
    uint32_t   peerId  = 0; // stable id assigned by this Host
    uint8_t    channel = 0;
    ByteBuffer data;        // payload for Receive events
};

// Channel convention shared by client and server.
constexpr uint8_t kChannelReliable   = 0; // ordered: edits, inventory, chat, login, chunks
constexpr uint8_t kChannelUnreliable = 1; // sequenced: frequent entity snapshots
constexpr size_t  kChannelCount      = 2;

// Thin RAII wrapper over an ENetHost, usable as either a server or a client.
// Peers are referenced by a stable integer id rather than raw ENetPeer*.
class Host {
public:
    Host() = default;
    ~Host();
    Host(const Host&)            = delete;
    Host& operator=(const Host&) = delete;

    bool createServer(uint16_t port, size_t maxClients, size_t channels = kChannelCount);
    bool createClient(size_t channels = kChannelCount);

    // Begin connecting to a remote host; returns the peer id (0 on failure). The
    // matching Connect event arrives later from poll().
    uint32_t connect(const std::string& host, uint16_t port, size_t channels = kChannelCount);

    // Service the host. Returns true and fills `out` if an event occurred.
    bool poll(Event& out, uint32_t timeoutMs = 0);

    void send(uint32_t peerId, uint8_t channel, const ByteBuffer& buf, bool reliable);
    void broadcast(uint8_t channel, const ByteBuffer& buf, bool reliable);
    void disconnect(uint32_t peerId);
    void flush();

    bool   valid() const { return m_host != nullptr; }
    size_t peerCount() const { return m_peers.size(); }

private:
    _ENetHost* m_host      = nullptr;
    uint32_t   m_nextPeerId = 1;
    std::unordered_map<uint32_t, _ENetPeer*> m_peers;
};

} // namespace net
