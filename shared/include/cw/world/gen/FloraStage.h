#pragma once

#include <cw/world/gen/GenStage.h>

namespace cw {

class BaseTerrainStage;
class BiomeStage;

// Stage 4: scatters biome-appropriate flora (trees, spruces, cacti) on the
// surface. SHARED, GL-free, deterministic and thread-safe.
//
// Cross-chunk placement without touching neighbouring chunks: a tree's existence
// and shape are a pure hash of (worldSeed, originX, originZ), so any chunk that
// overlaps the tree computes the SAME tree. When generating a chunk we scan an
// overscanned column range (a canopy radius beyond each edge), and for every
// candidate origin — even one rooted in a neighbour — we write only the blocks
// that fall inside this chunk. Surface height + biome of neighbour columns are
// recomputed from the (const) terrain/climate noise, never read from another
// chunk's voxels, which keeps generation order- and thread-independent.
class FloraStage : public GenStage {
public:
    FloraStage(const BaseTerrainStage& terrain, const BiomeStage& biomes)
        : m_terrain(terrain), m_biomes(biomes) {}

    void        apply(Chunk& chunk, const GenContext& ctx) const override;
    const char* name() const override { return "flora"; }

private:
    const BaseTerrainStage& m_terrain; // heightAt() for any world column
    const BiomeStage&       m_biomes;  // biomeAt() for any world column
};

} // namespace cw
