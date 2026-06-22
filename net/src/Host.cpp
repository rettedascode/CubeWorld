#include "net/Host.h"

#include <enet/enet.h>

namespace net {

bool initialize() { return enet_initialize() == 0; }
void shutdown() { enet_deinitialize(); }

namespace {
// Stash/fetch our stable peer id inside ENetPeer::data.
void setPeerId(ENetPeer* peer, uint32_t id) {
    peer->data = reinterpret_cast<void*>(static_cast<uintptr_t>(id));
}
uint32_t peerId(ENetPeer* peer) {
    return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(peer->data));
}
} // namespace

Host::~Host() {
    if (m_host) enet_host_destroy(m_host);
}

bool Host::createServer(uint16_t port, size_t maxClients, size_t channels) {
    ENetAddress addr;
    addr.host = ENET_HOST_ANY;
    addr.port = port;
    m_host = enet_host_create(&addr, maxClients, channels, 0, 0);
    return m_host != nullptr;
}

bool Host::createClient(size_t channels) {
    m_host = enet_host_create(nullptr, 1, channels, 0, 0);
    return m_host != nullptr;
}

uint32_t Host::connect(const std::string& host, uint16_t port, size_t channels) {
    if (!m_host) return 0;
    ENetAddress addr;
    enet_address_set_host(&addr, host.c_str());
    addr.port = port;

    ENetPeer* peer = enet_host_connect(m_host, &addr, channels, 0);
    if (!peer) return 0;

    const uint32_t id = m_nextPeerId++;
    setPeerId(peer, id);
    m_peers[id] = peer;
    return id;
}

bool Host::poll(Event& out, uint32_t timeoutMs) {
    if (!m_host) return false;

    ENetEvent ev;
    if (enet_host_service(m_host, &ev, timeoutMs) <= 0) return false;

    switch (ev.type) {
        case ENET_EVENT_TYPE_CONNECT: {
            uint32_t id = peerId(ev.peer);
            if (id == 0) { // inbound connection (server side): assign a fresh id
                id = m_nextPeerId++;
                setPeerId(ev.peer, id);
                m_peers[id] = ev.peer;
            }
            out.type   = EventType::Connect;
            out.peerId = id;
            return true;
        }
        case ENET_EVENT_TYPE_RECEIVE: {
            out.type    = EventType::Receive;
            out.peerId  = peerId(ev.peer);
            out.channel = ev.channelID;
            out.data    = ByteBuffer(ev.packet->data, ev.packet->dataLength);
            enet_packet_destroy(ev.packet);
            return true;
        }
        case ENET_EVENT_TYPE_DISCONNECT: {
            out.type   = EventType::Disconnect;
            out.peerId = peerId(ev.peer);
            m_peers.erase(out.peerId);
            ev.peer->data = nullptr;
            return true;
        }
        default:
            return false;
    }
}

void Host::send(uint32_t id, uint8_t channel, const ByteBuffer& buf, bool reliable) {
    auto it = m_peers.find(id);
    if (it == m_peers.end()) return;
    // No flags => unreliable but sequenced per channel (ENet's default).
    ENetPacket* packet = enet_packet_create(
        buf.data(), buf.size(), reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
    enet_peer_send(it->second, channel, packet);
}

void Host::broadcast(uint8_t channel, const ByteBuffer& buf, bool reliable) {
    if (!m_host) return;
    ENetPacket* packet = enet_packet_create(
        buf.data(), buf.size(), reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
    enet_host_broadcast(m_host, channel, packet);
}

void Host::disconnect(uint32_t id) {
    auto it = m_peers.find(id);
    if (it != m_peers.end()) enet_peer_disconnect(it->second, 0);
}

void Host::flush() {
    if (m_host) enet_host_flush(m_host);
}

} // namespace net
