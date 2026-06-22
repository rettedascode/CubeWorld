#pragma once

#include <cw/world/gen/GenStage.h>

class FastNoiseLite;

namespace cw {

// Stage 3: carves underground caves. SHARED, GL-free, deterministic, thread-safe.
//
// Caves are PURELY POSITIONAL (sampled from 3D noise at world coords), so they
// need no stateful worm walk and no neighbour-chunk access — a tunnel that spans
// chunks is carved identically by every chunk it passes through, automatically
// seamless and order/thread-independent. Two carvers combine:
//   - tunnels: the near-zero set of two independent 3D noise fields is a winding
//     1D curve; carving a tube around it gives perlin-worm-like passages.
//   - cheese: one 3D FBm field above a threshold opens occasional blobby caverns.
//
// Only Stone is removed, within a depth band above a protected floor. The soil/
// sand cap is never touched, so water bodies stay supported and surfaces intact
// (mountains, whose surface is stone, get natural cave mouths).
class CaveStage : public GenStage {
public:
    CaveStage(const FastNoiseLite& tunnelA, const FastNoiseLite& tunnelB,
              const FastNoiseLite& cheese)
        : m_tunnelA(tunnelA), m_tunnelB(tunnelB), m_cheese(cheese) {}

    void        apply(Chunk& chunk, const GenContext& ctx) const override;
    const char* name() const override { return "caves"; }

private:
    const FastNoiseLite& m_tunnelA;
    const FastNoiseLite& m_tunnelB;
    const FastNoiseLite& m_cheese;
};

} // namespace cw
