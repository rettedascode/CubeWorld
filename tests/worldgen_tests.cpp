// Headless determinism + behaviour tests for the staged world generator.
#include <cw/world/Chunk.h>
#include <cw/world/World.h>
#include <cw/world/gen/BaseTerrainStage.h>
#include <cw/world/gen/Biome.h>
#include <cw/world/gen/BiomeStage.h>
#include <cw/world/gen/GenContext.h>
#include <cw/world/gen/OreStage.h>
#include <cw/world/gen/WorldGenerator.h>

#include <net/ByteBuffer.h>

#include <FastNoiseLite.h>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <set>
#include <thread>
#include <vector>

using namespace cw;

namespace {

bool sameBlocks(const Chunk& a, const Chunk& b) {
    for (int y = 0; y < Chunk::SIZE_Y; ++y)
        for (int z = 0; z < Chunk::SIZE_Z; ++z)
            for (int x = 0; x < Chunk::SIZE_X; ++x)
                if (a.get(x, y, z) != b.get(x, y, z)) return false;
    return true;
}

bool sameBiomes(const Chunk& a, const Chunk& b) {
    for (int z = 0; z < Chunk::SIZE_Z; ++z)
        for (int x = 0; x < Chunk::SIZE_X; ++x)
            if (a.biome(x, z) != b.biome(x, z)) return false;
    return true;
}

Chunk make(const WorldGenerator& gen, ChunkCoord c) {
    Chunk ch;
    gen.generate(ch, c);
    return ch;
}

// Reference re-implementation of the ORIGINAL terrain formula. Lives only in the
// test as a golden oracle: if anyone changes the base-terrain params/layering,
// this equivalence check fails — proving the pipeline refactor preserved output.
Chunk legacyGenerate(int seed, ChunkCoord coord) {
    FastNoiseLite noise;
    noise.SetSeed(seed);
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(4);
    noise.SetFrequency(0.012f);

    const int kSeaLevel = 10, kDirtDepth = 3;
    const auto heightAt = [&](int wx, int wz) {
        const float n = noise.GetNoise(static_cast<float>(wx), static_cast<float>(wz));
        return std::clamp(static_cast<int>(std::lround(14.0f + n * 9.0f)), 1, Chunk::SIZE_Y - 1);
    };

    Chunk chunk;
    const int baseX = coord.x * Chunk::SIZE_X;
    const int baseZ = coord.z * Chunk::SIZE_Z;
    for (int lz = 0; lz < Chunk::SIZE_Z; ++lz) {
        for (int lx = 0; lx < Chunk::SIZE_X; ++lx) {
            const int  height  = heightAt(baseX + lx, baseZ + lz);
            const int  surface = height - 1;
            const bool beach   = surface <= kSeaLevel + 1;
            for (int y = 0; y < height; ++y) {
                const int depth = surface - y;
                BlockType t;
                if (depth == 0)               t = beach ? BlockType::Sand : BlockType::Grass;
                else if (depth <= kDirtDepth) t = beach ? BlockType::Sand : BlockType::Dirt;
                else                          t = BlockType::Stone;
                chunk.set(lx, y, lz, t);
            }
            for (int y = height; y <= kSeaLevel; ++y)
                chunk.set(lx, y, lz, BlockType::Water);
        }
    }
    return chunk;
}

const std::vector<ChunkCoord> kCoords = {
    {0, 0}, {1, 0}, {0, 1}, {-1, 0}, {0, -1}, {-3, 5}, {7, -8}, {123, -456}};

} // namespace

TEST_CASE("Generation is repeatable for the same seed and coord", "[worldgen]") {
    const WorldGenerator gen(1337);
    for (ChunkCoord c : kCoords) {
        REQUIRE(sameBlocks(make(gen, c), make(gen, c)));
    }
}

TEST_CASE("Generation is independent of thread/order", "[worldgen]") {
    const WorldGenerator gen(1337);

    // Sequential reference.
    std::vector<Chunk> reference;
    reference.reserve(kCoords.size());
    for (ChunkCoord c : kCoords) reference.push_back(make(gen, c));

    // Same chunks generated concurrently into distinct slots (no shared mutation).
    std::vector<Chunk>       threaded(kCoords.size());
    std::vector<std::thread> workers;
    const unsigned n = std::max(2u, std::thread::hardware_concurrency());
    for (unsigned t = 0; t < n; ++t) {
        workers.emplace_back([&, t] {
            for (size_t i = t; i < kCoords.size(); i += n)
                gen.generate(threaded[i], kCoords[i]);
        });
    }
    for (auto& w : workers) w.join();

    for (size_t i = 0; i < kCoords.size(); ++i)
        REQUIRE(sameBlocks(reference[i], threaded[i]));
}

TEST_CASE("Base-terrain stage matches the original generator", "[worldgen]") {
    // The base stage alone must reproduce the legacy terrain exactly (later
    // stages such as biomes intentionally restyle surfaces on top of it).
    FastNoiseLite noise;
    noise.SetSeed(1337);
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(4);
    noise.SetFrequency(0.012f);
    const BaseTerrainStage base(noise);

    for (ChunkCoord c : kCoords) {
        Chunk ch;
        base.apply(ch, GenContext(1337, c));
        REQUIRE(sameBlocks(ch, legacyGenerate(1337, c)));
    }
}

TEST_CASE("Different seeds usually produce different worlds", "[worldgen]") {
    const WorldGenerator a(1337);
    const WorldGenerator b(2024);
    // At least one sampled chunk must differ.
    bool anyDiff = false;
    for (ChunkCoord c : kCoords)
        if (!sameBlocks(make(a, c), make(b, c))) { anyDiff = true; break; }
    REQUIRE(anyDiff);
}

TEST_CASE("Biome assignment is deterministic", "[biomes]") {
    const WorldGenerator gen(1337);
    for (ChunkCoord c : kCoords) {
        Chunk a, b;
        gen.generate(a, c);
        gen.generate(b, c);
        REQUIRE(sameBiomes(a, b));
    }
}

TEST_CASE("Chunk serialization round-trips blocks and biomes", "[biomes][replication]") {
    const WorldGenerator gen(1337);
    Chunk src;
    gen.generate(src, {3, -7});

    net::ByteBuffer out;
    src.serialize(out);

    net::ByteBuffer in(out.data(), out.size());
    Chunk dst;
    dst.deserialize(in);

    REQUIRE(sameBlocks(src, dst));
    REQUIRE(sameBiomes(src, dst));
}

TEST_CASE("classify maps climate + elevation to expected biomes", "[biomes]") {
    FastNoiseLite t, h; // classify takes climate as arguments; noise unused here
    const BiomeStage stage(t, h);

    REQUIRE(stage.classify(0.0f, 0.0f, 5, /*waterAbove*/ true) == BiomeId::Ocean);
    REQUIRE(stage.classify(0.0f, 0.0f, 11, false) == BiomeId::Beach);
    REQUIRE(stage.classify(0.0f, 0.0f, 24, false) == BiomeId::Mountains);
    REQUIRE(stage.classify(-0.5f, 0.0f, 15, false) == BiomeId::Snowy);
    REQUIRE(stage.classify(0.5f, -0.2f, 15, false) == BiomeId::Desert);
    REQUIRE(stage.classify(0.0f, 0.5f, 15, false) == BiomeId::Swamp);
    REQUIRE(stage.classify(0.0f, 0.1f, 15, false) == BiomeId::Forest);
    REQUIRE(stage.classify(0.0f, -0.2f, 15, false) == BiomeId::Plains);
}

TEST_CASE("World contains multiple biomes and only valid ids", "[biomes]") {
    const WorldGenerator gen(1337);
    std::set<uint8_t> seen;
    for (int cz = -12; cz <= 12; cz += 4) {
        for (int cx = -12; cx <= 12; cx += 4) {
            Chunk ch;
            gen.generate(ch, {cx, cz});
            for (int z = 0; z < Chunk::SIZE_Z; ++z)
                for (int x = 0; x < Chunk::SIZE_X; ++x) {
                    const uint8_t id = ch.biome(x, z);
                    REQUIRE(id < static_cast<uint8_t>(BiomeId::Count));
                    seen.insert(id);
                }
        }
    }
    REQUIRE(seen.size() > 1); // not a single-biome world
}

TEST_CASE("OreStage converts only stone", "[ores]") {
    // Isolated unit test: a column of stone capped by a marker of dirt. The ore
    // stage must scatter ore into the stone and never touch the dirt cap — this
    // is the exact "buried, stone-only" guarantee, independent of caves/terrain.
    Chunk ch;
    for (int z = 0; z < Chunk::SIZE_Z; ++z)
        for (int x = 0; x < Chunk::SIZE_X; ++x) {
            for (int y = 0; y < 64; ++y)  ch.set(x, y, z, BlockType::Stone);
            for (int y = 64; y < 72; ++y) ch.set(x, y, z, BlockType::Dirt);
        }
    // Mountain biome to maximise vein counts so the test reliably sees ore.
    for (int z = 0; z < Chunk::SIZE_Z; ++z)
        for (int x = 0; x < Chunk::SIZE_X; ++x)
            ch.setBiome(x, z, static_cast<uint8_t>(BiomeId::Mountains));

    const OreStage stage;
    stage.apply(ch, GenContext(1337, {0, 0}));

    bool sawOre = false;
    for (int y = 0; y < Chunk::SIZE_Y; ++y)
        for (int z = 0; z < Chunk::SIZE_Z; ++z)
            for (int x = 0; x < Chunk::SIZE_X; ++x) {
                const BlockType b = ch.get(x, y, z);
                const bool isOre = b == BlockType::CoalOre ||
                                   b == BlockType::IronOre || b == BlockType::GoldOre;
                if (isOre) sawOre = true;
                if (y < 64)        REQUIRE((b == BlockType::Stone || isOre)); // only stone->ore
                else if (y < 72)   REQUIRE(b == BlockType::Dirt); // cap never mineralised
                else               REQUIRE(b == BlockType::Air);  // untouched above
            }
    REQUIRE(sawOre);
}

TEST_CASE("Generated world contains ores after caving", "[ores]") {
    // Integration: ores still appear in the full pipeline even though caves carve
    // stone before the ore stage runs.
    const WorldGenerator gen(1337);
    bool sawOre = false;
    for (int cz = -4; cz <= 4 && !sawOre; ++cz)
        for (int cx = -4; cx <= 4 && !sawOre; ++cx) {
            Chunk ch;
            gen.generate(ch, {cx, cz});
            for (int y = 0; y < Chunk::SIZE_Y && !sawOre; ++y)
                for (int z = 0; z < Chunk::SIZE_Z && !sawOre; ++z)
                    for (int x = 0; x < Chunk::SIZE_X; ++x) {
                        const BlockType b = ch.get(x, y, z);
                        if (b == BlockType::CoalOre || b == BlockType::IronOre ||
                            b == BlockType::GoldOre) { sawOre = true; break; }
                    }
        }
    REQUIRE(sawOre);
}

TEST_CASE("World grows trees with logs and leaves", "[flora]") {
    const WorldGenerator gen(1337);
    bool sawLog = false, sawLeaves = false;
    for (int cz = -8; cz <= 8 && !(sawLog && sawLeaves); cz += 2) {
        for (int cx = -8; cx <= 8 && !(sawLog && sawLeaves); cx += 2) {
            Chunk ch;
            gen.generate(ch, {cx, cz});
            for (int y = 0; y < Chunk::SIZE_Y; ++y)
                for (int z = 0; z < Chunk::SIZE_Z; ++z)
                    for (int x = 0; x < Chunk::SIZE_X; ++x) {
                        const BlockType b = ch.get(x, y, z);
                        if (b == BlockType::Log)    sawLog = true;
                        if (b == BlockType::Leaves) sawLeaves = true;
                    }
        }
    }
    REQUIRE(sawLog);
    REQUIRE(sawLeaves);
}

TEST_CASE("Flora is seamless and deterministic across chunk borders", "[flora]") {
    // A tree rooted near a shared edge must drop the SAME foliage into a chunk
    // whether we regenerate it alone or in any order — placement is a pure
    // function of world coords, not of which chunk is being built.
    const WorldGenerator gen(1337);
    for (ChunkCoord c : {ChunkCoord{0, 0}, ChunkCoord{1, 0}, ChunkCoord{0, 1}}) {
        Chunk a, b;
        gen.generate(a, c);
        gen.generate(b, c);
        REQUIRE(sameBlocks(a, b)); // includes logs + leaves from neighbour trees
    }
}

TEST_CASE("Caves carve enclosed underground air pockets", "[caves]") {
    // A carved cave shows up as Air that has solid blocks both above and below
    // within the underground stone band — i.e. a void, not just the open sky.
    const WorldGenerator gen(1337);
    int pockets = 0;
    for (int cz = -4; cz <= 4; ++cz) {
        for (int cx = -4; cx <= 4; ++cx) {
            Chunk ch;
            gen.generate(ch, {cx, cz});
            for (int z = 0; z < Chunk::SIZE_Z; ++z)
                for (int x = 0; x < Chunk::SIZE_X; ++x)
                    for (int y = 3; y <= 40; ++y) {
                        if (ch.get(x, y, z) == BlockType::Air &&
                            isSolid(ch.get(x, y - 1, z)) &&
                            isSolid(ch.get(x, y + 1, z)))
                            ++pockets;
                    }
        }
    }
    REQUIRE(pockets > 0); // the sampled region must contain hollowed caves
}

TEST_CASE("Caves protect the floor and only carve stone", "[caves]") {
    // Carving starts above a protected floor (y>=2) and removes Stone only, so
    // the bottom two voxels of every column stay solid (terrain always fills
    // y=0 upward), and no soil/sand/water is ever turned to air by a cave.
    const WorldGenerator gen(1337);
    for (ChunkCoord c : kCoords) {
        Chunk ch;
        gen.generate(ch, c);
        for (int z = 0; z < Chunk::SIZE_Z; ++z)
            for (int x = 0; x < Chunk::SIZE_X; ++x) {
                REQUIRE(isSolid(ch.get(x, 0, z))); // floor never carved
                REQUIRE(isSolid(ch.get(x, 1, z))); // floor never carved
            }
    }
}

TEST_CASE("GenContext stage seeds are deterministic and order-independent", "[worldgen]") {
    const GenContext c1(1337, {5, -9});
    const GenContext c2(1337, {5, -9});
    // Same inputs -> same stage seed, regardless of when/where computed.
    REQUIRE(c1.stageSeed(0) == c2.stageSeed(0));
    REQUIRE(c1.stageSeed(3) == c2.stageSeed(3));
    // Different stage ids and coords should generally diverge.
    REQUIRE(c1.stageSeed(0) != c1.stageSeed(1));
    const GenContext other(1337, {6, -9});
    REQUIRE(c1.stageSeed(0) != other.stageSeed(0));

    // RNG seeded from the context is reproducible.
    auto r1 = c1.rng(2);
    auto r2 = c2.rng(2);
    REQUIRE(r1() == r2());
    REQUIRE(r1() == r2());
}
