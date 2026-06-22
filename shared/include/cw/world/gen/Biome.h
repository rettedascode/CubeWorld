#pragma once

#include <cw/world/Block.h>

#include <cstdint>

namespace cw {

// SHARED biome registry (GL-free). Holds gameplay data (surface/filler blocks,
// hooks for ores/vegetation/spawns) plus CLIENT-FACING tints as PLAIN floats —
// no rendering code, so rule 1 still holds. The client links shared and reads
// the tint by biome id; only the biome id itself is replicated.
enum class BiomeId : uint8_t {
    Plains = 0,
    Forest,
    Desert,
    Snowy,
    Mountains,
    Ocean,
    Beach,
    Swamp,
    Count
};

struct Rgb {
    float r, g, b;
};

struct BiomeDef {
    const char* name;
    BlockType   surface; // top block on land
    BlockType   filler;  // a few blocks below the surface
    Rgb         sky;     // client sky/fog tint
    Rgb         foliage; // client grass/leaf tint
};

const BiomeDef& biomeDef(BiomeId id);

inline BiomeId biomeFromId(uint8_t id) {
    return id < static_cast<uint8_t>(BiomeId::Count) ? static_cast<BiomeId>(id)
                                                     : BiomeId::Plains;
}

} // namespace cw
