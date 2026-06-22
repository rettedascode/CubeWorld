#include <cw/world/gen/FloraStage.h>

#include <cw/world/Chunk.h>
#include <cw/world/gen/BaseTerrainStage.h> // heightAt + kSeaLevel
#include <cw/world/gen/Biome.h>
#include <cw/world/gen/BiomeStage.h>
#include <cw/world/gen/GenContext.h> // mix64

#include <cstdlib> // std::abs

namespace cw {

namespace {

// Salt the column hash so flora randomness is independent of other stages.
constexpr uint32_t kFloraSalt = 0x5F10A;

// Leaves reach 2 blocks out from the trunk, so a tree rooted up to 2 columns
// outside a chunk can still drop foliage inside it. We overscan by this much.
constexpr int kCanopyRadius = 2;

// Deterministic per-column hash: identical for a world column no matter which
// chunk (or thread) evaluates it, so trees straddling a border are seamless.
uint64_t hashColumn(int32_t seed, int wx, int wz, uint32_t salt) {
    uint64_t h = static_cast<uint64_t>(static_cast<uint32_t>(seed));
    h = mix64(h ^ (static_cast<uint64_t>(static_cast<uint32_t>(wx)) * 0x9E3779B97F4A7C15ull));
    h = mix64(h ^ (static_cast<uint64_t>(static_cast<uint32_t>(wz)) * 0xC2B2AE3D27D4EB4Full));
    h = mix64(h ^ static_cast<uint64_t>(salt));
    return h;
}

// Top 53 bits of a 64-bit hash mapped to [0,1).
double hash01(uint64_t h) {
    return static_cast<double>(h >> 11) * (1.0 / 9007199254740992.0);
}

// Probability per surface column that a tree/cactus is rooted there. 0 = none.
double treeDensity(BiomeId biome) {
    switch (biome) {
        case BiomeId::Forest:    return 0.060;
        case BiomeId::Swamp:     return 0.040;
        case BiomeId::Snowy:     return 0.020;
        case BiomeId::Desert:    return 0.012; // cacti
        case BiomeId::Plains:    return 0.006;
        case BiomeId::Mountains: return 0.004;
        default:                 return 0.0;   // ocean / beach
    }
}

} // namespace

void FloraStage::apply(Chunk& chunk, const GenContext& ctx) const {
    // Write a block into THIS chunk only; world coords outside it are ignored,
    // which is how a neighbour-rooted tree contributes just its overlapping part.
    const auto put = [&](int wx, int y, int wz, BlockType b, bool onlyAir) {
        const int lx = wx - ctx.baseX;
        const int lz = wz - ctx.baseZ;
        if (lx < 0 || lx >= Chunk::SIZE_X) return;
        if (lz < 0 || lz >= Chunk::SIZE_Z) return;
        if (y < 0 || y >= Chunk::SIZE_Y) return;
        if (onlyAir && chunk.get(lx, y, lz) != BlockType::Air) return;
        chunk.set(lx, y, lz, b);
    };

    const int loX = ctx.baseX - kCanopyRadius;
    const int hiX = ctx.baseX + Chunk::SIZE_X + kCanopyRadius;
    const int loZ = ctx.baseZ - kCanopyRadius;
    const int hiZ = ctx.baseZ + Chunk::SIZE_Z + kCanopyRadius;

    for (int wz = loZ; wz < hiZ; ++wz) {
        for (int wx = loX; wx < hiX; ++wx) {
            const int surfaceY = m_terrain.heightAt(wx, wz) - 1;
            if (surfaceY <= BaseTerrainStage::kSeaLevel) continue; // underwater / shore

            const BiomeId biome = m_biomes.biomeAt(wx, wz, surfaceY, /*waterAbove*/ false);
            const double density = treeDensity(biome);
            if (density <= 0.0) continue;

            const uint64_t h = hashColumn(ctx.worldSeed, wx, wz, kFloraSalt);
            if (hash01(h) >= density) continue; // no plant rooted here

            const uint64_t shape = mix64(h); // independent stream for dimensions

            if (biome == BiomeId::Desert) {
                const int height = 2 + static_cast<int>(shape % 3); // 2..4
                for (int i = 1; i <= height; ++i)
                    put(wx, surfaceY + i, wz, BlockType::Cactus, true);
                continue;
            }

            const bool spruce  = (biome == BiomeId::Snowy);
            const int  trunkH  = (spruce ? 5 : 4) + static_cast<int>(shape % 3); // 4..6 / 5..7
            for (int i = 1; i <= trunkH; ++i)
                put(wx, surfaceY + i, wz, BlockType::Log, true);

            // Canopy: two wide layers (radius 2, corners trimmed) topped by a
            // small cap. Leaves only fill Air, so they never eat the trunk.
            const int top = surfaceY + trunkH;
            const auto leafLayer = [&](int y, int radius, bool trimCorners) {
                for (int dz = -radius; dz <= radius; ++dz)
                    for (int dx = -radius; dx <= radius; ++dx) {
                        if (trimCorners && std::abs(dx) == radius && std::abs(dz) == radius)
                            continue;
                        put(wx + dx, y, wz + dz, BlockType::Leaves, true);
                    }
            };
            leafLayer(top - 1, 2, true);
            leafLayer(top,     2, true);
            leafLayer(top + 1, 1, false);
            put(wx, top + 2, wz, BlockType::Leaves, true);
        }
    }
}

} // namespace cw
