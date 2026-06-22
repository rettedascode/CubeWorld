#include <cw/world/World.h>

namespace cw {

Chunk* World::getChunk(ChunkCoord c) {
    auto it = m_chunks.find(c);
    return it == m_chunks.end() ? nullptr : it->second.get();
}

const Chunk* World::getChunk(ChunkCoord c) const {
    auto it = m_chunks.find(c);
    return it == m_chunks.end() ? nullptr : it->second.get();
}

Chunk& World::createChunk(ChunkCoord c) {
    auto& slot = m_chunks[c];
    if (!slot) slot = std::make_unique<Chunk>();
    return *slot;
}

void World::removeChunk(ChunkCoord c) { m_chunks.erase(c); }

BlockType World::getBlock(int wx, int wy, int wz) const {
    const Chunk* chunk = getChunk(worldToChunk(wx, wz));
    if (!chunk) return BlockType::Air;
    return chunk->get(floorMod(wx, Chunk::SIZE_X), wy, floorMod(wz, Chunk::SIZE_Z));
}

void World::setBlock(int wx, int wy, int wz, BlockType type) {
    Chunk& chunk = createChunk(worldToChunk(wx, wz));
    chunk.set(floorMod(wx, Chunk::SIZE_X), wy, floorMod(wz, Chunk::SIZE_Z), type);
}

uint8_t World::getBiome(int wx, int wz) const {
    const Chunk* chunk = getChunk(worldToChunk(wx, wz));
    if (!chunk) return 0;
    return chunk->biome(floorMod(wx, Chunk::SIZE_X), floorMod(wz, Chunk::SIZE_Z));
}

} // namespace cw
