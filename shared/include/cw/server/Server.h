#pragma once

#include <cw/server/Persistence.h>
#include <cw/sim/Simulation.h>
#include <cw/world/World.h>
#include <cw/world/gen/WorldGenerator.h>
#include <net/Host.h>

#include <atomic>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace cw {

// Authoritative game host: owns the Simulation and the fixed-rate tick loop,
// accepts connections and answers the handshake + ping protocol.
//
// SHARED so it can be run two ways from ONE code path:
//   - the dedicated `server` executable (blocking run() on the main thread), and
//   - the client's IntegratedServer (run() on a background thread for singleplayer).
// No rendering — the sim is headless.
class Server {
public:
    bool start(uint16_t port);
    void run();  // blocking fixed-tick loop until stop()
    void stop(); // thread-safe: asks run() to exit (used by the integrated host)

private:
    void pollNetwork();
    void handlePacket(uint32_t peer, net::ByteBuffer& data);
    void onDisconnect(uint32_t peer);
    void sendSnapshots();
    void streamChunks(); // generate + send chunks around each player
    void pickupItems();  // players collect nearby item entities
    void updateSurvival(); // hunger drain, starvation, regen, death/respawn
    void spawnHostiles();  // periodically spawn hostile mobs near players
    void sendStats(uint32_t peer, EntityId eid);
    void respawn(Entity& e);
    void broadcastBlockUpdate(ChunkCoord cc, int x, int y, int z, BlockType block);

    Chunk*    ensureChunk(ChunkCoord c); // load (edited or generated) into the world
    glm::vec3 findSpawnPosition(int x, int z); // ground feet position for (x,z)
    void      savePlayer(uint32_t peer); // persist one player's data
    void   saveAll();                 // flush regions + all online players

    void broadcastChat(const std::string& sender, const std::string& text);
    void sendChat(uint32_t peer, const std::string& text); // system line to one client
    void broadcastPlayerList();
    void handleCommand(uint32_t peer, const std::string& line); // "/..." from chat

    net::Host         m_host;
    Simulation        m_sim;
    WorldGenerator    m_worldGen;
    std::atomic<bool> m_running{false};

    std::unordered_map<uint32_t, EntityId> m_playerEntity;  // peerId -> entity id
    std::unordered_map<uint32_t, uint32_t> m_lastInputSeq;  // peerId -> last applied input

    // Chunks already sent to each peer (so we don't resend).
    std::unordered_map<uint32_t, std::unordered_set<ChunkCoord, ChunkCoordHash>> m_sentChunks;

    bool m_mobsSpawned = false; // spawn ambient mobs once, on first join

    RegionStore m_regions{"world/region"};
    PlayerStore m_playerStore{"world/players"};
    std::unordered_map<uint32_t, std::string> m_playerName; // peerId -> name (for saving)
};

} // namespace cw
