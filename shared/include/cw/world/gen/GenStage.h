#pragma once

namespace cw {

class Chunk;
struct GenContext;

// One stage of the staged worldgen pipeline (heightmap, biomes, caves, ores,
// flora, structures, ...). SHARED, GL-free. Stages must be:
//   - deterministic: output depends only on the Chunk + GenContext, and
//   - stateless/immutable: hold only const config, so the same WorldGenerator
//     can run stages on multiple chunks concurrently with no data races.
class GenStage {
public:
    virtual ~GenStage() = default;

    virtual void        apply(Chunk& chunk, const GenContext& ctx) const = 0;
    virtual const char* name() const = 0;
};

} // namespace cw
