#pragma once

#include <cw/world/Chunk.h>
#include <cw/world/World.h> // ChunkCoord
#include <cw/world/gen/GenStage.h>

#include <FastNoiseLite.h>

#include <memory>
#include <vector>

namespace cw {

// Deterministic, seed-driven world generator built from an ordered list of
// GenStages (heightmap -> biomes -> caves -> ores -> flora -> structures -> ...).
// SHARED, GL-free. generate() is const and holds no per-call mutable state, so
// it is safe to call concurrently for different chunks (future worker-thread gen).
//
// W1 ships only the base-terrain stage; later milestones append stages.
class WorldGenerator {
public:
    explicit WorldGenerator(int seed = 1337);

    void generate(Chunk& chunk, ChunkCoord coord) const;
    int  seed() const { return m_seed; }

private:
    int                                    m_seed;
    FastNoiseLite                          m_noise;       // base terrain heightmap
    FastNoiseLite                          m_temperature; // biome climate
    FastNoiseLite                          m_humidity;    // biome climate
    FastNoiseLite                          m_caveA;       // cave tunnel field A
    FastNoiseLite                          m_caveB;       // cave tunnel field B
    FastNoiseLite                          m_caveCheese;  // cave cavern field
    std::vector<std::unique_ptr<GenStage>> m_stages;
};

} // namespace cw
