#pragma once

#include <cw/world/gen/GenStage.h>

namespace cw {

// Stage 3: scatters ore veins into the stone already laid down by the base
// terrain. Veins are small random-walk blobs that only ever replace Stone, so
// they never poke through the surface or overwrite soil/water. Each ore has a
// depth band and a per-biome abundance multiplier (mountains are metal-rich,
// deserts hide gold). SHARED, GL-free, and fully chunk-local: all randomness
// comes from GenContext::rng(stageId), so the result is deterministic and
// thread-safe without touching neighbouring chunks.
class OreStage : public GenStage {
public:
    void        apply(Chunk& chunk, const GenContext& ctx) const override;
    const char* name() const override { return "ores"; }
};

} // namespace cw
