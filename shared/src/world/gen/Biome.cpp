#include <cw/world/gen/Biome.h>

#include <array>

namespace cw {

const BiomeDef& biomeDef(BiomeId id) {
    // Indexed by BiomeId. surface/filler drive the BiomeStage; sky/foliage are
    // read by the client for tinting.
    static const std::array<BiomeDef, static_cast<size_t>(BiomeId::Count)> kBiomes = {{
        // name        surface            filler              sky                     foliage
        {"plains",    BlockType::Grass, BlockType::Dirt,  {0.45f, 0.69f, 1.00f}, {0.55f, 0.85f, 0.35f}},
        {"forest",    BlockType::Grass, BlockType::Dirt,  {0.45f, 0.69f, 1.00f}, {0.32f, 0.68f, 0.28f}},
        {"desert",    BlockType::Sand,  BlockType::Sand,  {0.62f, 0.76f, 1.00f}, {0.74f, 0.72f, 0.38f}},
        {"snowy",     BlockType::Snow,  BlockType::Dirt,  {0.72f, 0.82f, 0.95f}, {0.62f, 0.76f, 0.62f}},
        {"mountains", BlockType::Stone, BlockType::Stone, {0.52f, 0.72f, 1.00f}, {0.50f, 0.75f, 0.42f}},
        {"ocean",     BlockType::Sand,  BlockType::Sand,  {0.40f, 0.60f, 0.95f}, {0.50f, 0.80f, 0.40f}},
        {"beach",     BlockType::Sand,  BlockType::Sand,  {0.50f, 0.72f, 1.00f}, {0.55f, 0.85f, 0.35f}},
        {"swamp",     BlockType::Grass, BlockType::Dirt,  {0.49f, 0.57f, 0.55f}, {0.42f, 0.52f, 0.26f}},
    }};
    return kBiomes[static_cast<size_t>(id)];
}

} // namespace cw
