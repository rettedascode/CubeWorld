#pragma once

#include <cw/entity/Entity.h>
#include <cw/item/Inventory.h>
#include <cw/sim/PlayerMovement.h>
#include <cw/world/Chunk.h>
#include <cw/world/World.h> // ChunkCoord
#include <net/Host.h>

#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace cw {

// A chunk received from the server, waiting to be inserted + meshed by the layer.
struct ChunkUpdate {
    ChunkCoord             coord;
    std::shared_ptr<Chunk> chunk;
};

// An authoritative single-block change received from the server.
struct BlockUpdate {
    int       x = 0, y = 0, z = 0;
    BlockType block = BlockType::Air;
};

// A timestamped authoritative sample used for snapshot interpolation.
struct EntitySample {
    uint64_t  timeMs = 0;
    glm::vec3 position{0.0f};
    float     yaw = 0.0f;
};

// Lightweight client-side mirror of a remote entity. `position`/`yaw` hold the
// interpolated render state (~100 ms behind the latest snapshot).
struct RemoteEntity {
    EntityId    id      = 0;
    uint8_t     type    = 0;
    bool        hostile = false;
    glm::vec3   position{0.0f};
    float       yaw = 0.0f;
    std::string name;
    std::deque<EntitySample> history;
};

// CLIENT-side network connection to the server. For A2 it performs the handshake
// and periodic latency pings; movement/world replication come in Phase B. It
// does NOT touch rendering (it lives in the client only because it drives the
// presentation layer).
class NetClient {
public:
    bool connect(const std::string& host, uint16_t port, const std::string& name);
    void update();   // poll events, handle packets, send periodic pings
    void disconnect();

    bool     connected() const { return m_connected; }
    uint32_t playerId() const { return m_playerId; }

    // Send one input command (reliable-ordered so the server applies all in order).
    void sendInput(const InputCommand& cmd);

    // Latest authoritative state of the local player + the last input it reflects.
    // Returns false if no new snapshot has arrived since the last call.
    bool takeReconcile(glm::vec3& outPosition, uint32_t& outAckSeq);

    // Remote entities (excludes the local player). Read by the renderer.
    const std::unordered_map<EntityId, RemoteEntity>& entities() const { return m_entities; }

    // Chunks received since the last call (the layer inserts + meshes them).
    std::vector<ChunkUpdate> takeChunkUpdates();

    // Request a break/place; the server validates and echoes an authoritative delta.
    // `slot` is the hotbar slot to place from (ignored for Break).
    void sendBlockEdit(uint8_t action, int x, int y, int z, uint8_t slot);

    // Authoritative block deltas received since the last call.
    std::vector<BlockUpdate> takeBlockUpdates();

    // The local player's inventory (server-authoritative mirror, for the HUD).
    const Inventory& inventory() const { return m_inventory; }

    // Request eating the food item in a hotbar slot.
    void sendEat(uint8_t slot);

    // Request crafting a recipe by index.
    void sendCraft(uint16_t recipe);

    // Request a melee attack on an entity.
    void sendAttack(uint32_t entityId);

    float health() const { return m_health; }
    float food() const { return m_food; }

    uint32_t worldTick() const { return m_worldTick; }
    uint32_t dayLength() const { return m_dayLength; }

    void sendChat(const std::string& text);
    const std::vector<std::string>& chatLog() const { return m_chatLog; }
    const std::vector<std::string>& playerNames() const { return m_playerNames; }

private:
    void handlePacket(net::ByteBuffer& data);
    void interpolateEntities();
    static uint64_t nowMs();

    net::Host m_host;
    uint32_t  m_serverPeer = 0;
    uint32_t  m_playerId   = 0;
    bool      m_active     = false; // host created + connect issued
    bool      m_connected  = false; // handshake completed
    std::string m_name;

    uint64_t m_lastPingMs = 0;

    std::unordered_map<EntityId, RemoteEntity> m_entities;

    // Local-player reconciliation state from the most recent snapshot.
    bool      m_hasReconcile = false;
    glm::vec3 m_authPosition{0.0f};
    uint32_t  m_ackSeq = 0;

    std::vector<ChunkUpdate> m_chunkUpdates; // received chunks awaiting the layer
    std::vector<BlockUpdate> m_blockUpdates; // received block deltas awaiting the layer

    Inventory m_inventory; // local player's inventory mirror
    float     m_health = 20.0f;
    float     m_food   = 20.0f;

    uint32_t  m_worldTick = 0;
    uint32_t  m_dayLength = 1;

    std::vector<std::string> m_chatLog;
    std::vector<std::string> m_playerNames;
};

} // namespace cw
