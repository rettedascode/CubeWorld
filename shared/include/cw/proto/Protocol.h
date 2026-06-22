#pragma once

#include <cw/item/Inventory.h>

#include <net/ByteBuffer.h>

#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <vector>

// SHARED network protocol. Each packet is a struct that (de)serializes only its
// PAYLOAD; the sender writes the PacketId first via writePacketId(). NO OpenGL.
namespace cw {

constexpr uint32_t kProtocolVersion = 14; // v14: chunks carry an RLE biome section
constexpr uint16_t kDefaultPort     = 25565;
constexpr int      kViewDistance    = 6;    // chunk streaming radius (server + client)
constexpr float    kReachDistance   = 8.0f; // max block interaction distance

enum class PacketId : uint16_t {
    None = 0,
    C2S_Hello,      // client -> server: request to join
    S2C_Welcome,    // server -> client: accepted, here is your id
    S2C_Disconnect, // server -> client: rejected/kicked with a reason
    C2S_Ping,       // client -> server: latency probe
    S2C_Pong,       // server -> client: echo + current tick

    S2C_SpawnEntity,   // server -> client: an entity appeared
    S2C_DespawnEntity, // server -> client: an entity was removed

    C2S_Input,    // client -> server: one timestamped input command
    S2C_Snapshot, // server -> client: authoritative entity states (+ input ack)

    S2C_ChunkData, // server -> client: RLE chunk (preceded by int32 cx, cz)

    C2S_BlockEdit,   // client -> server: request to break/place a block
    S2C_BlockUpdate, // server -> client: a block changed (authoritative delta)

    S2C_Inventory, // server -> client: full inventory sync (owner only)

    S2C_Stats, // server -> client: health/food for the owning player
    C2S_Eat,   // client -> server: eat the food item in a hotbar slot

    C2S_Craft,  // client -> server: craft a recipe by index
    C2S_Attack, // client -> server: melee-attack an entity

    S2C_Time, // server -> client: authoritative world time

    C2S_Chat,       // client -> server: a chat message (may be a /command)
    S2C_Chat,       // server -> client: a chat line to display
    S2C_PlayerList, // server -> client: current player names
};

namespace BlockEditAction {
enum : uint8_t { Break = 0, Place = 1 };
}

void     writePacketId(net::ByteBuffer& b, PacketId id);
PacketId readPacketId(net::ByteBuffer& b);

void      writeVec3(net::ByteBuffer& b, const glm::vec3& v);
glm::vec3 readVec3(net::ByteBuffer& b);

struct C2SHello {
    uint32_t    protocolVersion = kProtocolVersion;
    std::string name;
    void serialize(net::ByteBuffer& b) const;
    void deserialize(net::ByteBuffer& b);
};

struct S2CWelcome {
    uint32_t protocolVersion = kProtocolVersion;
    uint32_t playerId        = 0;
    uint32_t tickRate        = 0;
    void serialize(net::ByteBuffer& b) const;
    void deserialize(net::ByteBuffer& b);
};

struct S2CDisconnect {
    std::string reason;
    void serialize(net::ByteBuffer& b) const;
    void deserialize(net::ByteBuffer& b);
};

struct C2SPing {
    uint64_t clientTimeMs = 0;
    void serialize(net::ByteBuffer& b) const;
    void deserialize(net::ByteBuffer& b);
};

struct S2CPong {
    uint64_t clientTimeMs = 0; // echoed back for RTT
    uint32_t serverTick   = 0;
    void serialize(net::ByteBuffer& b) const;
    void deserialize(net::ByteBuffer& b);
};

struct S2CSpawnEntity {
    uint32_t    id   = 0;
    uint8_t     type = 0; // EntityType
    glm::vec3   position{0.0f};
    float       yaw = 0.0f;
    std::string name;
    uint16_t    item      = 0; // ItemType (for Item entities)
    uint8_t     itemCount = 0;
    uint8_t     hostile   = 0; // mobs
    void serialize(net::ByteBuffer& b) const;
    void deserialize(net::ByteBuffer& b);
};

struct S2CDespawnEntity {
    uint32_t id = 0;
    void serialize(net::ByteBuffer& b) const;
    void deserialize(net::ByteBuffer& b);
};

// One entity's authoritative state within a snapshot.
struct EntitySnapshot {
    uint32_t  id = 0;
    glm::vec3 position{0.0f};
    float     yaw = 0.0f;
};

struct S2CSnapshot {
    uint32_t serverTick = 0;
    uint32_t lastInputSeq = 0; // last input from THIS client the server has applied
    std::vector<EntitySnapshot> entities;
    void serialize(net::ByteBuffer& b) const;
    void deserialize(net::ByteBuffer& b);
};

struct C2SBlockEdit {
    uint8_t action = 0; // BlockEditAction
    int32_t x = 0, y = 0, z = 0;
    uint8_t slot = 0;   // hotbar slot to place from (ignored for Break)
    void serialize(net::ByteBuffer& b) const;
    void deserialize(net::ByteBuffer& b);
};

struct S2CBlockUpdate {
    int32_t x = 0, y = 0, z = 0;
    uint8_t block = 0;
    void serialize(net::ByteBuffer& b) const;
    void deserialize(net::ByteBuffer& b);
};

struct S2CInventory {
    Inventory inventory;
    void serialize(net::ByteBuffer& b) const;
    void deserialize(net::ByteBuffer& b);
};

struct S2CStats {
    float health = 20.0f;
    float food   = 20.0f;
    void serialize(net::ByteBuffer& b) const;
    void deserialize(net::ByteBuffer& b);
};

struct C2SEat {
    uint8_t slot = 0;
    void serialize(net::ByteBuffer& b) const;
    void deserialize(net::ByteBuffer& b);
};

struct C2SCraft {
    uint16_t recipe = 0;
    void serialize(net::ByteBuffer& b) const;
    void deserialize(net::ByteBuffer& b);
};

struct C2SAttack {
    uint32_t target = 0; // entity id
    void serialize(net::ByteBuffer& b) const;
    void deserialize(net::ByteBuffer& b);
};

struct S2CTime {
    uint32_t worldTick = 0;
    uint32_t dayLength = 1; // ticks per full day
    void serialize(net::ByteBuffer& b) const;
    void deserialize(net::ByteBuffer& b);
};

struct C2SChat {
    std::string text;
    void serialize(net::ByteBuffer& b) const;
    void deserialize(net::ByteBuffer& b);
};

struct S2CChat {
    std::string sender; // empty for system/server lines
    std::string text;
    void serialize(net::ByteBuffer& b) const;
    void deserialize(net::ByteBuffer& b);
};

struct S2CPlayerList {
    std::vector<std::string> names;
    void serialize(net::ByteBuffer& b) const;
    void deserialize(net::ByteBuffer& b);
};

} // namespace cw
