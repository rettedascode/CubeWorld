#include <cw/world/gen/CaveStage.h>

#include <cw/world/Chunk.h>
#include <cw/world/gen/GenContext.h>

#include <FastNoiseLite.h>

namespace cw {

namespace {
// Depth band the carvers operate in. The floor is protected so caves never open
// straight onto y=0, and the ceiling keeps carving to the underground.
constexpr int kFloorY = 2;
constexpr int kCeilY  = 48;

// Tunnel tube: carve where both tunnel fields are simultaneously near zero. The
// joint zero-set is a 1D curve, so the swept region is a winding tube.
constexpr float kTunnelRadiusSq = 0.020f;

// Cheese cavern: carve where the FBm field rises above this (rare blobby voids).
constexpr float kCheeseThreshold = 0.62f;
} // namespace

void CaveStage::apply(Chunk& chunk, const GenContext& ctx) const {
    for (int lz = 0; lz < Chunk::SIZE_Z; ++lz) {
        for (int lx = 0; lx < Chunk::SIZE_X; ++lx) {
            const float wx = static_cast<float>(ctx.baseX + lx);
            const float wz = static_cast<float>(ctx.baseZ + lz);
            for (int y = kFloorY; y <= kCeilY; ++y) {
                // Only stone is carved: the soil/sand cap stays intact so water
                // bodies remain supported and topsoil is undisturbed.
                if (chunk.get(lx, y, lz) != BlockType::Stone) continue;

                const float wy = static_cast<float>(y);
                const float a  = m_tunnelA.GetNoise(wx, wy, wz);
                const float b  = m_tunnelB.GetNoise(wx, wy, wz);
                const bool tunnel = (a * a + b * b) < kTunnelRadiusSq;
                const bool cheese = m_cheese.GetNoise(wx, wy, wz) > kCheeseThreshold;

                if (tunnel || cheese) chunk.set(lx, y, lz, BlockType::Air);
            }
        }
    }
}

} // namespace cw
