#pragma once

#include <cw/world/World.h>

#include <memory>

namespace ce { class Mesh; }

namespace cw {

// Turns one chunk's voxels into a single engine Mesh, emitting a face only when
// the neighbouring block in that direction is non-opaque (face culling).
// Neighbours are queried through the World, so faces on chunk borders are
// culled against the adjacent chunk and seams stitch seamlessly.
//
// Positions are emitted in chunk-LOCAL space (0..16); the caller renders each
// chunk mesh with a translation to its world position.
//
// This is the expensive step: run it only when a chunk changes, never per frame.
class ChunkMesher {
public:
    static std::unique_ptr<ce::Mesh> build(const World& world, ChunkCoord coord);
};

} // namespace cw
