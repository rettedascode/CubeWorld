#include <cw/world/gen/OreStage.h>

#include <cw/world/Chunk.h>
#include <cw/world/gen/Biome.h>
#include <cw/world/gen/GenContext.h>

#include <algorithm>
#include <array>

namespace cw {

namespace {

// Unique stage id so the ore RNG never collides with another stage's stream.
constexpr uint32_t kOreStageId = 3;

struct OreKind {
    BlockType block;
    int       yMin, yMax;  // inclusive depth band the vein may seed in
    int       baseVeins;   // veins per chunk before the biome multiplier
    int       veinSize;    // blocks grown per vein
};

// Coal is common and shallow; iron sits mid-depth; gold is rare and deep.
constexpr std::array<OreKind, 3> kOres = {{
    {BlockType::CoalOre, 3, 50, 10, 7},
    {BlockType::IronOre, 2, 30, 6,  5},
    {BlockType::GoldOre, 1, 14, 3,  4},
}};

// Per-biome abundance multiplier for each ore (index matches kOres). Mountains
// expose rich metal seams; deserts hide extra gold; snowy peaks have more coal.
float oreMultiplier(BiomeId biome, size_t oreIndex) {
    switch (biome) {
        case BiomeId::Mountains:
            return oreIndex == 0 ? 1.5f : 2.0f; // +coal, ++iron/gold
        case BiomeId::Desert:
            return oreIndex == 2 ? 1.8f : 1.0f; // ++gold
        case BiomeId::Snowy:
            return oreIndex == 0 ? 1.3f : 1.0f; // +coal
        default:
            return 1.0f;
    }
}

} // namespace

void OreStage::apply(Chunk& chunk, const GenContext& ctx) const {
    std::mt19937 rng = ctx.rng(kOreStageId);

    // Weight abundance by the chunk's central biome (cheap, stable proxy).
    const BiomeId biome =
        biomeFromId(chunk.biome(Chunk::SIZE_X / 2, Chunk::SIZE_Z / 2));

    for (size_t oi = 0; oi < kOres.size(); ++oi) {
        const OreKind& ore = kOres[oi];
        const int veins =
            static_cast<int>(ore.baseVeins * oreMultiplier(biome, oi) + 0.5f);
        const int yHi = std::min(ore.yMax, Chunk::SIZE_Y - 1);
        if (ore.yMin > yHi) continue;

        std::uniform_int_distribution<int> dx(0, Chunk::SIZE_X - 1);
        std::uniform_int_distribution<int> dz(0, Chunk::SIZE_Z - 1);
        std::uniform_int_distribution<int> dy(ore.yMin, yHi);
        std::uniform_int_distribution<int> dir(0, 5);

        for (int v = 0; v < veins; ++v) {
            int x = dx(rng), y = dy(rng), z = dz(rng);
            // Random-walk blob; only stone is converted so veins stay buried.
            for (int b = 0; b < ore.veinSize; ++b) {
                if (Chunk::inBounds(x, y, z) &&
                    chunk.get(x, y, z) == BlockType::Stone) {
                    chunk.set(x, y, z, ore.block);
                }
                switch (dir(rng)) {
                    case 0: ++x; break;
                    case 1: --x; break;
                    case 2: ++y; break;
                    case 3: --y; break;
                    case 4: ++z; break;
                    default: --z; break;
                }
            }
        }
    }
}

} // namespace cw
