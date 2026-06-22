#pragma once

#include <cw/item/Inventory.h>
#include <cw/world/Chunk.h>
#include <cw/world/World.h> // ChunkCoord + floorDiv

#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace cw {

// Persists EDITED chunks to region files (32x32 chunks each). Unmodified terrain
// is regenerated from the seed, so only player changes hit disk. SERVER-side.
class RegionStore {
public:
    static constexpr int kRegionSize = 32;

    explicit RegionStore(std::string directory);

    bool loadChunk(ChunkCoord c, Chunk& out); // true if an edited version existed
    void saveChunk(ChunkCoord c, const Chunk& in);
    void flush(); // write dirty regions to disk

private:
    struct RegionCoord {
        int x, z;
        bool operator==(const RegionCoord& o) const { return x == o.x && z == o.z; }
    };
    struct RegionHash {
        std::size_t operator()(const RegionCoord& r) const {
            return std::hash<int>{}(r.x) ^ (std::hash<int>{}(r.z) * 0x9e3779b9u);
        }
    };

    RegionCoord regionOf(ChunkCoord c) const {
        return {floorDiv(c.x, kRegionSize), floorDiv(c.z, kRegionSize)};
    }
    void   ensureRegionLoaded(RegionCoord r);
    std::string regionPath(RegionCoord r) const;

    std::string m_dir;
    // region -> (chunk -> RLE bytes)
    std::unordered_map<RegionCoord,
                       std::unordered_map<ChunkCoord, std::vector<uint8_t>, ChunkCoordHash>,
                       RegionHash> m_cache;
    std::unordered_set<RegionCoord, RegionHash> m_loaded;
    std::unordered_set<RegionCoord, RegionHash> m_dirty;
};

// Persisted per-player state.
struct PlayerData {
    glm::vec3 position{0.0f};
    float     yaw    = 0.0f;
    float     pitch  = 0.0f;
    float     health = 20.0f;
    float     food   = 20.0f;
    Inventory inventory;
};

// Loads/saves player data to <directory>/<name>.dat. SERVER-side.
class PlayerStore {
public:
    explicit PlayerStore(std::string directory);
    bool load(const std::string& name, PlayerData& out) const;
    void save(const std::string& name, const PlayerData& data) const;

private:
    std::string path(const std::string& name) const;
    std::string m_dir;
};

} // namespace cw
