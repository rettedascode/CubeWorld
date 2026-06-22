#include <cw/world/Chunk.h>

#include <net/ByteBuffer.h>

namespace cw {

void Chunk::serialize(net::ByteBuffer& b) const {
    // Blocks (RLE over the flat volume).
    int i = 0;
    while (i < VOLUME) {
        const BlockType type = m_blocks[i];
        int j = i + 1;
        while (j < VOLUME && m_blocks[j] == type) ++j;
        b.writeU32(static_cast<uint32_t>(j - i));   // run length
        b.writeU8(static_cast<uint8_t>(type));       // block type
        i = j;
    }

    // Biomes (RLE over the per-column area) — appended in protocol v14.
    int a = 0;
    while (a < AREA) {
        const uint8_t id = m_biomes[a];
        int j = a + 1;
        while (j < AREA && m_biomes[j] == id) ++j;
        b.writeU32(static_cast<uint32_t>(j - a));
        b.writeU8(id);
        a = j;
    }
}

void Chunk::deserialize(net::ByteBuffer& b) {
    int i = 0;
    while (i < VOLUME && b.ok()) {
        const uint32_t  run  = b.readU32();
        const BlockType type = static_cast<BlockType>(b.readU8());
        for (uint32_t k = 0; k < run && i < VOLUME; ++k) m_blocks[i++] = type;
    }

    int a = 0;
    while (a < AREA && b.ok()) {
        const uint32_t run = b.readU32();
        const uint8_t  id  = b.readU8();
        for (uint32_t k = 0; k < run && a < AREA; ++k) m_biomes[a++] = id;
    }
}

Chunk::Chunk() {
    m_blocks.fill(BlockType::Air);
    m_biomes.fill(0);
}

BlockType Chunk::get(int x, int y, int z) const {
    if (!inBounds(x, y, z)) return BlockType::Air;
    return m_blocks[index(x, y, z)];
}

void Chunk::set(int x, int y, int z, BlockType type) {
    if (!inBounds(x, y, z)) return;
    m_blocks[index(x, y, z)] = type;
}

uint8_t Chunk::biome(int x, int z) const {
    if (x < 0 || x >= SIZE_X || z < 0 || z >= SIZE_Z) return 0;
    return m_biomes[x + SIZE_X * z];
}

void Chunk::setBiome(int x, int z, uint8_t id) {
    if (x < 0 || x >= SIZE_X || z < 0 || z >= SIZE_Z) return;
    m_biomes[x + SIZE_X * z] = id;
}

} // namespace cw
