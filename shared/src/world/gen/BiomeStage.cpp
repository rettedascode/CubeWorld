#include <cw/world/gen/BiomeStage.h>

#include <cw/world/Chunk.h>
#include <cw/world/gen/BaseTerrainStage.h> // kSeaLevel
#include <cw/world/gen/GenContext.h>

#include <FastNoiseLite.h>

namespace cw {

namespace {
constexpr int kSeaLevel      = BaseTerrainStage::kSeaLevel;
constexpr int kMountainLevel = kSeaLevel + 11; // surfaces this high read as mountains
constexpr int kFillerDepth   = 3;
} // namespace

BiomeId BiomeStage::classify(float temperature, float humidity, int surfaceY,
                             bool waterAbove) const {
    if (waterAbove)                  return BiomeId::Ocean;
    if (surfaceY <= kSeaLevel + 1)   return BiomeId::Beach;
    if (surfaceY >= kMountainLevel)  return BiomeId::Mountains;

    if (temperature < -0.35f)                          return BiomeId::Snowy;
    if (temperature > 0.35f && humidity < -0.05f)      return BiomeId::Desert;
    if (humidity > 0.30f)                              return BiomeId::Swamp;
    if (humidity > 0.00f)                              return BiomeId::Forest;
    return BiomeId::Plains;
}

BiomeId BiomeStage::biomeAt(int worldX, int worldZ, int surfaceY, bool waterAbove) const {
    const float temp = m_temp.GetNoise(static_cast<float>(worldX),
                                       static_cast<float>(worldZ));
    const float hum  = m_humidity.GetNoise(static_cast<float>(worldX),
                                           static_cast<float>(worldZ));
    return classify(temp, hum, surfaceY, waterAbove);
}

void BiomeStage::apply(Chunk& chunk, const GenContext& ctx) const {
    for (int lz = 0; lz < Chunk::SIZE_Z; ++lz) {
        for (int lx = 0; lx < Chunk::SIZE_X; ++lx) {
            // Highest solid (opaque) block = land/ocean-floor surface.
            int surfaceY = 0;
            for (int y = Chunk::SIZE_Y - 1; y >= 0; --y) {
                if (isOpaque(chunk.get(lx, y, lz))) { surfaceY = y; break; }
            }
            const bool waterAbove = chunk.get(lx, surfaceY + 1, lz) == BlockType::Water;

            const float temp = m_temp.GetNoise(static_cast<float>(ctx.baseX + lx),
                                               static_cast<float>(ctx.baseZ + lz));
            const float hum  = m_humidity.GetNoise(static_cast<float>(ctx.baseX + lx),
                                                   static_cast<float>(ctx.baseZ + lz));

            const BiomeId id = classify(temp, hum, surfaceY, waterAbove);
            chunk.setBiome(lx, lz, static_cast<uint8_t>(id));

            // Ocean/beach keep the base sand/water shoreline; land biomes restyle
            // their surface + filler columns.
            if (id == BiomeId::Ocean || id == BiomeId::Beach) continue;

            const BiomeDef& def = biomeDef(id);
            chunk.set(lx, surfaceY, lz, def.surface);
            for (int d = 1; d <= kFillerDepth; ++d) {
                const int y = surfaceY - d;
                if (y < 0) break;
                if (chunk.get(lx, y, lz) == BlockType::Stone) break; // don't recolour rock
                chunk.set(lx, y, lz, def.filler);
            }
        }
    }
}

} // namespace cw
