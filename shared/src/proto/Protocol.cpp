#include <cw/proto/Protocol.h>

namespace cw {

void writePacketId(net::ByteBuffer& b, PacketId id) {
    b.writeU16(static_cast<uint16_t>(id));
}

PacketId readPacketId(net::ByteBuffer& b) {
    return static_cast<PacketId>(b.readU16());
}

void writeVec3(net::ByteBuffer& b, const glm::vec3& v) {
    b.writeFloat(v.x);
    b.writeFloat(v.y);
    b.writeFloat(v.z);
}

glm::vec3 readVec3(net::ByteBuffer& b) {
    glm::vec3 v;
    v.x = b.readFloat();
    v.y = b.readFloat();
    v.z = b.readFloat();
    return v;
}

void C2SHello::serialize(net::ByteBuffer& b) const {
    b.writeU32(protocolVersion);
    b.writeString(name);
}
void C2SHello::deserialize(net::ByteBuffer& b) {
    protocolVersion = b.readU32();
    name            = b.readString();
}

void S2CWelcome::serialize(net::ByteBuffer& b) const {
    b.writeU32(protocolVersion);
    b.writeU32(playerId);
    b.writeU32(tickRate);
}
void S2CWelcome::deserialize(net::ByteBuffer& b) {
    protocolVersion = b.readU32();
    playerId        = b.readU32();
    tickRate        = b.readU32();
}

void S2CDisconnect::serialize(net::ByteBuffer& b) const { b.writeString(reason); }
void S2CDisconnect::deserialize(net::ByteBuffer& b) { reason = b.readString(); }

void C2SPing::serialize(net::ByteBuffer& b) const { b.writeU64(clientTimeMs); }
void C2SPing::deserialize(net::ByteBuffer& b) { clientTimeMs = b.readU64(); }

void S2CPong::serialize(net::ByteBuffer& b) const {
    b.writeU64(clientTimeMs);
    b.writeU32(serverTick);
}
void S2CPong::deserialize(net::ByteBuffer& b) {
    clientTimeMs = b.readU64();
    serverTick   = b.readU32();
}

void S2CSpawnEntity::serialize(net::ByteBuffer& b) const {
    b.writeU32(id);
    b.writeU8(type);
    writeVec3(b, position);
    b.writeFloat(yaw);
    b.writeString(name);
    b.writeU16(item);
    b.writeU8(itemCount);
    b.writeU8(hostile);
}
void S2CSpawnEntity::deserialize(net::ByteBuffer& b) {
    id        = b.readU32();
    type      = b.readU8();
    position  = readVec3(b);
    yaw       = b.readFloat();
    name      = b.readString();
    item      = b.readU16();
    itemCount = b.readU8();
    hostile   = b.readU8();
}

void S2CDespawnEntity::serialize(net::ByteBuffer& b) const { b.writeU32(id); }
void S2CDespawnEntity::deserialize(net::ByteBuffer& b) { id = b.readU32(); }

void S2CSnapshot::serialize(net::ByteBuffer& b) const {
    b.writeU32(serverTick);
    b.writeU32(lastInputSeq);
    b.writeU16(static_cast<uint16_t>(entities.size()));
    for (const auto& e : entities) {
        b.writeU32(e.id);
        writeVec3(b, e.position);
        b.writeFloat(e.yaw);
    }
}
void S2CSnapshot::deserialize(net::ByteBuffer& b) {
    serverTick   = b.readU32();
    lastInputSeq = b.readU32();
    const uint16_t count = b.readU16();
    entities.clear();
    for (uint16_t i = 0; i < count; ++i) {
        EntitySnapshot e;
        e.id       = b.readU32();
        e.position = readVec3(b);
        e.yaw      = b.readFloat();
        if (!b.ok()) break;
        entities.push_back(e);
    }
}

void C2SBlockEdit::serialize(net::ByteBuffer& b) const {
    b.writeU8(action);
    b.writeI32(x);
    b.writeI32(y);
    b.writeI32(z);
    b.writeU8(slot);
}
void C2SBlockEdit::deserialize(net::ByteBuffer& b) {
    action = b.readU8();
    x      = b.readI32();
    y      = b.readI32();
    z      = b.readI32();
    slot   = b.readU8();
}

void S2CBlockUpdate::serialize(net::ByteBuffer& b) const {
    b.writeI32(x);
    b.writeI32(y);
    b.writeI32(z);
    b.writeU8(block);
}
void S2CBlockUpdate::deserialize(net::ByteBuffer& b) {
    x     = b.readI32();
    y     = b.readI32();
    z     = b.readI32();
    block = b.readU8();
}

void S2CInventory::serialize(net::ByteBuffer& b) const { inventory.serialize(b); }
void S2CInventory::deserialize(net::ByteBuffer& b) { inventory.deserialize(b); }

void S2CStats::serialize(net::ByteBuffer& b) const {
    b.writeFloat(health);
    b.writeFloat(food);
}
void S2CStats::deserialize(net::ByteBuffer& b) {
    health = b.readFloat();
    food   = b.readFloat();
}

void C2SEat::serialize(net::ByteBuffer& b) const { b.writeU8(slot); }
void C2SEat::deserialize(net::ByteBuffer& b) { slot = b.readU8(); }

void C2SCraft::serialize(net::ByteBuffer& b) const { b.writeU16(recipe); }
void C2SCraft::deserialize(net::ByteBuffer& b) { recipe = b.readU16(); }

void C2SAttack::serialize(net::ByteBuffer& b) const { b.writeU32(target); }
void C2SAttack::deserialize(net::ByteBuffer& b) { target = b.readU32(); }

void S2CTime::serialize(net::ByteBuffer& b) const {
    b.writeU32(worldTick);
    b.writeU32(dayLength);
}
void S2CTime::deserialize(net::ByteBuffer& b) {
    worldTick = b.readU32();
    dayLength = b.readU32();
}

void C2SChat::serialize(net::ByteBuffer& b) const { b.writeString(text); }
void C2SChat::deserialize(net::ByteBuffer& b) { text = b.readString(); }

void S2CChat::serialize(net::ByteBuffer& b) const {
    b.writeString(sender);
    b.writeString(text);
}
void S2CChat::deserialize(net::ByteBuffer& b) {
    sender = b.readString();
    text   = b.readString();
}

void S2CPlayerList::serialize(net::ByteBuffer& b) const {
    b.writeU16(static_cast<uint16_t>(names.size()));
    for (const auto& n : names) b.writeString(n);
}
void S2CPlayerList::deserialize(net::ByteBuffer& b) {
    const uint16_t count = b.readU16();
    names.clear();
    for (uint16_t i = 0; i < count; ++i) {
        names.push_back(b.readString());
        if (!b.ok()) break;
    }
}

} // namespace cw
