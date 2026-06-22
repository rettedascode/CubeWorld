#pragma once

#include <cw/world/Block.h>

#include <array>

namespace net { class ByteBuffer; }

namespace cw {

// A fixed-size column of voxels. Coordinates are local to the chunk:
// x,z in [0,16), y in [0,256), right-handed with Y up.
//
// Blocks are stored in a flat array indexed  x + 16*(z + 16*y)  (i.e. X is the
// fastest-varying axis, then Z, then Y). One engine Mesh is built per chunk.
class Chunk {
public:
    static constexpr int SIZE_X = 16;
    static constexpr int SIZE_Y = 256;
    static constexpr int SIZE_Z = 16;
    static constexpr int VOLUME = SIZE_X * SIZE_Y * SIZE_Z;
    static constexpr int AREA   = SIZE_X * SIZE_Z; // one biome id per column

    Chunk(); // all Air, biome 0

    // Out-of-range reads return Air so the mesher can query neighbours freely.
    BlockType get(int x, int y, int z) const;
    void      set(int x, int y, int z, BlockType type);

    // Per-column biome id (W2). Out-of-range reads return 0.
    uint8_t biome(int x, int z) const;
    void    setBiome(int x, int z, uint8_t id);

    static bool inBounds(int x, int y, int z) {
        return x >= 0 && x < SIZE_X &&
               y >= 0 && y < SIZE_Y &&
               z >= 0 && z < SIZE_Z;
    }

    static int index(int x, int y, int z) {
        return x + SIZE_X * (z + SIZE_Z * y);
    }

    // Run-length encode/decode the block array for network transfer. Terrain is
    // highly repetitive (air above, stone below), so this shrinks the 64 KB raw
    // array to a few hundred bytes for typical chunks.
    void serialize(net::ByteBuffer& b) const;
    void deserialize(net::ByteBuffer& b);

private:
    std::array<BlockType, VOLUME> m_blocks;
    std::array<uint8_t, AREA>     m_biomes;
};

} // namespace cw
