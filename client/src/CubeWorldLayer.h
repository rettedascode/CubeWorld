#pragma once

#include "net/IntegratedServer.h"
#include "net/NetClient.h"
#include "player/PlayerController.h"
#include "render/EntityRenderer.h"
#include "render/HudRenderer.h"

#include <cw/world/World.h>

#include <ce/core/Layer.h>

#include <glm/glm.hpp>

#include <array>
#include <climits>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace ce {
class Shader;
class Mesh;
class Texture2D;
} // namespace ce

namespace cw {

// B3: the world is owned by the server and streamed to the client. The client
// inserts received chunks, meshes them (re-meshing seams as neighbours arrive),
// and unloads chunks beyond the view distance.
class CubeWorldLayer : public ce::Layer {
public:
    // Singleplayer (default) hosts an integrated server and connects to loopback.
    // Pass singleplayer=false + a host to connect to a remote dedicated server.
    explicit CubeWorldLayer(std::string serverHost = "127.0.0.1", bool singleplayer = true);
    ~CubeWorldLayer() override; // defined in .cpp where the engine types are complete

    void onAttach() override;
    void onDetach() override;
    void onUpdate(float dt) override;
    void onRender() override;
    void onEvent(ce::Event& e) override;

private:
    void updateProjection(uint32_t width, uint32_t height);

    void updateMining(float dt);     // progressive mining while LMB is held
    EntityId pickTargetMob() const;  // mob in the crosshair within reach (0 = none)
    void placeBlock();
    void onBlockChanged(int wx, int wy, int wz); // mark owner + border neighbours dirty

    ChunkCoord playerChunk() const;
    void       markDirty(ChunkCoord c);          // queue c (+ loaded neighbours) to remesh
    void       processDirtyMeshes(int maxPerFrame);
    void       unloadFar(ChunkCoord center);

    World m_world;
    std::unordered_map<ChunkCoord, std::unique_ptr<ce::Mesh>, ChunkCoordHash> m_meshes;
    std::unordered_set<ChunkCoord, ChunkCoordHash> m_dirty; // chunks needing a (re)mesh

    std::unique_ptr<ce::Shader>       m_shader;
    std::unique_ptr<ce::Texture2D>    m_texture;
    PlayerController                   m_player;
    EntityRenderer                    m_entityRenderer;
    HudRenderer                       m_hud;
    uint32_t                          m_screenW = 1280;
    uint32_t                          m_screenH = 720;

    // Networking. In singleplayer the client hosts an in-process server and
    // connects to it over loopback — the same path used for multiplayer.
    std::string      m_serverHost   = "127.0.0.1";
    bool             m_singleplayer = true;
    IntegratedServer m_integrated;
    NetClient        m_net;

    ChunkCoord m_lastChunk{INT_MAX, INT_MAX}; // force unload check on first update

    int  m_selectedSlot = 0; // active hotbar slot (0..8), set by scroll / number keys
    bool m_crafting     = false; // crafting overlay open (cursor freed)

    // Chat input state.
    bool        m_chatOpen    = false;
    bool        m_swallowChar = false; // ignore the keystroke that opened chat
    std::string m_chatInput;

    // Progressive mining state.
    glm::ivec3 m_mineTarget{0};
    float      m_mineProgress  = 0.0f;
    bool       m_hasMineTarget = false;
};

} // namespace cw
