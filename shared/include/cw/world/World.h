#pragma once

#include <cw/world/Chunk.h>

#include <cstddef>
#include <memory>
#include <unordered_map>

namespace cw {

// Floor division / modulo that round toward negative infinity, so world->chunk
// coordinate conversion is correct for negative coordinates (built-in / and %
// truncate toward zero, which is wrong here).
inline int floorDiv(int a, int b) {
    int q = a / b;
    if ((a % b != 0) && ((a < 0) != (b < 0))) --q;
    return q;
}
inline int floorMod(int a, int b) {
    int r = a % b;
    if (r != 0 && ((r < 0) != (b < 0))) r += b;
    return r;
}

// Chunk grid coordinate (chunks span the full Y range, so only X/Z).
struct ChunkCoord {
    int x = 0;
    int z = 0;
    bool operator==(const ChunkCoord& o) const { return x == o.x && z == o.z; }
};

struct ChunkCoordHash {
    std::size_t operator()(const ChunkCoord& c) const {
        std::size_t h = std::hash<int>{}(c.x);
        h ^= std::hash<int>{}(c.z) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

// Owns the loaded chunks and resolves block queries in world space, including
// across chunk borders (so the mesher can cull seam faces correctly).
class World {
public:
    using ChunkMap = std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>, ChunkCoordHash>;

    Chunk*       getChunk(ChunkCoord c);
    const Chunk* getChunk(ChunkCoord c) const;
    Chunk&       createChunk(ChunkCoord c); // empty chunk if not already present
    void         removeChunk(ChunkCoord c); // unload (drops voxel data)

    BlockType getBlock(int wx, int wy, int wz) const; // Air if chunk not loaded
    void      setBlock(int wx, int wy, int wz, BlockType type);

    uint8_t   getBiome(int wx, int wz) const; // 0 (plains) if chunk not loaded

    static ChunkCoord worldToChunk(int wx, int wz) {
        return {floorDiv(wx, Chunk::SIZE_X), floorDiv(wz, Chunk::SIZE_Z)};
    }

    const ChunkMap& chunks() const { return m_chunks; }

private:
    ChunkMap m_chunks;
};

} // namespace cw
