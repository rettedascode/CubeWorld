#pragma once

#include <cstdint>

namespace cw {

// Voxel block types. Air is 0 and is never meshed.
//
// SHARED (headless simulation): this is pure game state with no rendering info.
// Texture/atlas mapping for a block lives on the CLIENT (see render/BlockTextures.h).
enum class BlockType : uint8_t {
    Air = 0,
    Grass,
    Dirt,
    Stone,
    Sand,
    Water,
    Snow,    // W2: snowy-biome surface
    Log,     // W3: tree trunk
    Leaves,  // W3: tree canopy (foliage-tinted on the client)
    Cactus,  // W3: desert flora
    CoalOre, // W3: ores embedded in stone
    IronOre,
    GoldOre,
    Count
};

// Seconds to mine by hand (divided by tool speed). Used by the client to gate
// progressive mining; the server validates only reach.
inline float blockHardness(BlockType t) {
    switch (t) {
        case BlockType::Stone:   return 1.6f;
        case BlockType::CoalOre:
        case BlockType::IronOre:
        case BlockType::GoldOre:  return 3.0f; // ore-bearing rock is tougher
        case BlockType::Log:
        case BlockType::Cactus:  return 2.0f;
        case BlockType::Dirt:
        case BlockType::Grass:
        case BlockType::Sand:    return 0.5f;
        case BlockType::Snow:    return 0.3f;
        case BlockType::Leaves:  return 0.2f;
        default:                 return 0.4f;
    }
}

inline bool isAir(BlockType t) { return t == BlockType::Air; }

// Solid = anything that gets meshed / collides (everything but air).
inline bool isSolid(BlockType t) { return t != BlockType::Air; }

// Opaque = fully blocks the view of the neighbour behind it. A face is culled
// when its neighbour is opaque. Water is solid but not opaque (transparent).
inline bool isOpaque(BlockType t) {
    return t != BlockType::Air && t != BlockType::Water;
}

} // namespace cw
