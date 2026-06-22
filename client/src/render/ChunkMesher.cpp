#include "render/ChunkMesher.h"

#include "render/BlockTextures.h"

#include <cw/world/Chunk.h>
#include <cw/world/World.h>
#include <cw/world/gen/Biome.h>

#include <ce/render/BufferLayout.h>
#include <ce/render/Mesh.h>
#include <ce/util/Log.h>

#include <cstdint>
#include <vector>

namespace cw {

namespace {

// Which texture a face samples from its block: a side, the top, or the bottom.
enum class FaceSlot { Side, Top, Bottom };

// One cube face: the neighbour direction to test for culling, a baked
// brightness (cheap directional shading), which texture slot it uses, and the
// 4 corner offsets within the block's unit cell. Corners are wound counter-
// clockwise as seen from outside to match the engine's GL_CCW front-face
// convention.
struct FaceDef {
    int      dx, dy, dz;
    float    brightness;
    FaceSlot slot;
    float    corner[4][3];
};

const FaceDef kFaces[6] = {
    // +Z (front)
    { 0,  0,  1, 0.8f, FaceSlot::Side,   {{0,0,1}, {1,0,1}, {1,1,1}, {0,1,1}}},
    // -Z (back)
    { 0,  0, -1, 0.8f, FaceSlot::Side,   {{1,0,0}, {0,0,0}, {0,1,0}, {1,1,0}}},
    // -X (left)
    {-1,  0,  0, 0.6f, FaceSlot::Side,   {{0,0,0}, {0,0,1}, {0,1,1}, {0,1,0}}},
    // +X (right)
    { 1,  0,  0, 0.6f, FaceSlot::Side,   {{1,0,1}, {1,0,0}, {1,1,0}, {1,1,1}}},
    // +Y (top, brightest)
    { 0,  1,  0, 1.0f, FaceSlot::Top,    {{0,1,1}, {1,1,1}, {1,1,0}, {0,1,0}}},
    // -Y (bottom, darkest)
    { 0, -1,  0, 0.4f, FaceSlot::Bottom, {{0,0,0}, {1,0,0}, {1,0,1}, {0,0,1}}},
};

// Per-corner (u,v) flags in [0,1] selecting the tile sub-rect's edges.
const float kCornerUV[4][2] = {{0,0}, {1,0}, {1,1}, {0,1}};

int tileForFace(BlockType block, FaceSlot slot) {
    const BlockTextures tex = blockTextures(block);
    switch (slot) {
        case FaceSlot::Top:    return tex.top;
        case FaceSlot::Bottom: return tex.bottom;
        default:               return tex.side;
    }
}

// Atlas UV rect for a tile. Half-texel inset prevents sampling neighbouring
// tiles at the edges; the v axis is flipped because stb loads with a vertical
// flip (PNG row 0 is the TOP, which maps to the high-v end of the texture).
struct UVRect { float u0, v0, u1, v1; };

UVRect tileRect(int tile) {
    const float span  = 1.0f / kAtlasCols;
    const float inset = 0.5f / (kAtlasCols * kAtlasTilePixels);

    const int col = tile % kAtlasCols;
    const int row = tile / kAtlasCols; // measured from the top of the PNG

    const float u0 = col * span + inset;
    const float u1 = (col + 1) * span - inset;
    const float vTop = 1.0f - row * span;          // top edge of tile
    const float vBot = 1.0f - (row + 1) * span;     // bottom edge of tile
    return {u0, vBot + inset, u1, vTop - inset};
}

} // namespace

std::unique_ptr<ce::Mesh> ChunkMesher::build(const World& world, ChunkCoord coord) {
    const Chunk* chunk = world.getChunk(coord);

    std::vector<float>    vertices; // x,y,z,u,v,brightness,r,g,b per vertex (9)
    std::vector<uint32_t> indices;
    constexpr int kFloatsPerVertex = 9;

    const int baseX = coord.x * Chunk::SIZE_X; // chunk origin in world space
    const int baseZ = coord.z * Chunk::SIZE_Z;

    for (int y = 0; chunk && y < Chunk::SIZE_Y; ++y) {
        for (int z = 0; z < Chunk::SIZE_Z; ++z) {
            for (int x = 0; x < Chunk::SIZE_X; ++x) {
                const BlockType block = chunk->get(x, y, z);
                if (!isSolid(block)) continue;

                // Foliage tint: grass and leaves take the column's biome foliage
                // colour; everything else is untinted (white).
                Rgb tint{1.0f, 1.0f, 1.0f};
                if (block == BlockType::Grass || block == BlockType::Leaves)
                    tint = biomeDef(biomeFromId(chunk->biome(x, z))).foliage;

                for (const FaceDef& face : kFaces) {
                    // Cull the face if an opaque neighbour hides it, or if the
                    // neighbour is the same (non-opaque) type — e.g. water next
                    // to water needs no internal face. Neighbours are queried in
                    // world space so they resolve across chunk borders.
                    const BlockType neighbour = world.getBlock(
                        baseX + x + face.dx, y + face.dy, baseZ + z + face.dz);
                    if (isOpaque(neighbour) || neighbour == block)
                        continue;

                    const UVRect uv = tileRect(tileForFace(block, face.slot));

                    const uint32_t base =
                        static_cast<uint32_t>(vertices.size() / kFloatsPerVertex);
                    for (int i = 0; i < 4; ++i) {
                        vertices.push_back(x + face.corner[i][0]);
                        vertices.push_back(y + face.corner[i][1]);
                        vertices.push_back(z + face.corner[i][2]);
                        vertices.push_back(kCornerUV[i][0] == 0.0f ? uv.u0 : uv.u1);
                        vertices.push_back(kCornerUV[i][1] == 0.0f ? uv.v0 : uv.v1);
                        vertices.push_back(face.brightness);
                        vertices.push_back(tint.r);
                        vertices.push_back(tint.g);
                        vertices.push_back(tint.b);
                    }
                    indices.push_back(base + 0);
                    indices.push_back(base + 1);
                    indices.push_back(base + 2);
                    indices.push_back(base + 2);
                    indices.push_back(base + 3);
                    indices.push_back(base + 0);
                }
            }
        }
    }

    // The GAME defines the block vertex layout; the engine stays format-agnostic.
    const ce::BufferLayout layout = {
        {ce::ShaderDataType::Float3, "a_position"},
        {ce::ShaderDataType::Float2, "a_texCoord"},
        {ce::ShaderDataType::Float,  "a_brightness"},
        {ce::ShaderDataType::Float3, "a_color"},
    };

    CE_LOG_INFO("Meshed chunk (", coord.x, ",", coord.z, "): ",
                indices.size() / 6, " faces, ",
                vertices.size() / kFloatsPerVertex, " vertices");

    return std::make_unique<ce::Mesh>(
        vertices.data(), static_cast<uint32_t>(vertices.size() * sizeof(float)),
        indices.data(),  static_cast<uint32_t>(indices.size()), layout);
}

} // namespace cw
