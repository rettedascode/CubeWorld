#include "net/NetClient.h"

#include <cw/proto/Protocol.h>

#include <ce/util/Log.h>

#include <glm/glm.hpp>

#include <chrono>

namespace cw {

namespace {
constexpr uint64_t kPingIntervalMs = 1000;
constexpr uint64_t kInterpDelayMs  = 100; // render remote entities this far in the past

template <typename T>
void sendReliable(net::Host& host, uint32_t peer, PacketId id, const T& msg) {
    net::ByteBuffer b;
    writePacketId(b, id);
    msg.serialize(b);
    host.send(peer, net::kChannelReliable, b, true);
}
} // namespace

uint64_t NetClient::nowMs() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

bool NetClient::connect(const std::string& host, uint16_t port, const std::string& name) {
    m_name = name;
    if (!m_host.createClient()) {
        CE_LOG_ERROR("NetClient: failed to create ENet client host");
        return false;
    }
    m_serverPeer = m_host.connect(host, port);
    if (m_serverPeer == 0) {
        CE_LOG_ERROR("NetClient: failed to start connection to ", host, ":", port);
        return false;
    }
    m_active = true;
    CE_LOG_INFO("NetClient: connecting to ", host, ":", port, " ...");
    return true;
}

void NetClient::update() {
    if (!m_active) return;

    net::Event e;
    while (m_host.poll(e, 0)) {
        switch (e.type) {
            case net::EventType::Connect: {
                CE_LOG_INFO("NetClient: connected, sending hello");
                C2SHello hello;
                hello.name = m_name;
                sendReliable(m_host, m_serverPeer, PacketId::C2S_Hello, hello);
                break;
            }
            case net::EventType::Receive:
                handlePacket(e.data);
                break;
            case net::EventType::Disconnect:
                CE_LOG_WARN("NetClient: disconnected from server");
                m_connected = false;
                m_active    = false;
                break;
            default:
                break;
        }
    }

    // Heartbeat ping once connected.
    if (m_connected) {
        const uint64_t now = nowMs();
        if (now - m_lastPingMs >= kPingIntervalMs) {
            m_lastPingMs = now;
            C2SPing ping{now};
            sendReliable(m_host, m_serverPeer, PacketId::C2S_Ping, ping);
        }
    }

    interpolateEntities();
}

void NetClient::sendInput(const InputCommand& cmd) {
    if (!m_connected) return;
    sendReliable(m_host, m_serverPeer, PacketId::C2S_Input, cmd);
}

std::vector<ChunkUpdate> NetClient::takeChunkUpdates() {
    std::vector<ChunkUpdate> out = std::move(m_chunkUpdates);
    m_chunkUpdates.clear(); // ensure a defined empty state after the move
    return out;
}

void NetClient::sendBlockEdit(uint8_t action, int x, int y, int z, uint8_t slot) {
    if (!m_connected) return;
    C2SBlockEdit edit;
    edit.action = action;
    edit.x      = x;
    edit.y      = y;
    edit.z      = z;
    edit.slot   = slot;
    sendReliable(m_host, m_serverPeer, PacketId::C2S_BlockEdit, edit);
}

void NetClient::sendEat(uint8_t slot) {
    if (!m_connected) return;
    C2SEat eat{slot};
    sendReliable(m_host, m_serverPeer, PacketId::C2S_Eat, eat);
}

void NetClient::sendCraft(uint16_t recipe) {
    if (!m_connected) return;
    C2SCraft craft{recipe};
    sendReliable(m_host, m_serverPeer, PacketId::C2S_Craft, craft);
}

void NetClient::sendAttack(uint32_t entityId) {
    if (!m_connected) return;
    C2SAttack atk{entityId};
    sendReliable(m_host, m_serverPeer, PacketId::C2S_Attack, atk);
}

void NetClient::sendChat(const std::string& text) {
    if (!m_connected || text.empty()) return;
    C2SChat c{text};
    sendReliable(m_host, m_serverPeer, PacketId::C2S_Chat, c);
}

std::vector<BlockUpdate> NetClient::takeBlockUpdates() {
    std::vector<BlockUpdate> out = std::move(m_blockUpdates);
    m_blockUpdates.clear();
    return out;
}

bool NetClient::takeReconcile(glm::vec3& outPosition, uint32_t& outAckSeq) {
    if (!m_hasReconcile) return false;
    outPosition    = m_authPosition;
    outAckSeq      = m_ackSeq;
    m_hasReconcile = false;
    return true;
}

// Render remote entities ~kInterpDelayMs in the past, lerping between the two
// snapshots that bracket that render time. This hides packet jitter and the
// 20 Hz snapshot rate.
void NetClient::interpolateEntities() {
    const uint64_t renderTime = nowMs() - kInterpDelayMs;

    for (auto& [id, e] : m_entities) {
        auto& h = e.history;
        if (h.empty()) continue;

        if (renderTime <= h.front().timeMs) {
            e.position = h.front().position;
            e.yaw      = h.front().yaw;
            continue;
        }
        if (renderTime >= h.back().timeMs) {
            e.position = h.back().position;
            e.yaw      = h.back().yaw;
            continue;
        }
        for (size_t i = 0; i + 1 < h.size(); ++i) {
            const EntitySample& a = h[i];
            const EntitySample& b = h[i + 1];
            if (renderTime >= a.timeMs && renderTime <= b.timeMs) {
                const float span = static_cast<float>(b.timeMs - a.timeMs);
                const float t    = span > 0.0f ? (renderTime - a.timeMs) / span : 0.0f;
                e.position = glm::mix(a.position, b.position, t);
                e.yaw      = glm::mix(a.yaw, b.yaw, t);
                break;
            }
        }
    }
}

void NetClient::handlePacket(net::ByteBuffer& data) {
    const PacketId id = readPacketId(data);
    switch (id) {
        case PacketId::S2C_Welcome: {
            S2CWelcome w;
            w.deserialize(data);
            if (!data.ok()) return;
            m_connected = true;
            m_playerId  = w.playerId;
            CE_LOG_INFO("NetClient: welcomed as player ", w.playerId,
                        " (server ", w.tickRate, " Hz)");
            break;
        }
        case PacketId::S2C_Pong: {
            S2CPong pong;
            pong.deserialize(data);
            if (!data.ok()) return;
            const uint64_t rtt = nowMs() - pong.clientTimeMs;
            CE_LOG_INFO("NetClient: pong (RTT ", rtt, " ms, server tick ",
                        pong.serverTick, ")");
            break;
        }
        case PacketId::S2C_Disconnect: {
            S2CDisconnect d;
            d.deserialize(data);
            CE_LOG_WARN("NetClient: kicked - ", d.reason);
            break;
        }
        case PacketId::S2C_SpawnEntity: {
            S2CSpawnEntity s;
            s.deserialize(data);
            if (!data.ok()) return;
            if (s.id == m_playerId) return; // that's us; the camera represents it
            RemoteEntity& e = m_entities[s.id];
            e.id       = s.id;
            e.type     = s.type;
            e.hostile  = s.hostile != 0;
            e.position = s.position;
            e.yaw      = s.yaw;
            e.name     = s.name;
            e.history.clear();
            e.history.push_back({nowMs(), s.position, s.yaw}); // seed interpolation
            break;
        }
        case PacketId::S2C_DespawnEntity: {
            S2CDespawnEntity d;
            d.deserialize(data);
            if (!data.ok()) return;
            m_entities.erase(d.id);
            CE_LOG_INFO("NetClient: entity ", d.id, " despawned");
            break;
        }
        case PacketId::S2C_ChunkData: {
            ChunkUpdate u;
            u.coord.x = data.readI32();
            u.coord.z = data.readI32();
            u.chunk   = std::make_shared<Chunk>();
            u.chunk->deserialize(data);
            if (!data.ok()) return;
            m_chunkUpdates.push_back(std::move(u));
            break;
        }
        case PacketId::S2C_BlockUpdate: {
            S2CBlockUpdate up;
            up.deserialize(data);
            if (!data.ok()) return;
            m_blockUpdates.push_back(
                {up.x, up.y, up.z, static_cast<BlockType>(up.block)});
            break;
        }
        case PacketId::S2C_Inventory: {
            S2CInventory inv;
            inv.deserialize(data);
            if (!data.ok()) return;
            m_inventory = inv.inventory;
            break;
        }
        case PacketId::S2C_Stats: {
            S2CStats s;
            s.deserialize(data);
            if (!data.ok()) return;
            m_health = s.health;
            m_food   = s.food;
            break;
        }
        case PacketId::S2C_Time: {
            S2CTime t;
            t.deserialize(data);
            if (!data.ok()) return;
            m_worldTick = t.worldTick;
            m_dayLength = t.dayLength;
            break;
        }
        case PacketId::S2C_Chat: {
            S2CChat c;
            c.deserialize(data);
            if (!data.ok()) return;
            m_chatLog.push_back(c.sender.empty() ? c.text : c.sender + ": " + c.text);
            if (m_chatLog.size() > 100) m_chatLog.erase(m_chatLog.begin());
            break;
        }
        case PacketId::S2C_PlayerList: {
            S2CPlayerList pl;
            pl.deserialize(data);
            if (!data.ok()) return;
            m_playerNames = pl.names;
            break;
        }
        case PacketId::S2C_Snapshot: {
            S2CSnapshot snap;
            snap.deserialize(data);
            if (!data.ok()) return;

            const uint64_t now = nowMs();
            for (const auto& es : snap.entities) {
                if (es.id == m_playerId) {
                    // Our own authoritative state — hand off for reconciliation.
                    m_authPosition = es.position;
                    m_ackSeq       = snap.lastInputSeq;
                    m_hasReconcile = true;
                    continue;
                }
                RemoteEntity& e = m_entities[es.id];
                e.id = es.id;
                e.history.push_back({now, es.position, es.yaw});
                while (e.history.size() > 32) e.history.pop_front();
            }
            break;
        }
        default:
            break;
    }
}

void NetClient::disconnect() {
    if (m_active && m_serverPeer) {
        m_host.disconnect(m_serverPeer);
        m_host.flush();
    }
    m_active    = false;
    m_connected = false;
}

} // namespace cw
