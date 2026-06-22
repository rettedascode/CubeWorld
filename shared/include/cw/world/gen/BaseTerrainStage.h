#pragma once

#include <cw/world/gen/GenStage.h>

class FastNoiseLite; // vendored single header, global namespace

namespace cw {

// Stage 1: base terrain. Reproduces the original heightmap + layered fill
// (stone / dirt / grass, sand near the waterline, water in lows) exactly, so the
// world is unchanged by the pipeline refactor. Reads a shared const noise
// instance (concurrency-safe) plus the per-column world coordinates.
class BaseTerrainStage : public GenStage {
public:
    static constexpr int kSeaLevel = 10;

    explicit BaseTerrainStage(const FastNoiseLite& noise) : m_noise(noise) {}

    void        apply(Chunk& chunk, const GenContext& ctx) const override;
    const char* name() const override { return "base_terrain"; }

    int heightAt(int worldX, int worldZ) const;

private:
    const FastNoiseLite& m_noise;
};

} // namespace cw
