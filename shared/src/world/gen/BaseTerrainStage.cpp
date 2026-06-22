#include <cw/world/gen/BaseTerrainStage.h>

#include <cw/world/Chunk.h>
#include <cw/world/gen/GenContext.h>

#include <FastNoiseLite.h>

#include <algorithm>
#include <cmath>

namespace cw {

namespace {
// Terrain shaping parameters (unchanged from the original TerrainGenerator).
constexpr float kBaseHeight = 14.0f; // average surface height
constexpr float kAmplitude  = 9.0f;  // +/- height variation
constexpr int   kDirtDepth  = 3;     // soil layers below the surface block
} // namespace

int BaseTerrainStage::heightAt(int worldX, int worldZ) const {
    const float n = m_noise.GetNoise(static_cast<float>(worldX),
                                     static_cast<float>(worldZ));
    const int h = static_cast<int>(std::lround(kBaseHeight + n * kAmplitude));
    return std::clamp(h, 1, Chunk::SIZE_Y - 1);
}

void BaseTerrainStage::apply(Chunk& chunk, const GenContext& ctx) const {
    for (int lz = 0; lz < Chunk::SIZE_Z; ++lz) {
        for (int lx = 0; lx < Chunk::SIZE_X; ++lx) {
            const int height  = heightAt(ctx.baseX + lx, ctx.baseZ + lz);
            const int surface = height - 1;
            const bool beach  = surface <= kSeaLevel + 1; // sand at/near waterline

            for (int y = 0; y < height; ++y) {
                const int depth = surface - y;
                BlockType type;
                if (depth == 0)               type = beach ? BlockType::Sand : BlockType::Grass;
                else if (depth <= kDirtDepth) type = beach ? BlockType::Sand : BlockType::Dirt;
                else                          type = BlockType::Stone;
                chunk.set(lx, y, lz, type);
            }

            for (int y = height; y <= kSeaLevel; ++y)
                chunk.set(lx, y, lz, BlockType::Water);
        }
    }
}

} // namespace cw
