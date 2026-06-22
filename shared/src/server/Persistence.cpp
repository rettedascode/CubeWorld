#include <cw/server/Persistence.h>

#include <cw/proto/Protocol.h> // writeVec3/readVec3
#include <net/ByteBuffer.h>

#include <filesystem>
#include <fstream>

namespace cw {

namespace {
std::vector<uint8_t> readFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return {};
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(in)),
                                std::istreambuf_iterator<char>());
}
void writeFile(const std::string& path, const uint8_t* data, size_t size) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (out) out.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));
}
} // namespace

// ---- RegionStore -----------------------------------------------------------

RegionStore::RegionStore(std::string directory) : m_dir(std::move(directory)) {
    std::error_code ec;
    std::filesystem::create_directories(m_dir, ec);
}

std::string RegionStore::regionPath(RegionCoord r) const {
    return m_dir + "/r." + std::to_string(r.x) + "." + std::to_string(r.z) + ".bin";
}

void RegionStore::ensureRegionLoaded(RegionCoord r) {
    if (m_loaded.count(r)) return;
    m_loaded.insert(r);

    const std::vector<uint8_t> bytes = readFile(regionPath(r));
    if (bytes.empty()) return;

    net::ByteBuffer b(bytes.data(), bytes.size());
    const uint32_t count = b.readU32();
    auto& region = m_cache[r];
    for (uint32_t i = 0; i < count && b.ok(); ++i) {
        ChunkCoord c;
        c.x = b.readI32();
        c.z = b.readI32();
        const uint32_t len = b.readU32();
        std::vector<uint8_t> rle(len);
        for (uint32_t k = 0; k < len; ++k) rle[k] = b.readU8();
        if (!b.ok()) break;
        region[c] = std::move(rle);
    }
}

bool RegionStore::loadChunk(ChunkCoord c, Chunk& out) {
    const RegionCoord r = regionOf(c);
    ensureRegionLoaded(r);
    auto rit = m_cache.find(r);
    if (rit == m_cache.end()) return false;
    auto cit = rit->second.find(c);
    if (cit == rit->second.end()) return false;

    net::ByteBuffer b(cit->second.data(), cit->second.size());
    out.deserialize(b);
    return b.ok();
}

void RegionStore::saveChunk(ChunkCoord c, const Chunk& in) {
    const RegionCoord r = regionOf(c);
    ensureRegionLoaded(r);

    net::ByteBuffer b;
    in.serialize(b);
    m_cache[r][c] = std::vector<uint8_t>(b.data(), b.data() + b.size());
    m_dirty.insert(r);
}

void RegionStore::flush() {
    for (const RegionCoord& r : m_dirty) {
        auto it = m_cache.find(r);
        if (it == m_cache.end()) continue;

        net::ByteBuffer b;
        b.writeU32(static_cast<uint32_t>(it->second.size()));
        for (const auto& [coord, rle] : it->second) {
            b.writeI32(coord.x);
            b.writeI32(coord.z);
            b.writeU32(static_cast<uint32_t>(rle.size()));
            b.writeBytes(rle.data(), rle.size());
        }
        writeFile(regionPath(r), b.data(), b.size());
    }
    m_dirty.clear();
}

// ---- PlayerStore -----------------------------------------------------------

PlayerStore::PlayerStore(std::string directory) : m_dir(std::move(directory)) {
    std::error_code ec;
    std::filesystem::create_directories(m_dir, ec);
}

std::string PlayerStore::path(const std::string& name) const {
    return m_dir + "/" + name + ".dat";
}

bool PlayerStore::load(const std::string& name, PlayerData& out) const {
    const std::vector<uint8_t> bytes = readFile(path(name));
    if (bytes.empty()) return false;

    net::ByteBuffer b(bytes.data(), bytes.size());
    out.position = readVec3(b);
    out.yaw      = b.readFloat();
    out.pitch    = b.readFloat();
    out.health   = b.readFloat();
    out.food     = b.readFloat();
    out.inventory.deserialize(b);
    return b.ok();
}

void PlayerStore::save(const std::string& name, const PlayerData& data) const {
    net::ByteBuffer b;
    writeVec3(b, data.position);
    b.writeFloat(data.yaw);
    b.writeFloat(data.pitch);
    b.writeFloat(data.health);
    b.writeFloat(data.food);
    data.inventory.serialize(b);
    writeFile(path(name), b.data(), b.size());
}

} // namespace cw
