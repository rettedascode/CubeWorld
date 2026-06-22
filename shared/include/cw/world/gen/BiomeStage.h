#pragma once

#include <cw/world/gen/Biome.h>
#include <cw/world/gen/GenStage.h>

class FastNoiseLite;

namespace cw {

// Stage 2: assigns a biome per column from continuous temperature + humidity
// noise (Whittaker-style) with elevation overrides (ocean/beach/mountains), then
// rewrites surface/filler blocks per biome and records the biome id in the chunk.
// SHARED, deterministic; reads two shared const noise instances.
class BiomeStage : public GenStage {
public:
    explicit BiomeStage(const FastNoiseLite& temperature, const FastNoiseLite& humidity)
        : m_temp(temperature), m_humidity(humidity) {}

    void        apply(Chunk& chunk, const GenContext& ctx) const override;
    const char* name() const override { return "biomes"; }

    // Classify a column from its climate + surface height. Public for tests.
    BiomeId classify(float temperature, float humidity, int surfaceY, bool waterAbove) const;

    // Sample this stage's climate noise at a world column and classify it. Lets
    // later stages (e.g. flora) recompute the biome of any column — including
    // neighbours of the chunk being generated — without touching other chunks.
    BiomeId biomeAt(int worldX, int worldZ, int surfaceY, bool waterAbove) const;

private:
    const FastNoiseLite& m_temp;
    const FastNoiseLite& m_humidity;
};

} // namespace cw
