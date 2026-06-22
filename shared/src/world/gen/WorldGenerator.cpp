#include <cw/world/gen/WorldGenerator.h>

#include <cw/world/gen/BaseTerrainStage.h>
#include <cw/world/gen/BiomeStage.h>
#include <cw/world/gen/CaveStage.h>
#include <cw/world/gen/FloraStage.h>
#include <cw/world/gen/GenContext.h>
#include <cw/world/gen/OreStage.h>

namespace cw {

namespace {
// Base terrain noise configuration (unchanged from the original generator).
constexpr float kFrequency      = 0.012f;
constexpr int   kOctaves        = 4;
constexpr float kClimateFreq    = 0.0035f; // large biome regions
constexpr float kCaveFreq       = 0.022f;  // tunnel winding scale
constexpr float kCheeseFreq     = 0.045f;  // cavern blob scale
} // namespace

WorldGenerator::WorldGenerator(int seed) : m_seed(seed) {
    m_noise.SetSeed(seed);
    m_noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_noise.SetFractalOctaves(kOctaves);
    m_noise.SetFrequency(kFrequency);

    // Independent climate fields (distinct seeds so they don't correlate).
    m_temperature.SetSeed(seed + 101);
    m_temperature.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_temperature.SetFrequency(kClimateFreq);

    m_humidity.SetSeed(seed + 202);
    m_humidity.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_humidity.SetFrequency(kClimateFreq);

    // Cave carving fields. Two independent tunnel fields (their joint zero-set is
    // a winding tube) plus one FBm cheese field for occasional caverns.
    m_caveA.SetSeed(seed + 303);
    m_caveA.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_caveA.SetFrequency(kCaveFreq);

    m_caveB.SetSeed(seed + 404);
    m_caveB.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_caveB.SetFrequency(kCaveFreq);

    m_caveCheese.SetSeed(seed + 505);
    m_caveCheese.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_caveCheese.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_caveCheese.SetFractalOctaves(2);
    m_caveCheese.SetFrequency(kCheeseFreq);

    // Stage order is the pipeline order. Later milestones append stages here.
    // Flora needs to recompute the height + biome of arbitrary world columns, so
    // keep stable references to the terrain/biome stages it depends on (the
    // vector owns them; references stay valid across reallocation).
    auto base  = std::make_unique<BaseTerrainStage>(m_noise);
    auto biome = std::make_unique<BiomeStage>(m_temperature, m_humidity);
    const BaseTerrainStage& baseRef  = *base;
    const BiomeStage&       biomeRef = *biome;

    m_stages.push_back(std::move(base));
    m_stages.push_back(std::move(biome));
    m_stages.push_back(std::make_unique<CaveStage>(m_caveA, m_caveB, m_caveCheese));
    m_stages.push_back(std::make_unique<OreStage>());
    m_stages.push_back(std::make_unique<FloraStage>(baseRef, biomeRef));
}

void WorldGenerator::generate(Chunk& chunk, ChunkCoord coord) const {
    const GenContext ctx(m_seed, coord);
    for (const auto& stage : m_stages) stage->apply(chunk, ctx);
}

} // namespace cw
