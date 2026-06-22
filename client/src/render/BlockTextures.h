#pragma once

#include <cw/world/Block.h>

// CLIENT-side rendering data: maps a (headless) BlockType to texture atlas tiles.
// This deliberately lives in the client, not in shared, so the headless server
// never depends on texture/atlas concepts.
namespace cw {

// Texture atlas geometry (see tools/make_atlas.py). Square grid of tiles.
inline constexpr int kAtlasCols       = 4;
inline constexpr int kAtlasTilePixels = 16;

// Atlas tile index used by each face of a block. Many blocks use the same tile
// on all faces; grass differs (green top, grassy side, dirt bottom).
struct BlockTextures {
    int top;
    int side;
    int bottom;
};

inline BlockTextures blockTextures(BlockType t) {
    switch (t) {
        case BlockType::Grass: return {0, 1, 2}; // grass_top / grass_side / dirt
        case BlockType::Dirt:  return {2, 2, 2};
        case BlockType::Stone: return {3, 3, 3};
        case BlockType::Sand:  return {4, 4, 4};
        case BlockType::Water:   return {5, 5, 5};
        case BlockType::Snow:    return {6, 6, 6};
        case BlockType::Log:     return {8, 7, 8};    // rings top/bottom, bark side
        case BlockType::Leaves:  return {9, 9, 9};    // foliage-tinted in the mesher
        case BlockType::Cactus:  return {10, 10, 10};
        case BlockType::CoalOre: return {11, 11, 11};
        case BlockType::IronOre: return {12, 12, 12};
        case BlockType::GoldOre: return {13, 13, 13};
        default:                 return {3, 3, 3}; // fallback: stone
    }
}

} // namespace cw
