#pragma once

#include <cw/world/Chunk.h>
#include <cw/world/World.h> // ChunkCoord

#include <cstdint>
#include <random>

namespace cw {

// SHARED worldgen support (NO OpenGL). Per-chunk context handed to each
// generation stage. It carries the world seed + chunk coordinate and produces
// DETERMINISTIC, order/thread-independent randomness: the seed for a stage is a
// pure hash of (worldSeed, chunkX, chunkZ, stageId), so two chunks (or two
// threads in any order) always generate identically.

// SplitMix64 finalizer — a cheap, well-distributed integer mix.
inline uint64_t mix64(uint64_t x) {
    x += 0x9E3779B97F4A7C15ull;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
    return x ^ (x >> 31);
}

struct GenContext {
    int32_t    worldSeed = 0;
    ChunkCoord coord;
    int        baseX = 0; // coord.x * Chunk::SIZE_X (world X of local 0)
    int        baseZ = 0; // coord.z * Chunk::SIZE_Z

    GenContext(int32_t seed, ChunkCoord c)
        : worldSeed(seed), coord(c),
          baseX(c.x * Chunk::SIZE_X), baseZ(c.z * Chunk::SIZE_Z) {}

    // Deterministic 64-bit seed for a generation stage. Independent of call
    // order and thread — purely a function of (seed, coord, stageId).
    uint64_t stageSeed(uint32_t stageId) const {
        uint64_t h = static_cast<uint64_t>(static_cast<uint32_t>(worldSeed));
        h = mix64(h ^ (static_cast<uint64_t>(static_cast<uint32_t>(coord.x)) * 0x9E3779B97F4A7C15ull));
        h = mix64(h ^ (static_cast<uint64_t>(static_cast<uint32_t>(coord.z)) * 0xC2B2AE3D27D4EB4Full));
        h = mix64(h ^ static_cast<uint64_t>(stageId));
        return h;
    }

    // A fresh RNG seeded deterministically for a stage.
    std::mt19937 rng(uint32_t stageId) const {
        return std::mt19937(static_cast<std::mt19937::result_type>(stageSeed(stageId)));
    }
};

} // namespace cw
