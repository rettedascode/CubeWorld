#include "CubeWorldLayer.h"

#include "player/VoxelRaycast.h"
#include "render/ChunkMesher.h"

#include <cw/proto/Protocol.h>    // kDefaultPort, kViewDistance
#include <cw/sim/DayCycle.h>      // dayBrightness
#include <cw/world/gen/Biome.h>   // biome sky tint

#include <ce/event/ApplicationEvent.h>
#include <ce/event/KeyEvent.h>
#include <ce/event/MouseEvent.h>
#include <ce/input/Input.h>
#include <ce/render/Mesh.h>
#include <ce/render/Renderer.h>
#include <ce/render/Shader.h>
#include <ce/render/Texture.h>
#include <ce/util/Log.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <cstdlib>
#include <iterator>
#include <string>
#include <vector>

namespace cw {

namespace {

constexpr float kReach              = 8.0f; // block interaction distance
constexpr int   kMeshesPerFrame     = 6;    // dirty chunks meshed per frame (spreads cost)

std::string asset(const std::string& relative) {
    return std::string(CW_ASSET_DIR) + "/" + relative;
}

} // namespace

CubeWorldLayer::CubeWorldLayer(std::string serverHost, bool singleplayer)
    : m_serverHost(std::move(serverHost)), m_singleplayer(singleplayer) {}
CubeWorldLayer::~CubeWorldLayer() = default;

void CubeWorldLayer::onAttach() {
    ce::Renderer::setClearColor({0.45f, 0.69f, 1.0f, 1.0f}); // sky blue
    ce::Renderer::setFaceCulling(true);

    m_shader  = ce::Shader::fromFiles(asset("shaders/block.vert"),
                                      asset("shaders/block.frag"));
    m_texture = std::make_unique<ce::Texture2D>(asset("textures/atlas.png"));
    m_entityRenderer.load(CW_ASSET_DIR);
    m_hud.load(CW_ASSET_DIR);

    m_player.setCaptured(true); // grab the mouse for look controls
    updateProjection(1280, 720);

    CE_LOG_INFO("Controls: WASD move, Space/Shift up-down, mouse look, "
                "LMB break, RMB place, scroll select, Esc release cursor");

    // Singleplayer hosts an in-process server, then connects to it over loopback
    // exactly like a multiplayer client would connect to a remote host. ONE path.
    if (m_singleplayer) m_integrated.start(kDefaultPort);
    m_net.connect(m_serverHost, kDefaultPort, "player");
}

void CubeWorldLayer::onDetach() {
    m_net.disconnect();  // tell the server we're leaving...
    m_integrated.stop(); // ...then shut down the in-process server thread
    CE_LOG_INFO("CubeWorldLayer detached");
}

void CubeWorldLayer::onUpdate(float dt) {
    // Predict local movement, send the input, then reconcile against the server.
    const InputCommand cmd = m_player.update(dt, m_world);
    m_net.update(); // pump the network (snapshots, interpolation, pings)
    m_net.sendInput(cmd);

    glm::vec3 authPos;
    uint32_t  ackSeq;
    if (m_net.takeReconcile(authPos, ackSeq)) m_player.reconcile(authPos, ackSeq, m_world);

    updateMining(dt);

    // Day-night sky colour, tinted by the biome at the player's column.
    const glm::vec3 p   = m_player.position();
    const uint8_t   bid = m_world.getBiome(static_cast<int>(std::floor(p.x)),
                                           static_cast<int>(std::floor(p.z)));
    const Rgb       bSky = biomeDef(biomeFromId(bid)).sky;
    const float     b    = dayBrightness(m_net.worldTick(), m_net.dayLength());
    const glm::vec3 sky  = glm::mix(glm::vec3(0.02f, 0.02f, 0.08f),
                                    glm::vec3(bSky.r, bSky.g, bSky.b), b);
    ce::Renderer::setClearColor({sky.r, sky.g, sky.b, 1.0f});

    // Integrate chunks streamed from the server: copy data in and mark dirty
    // (plus already-loaded neighbours, whose seams now cull against the new chunk).
    for (auto& u : m_net.takeChunkUpdates()) {
        m_world.createChunk(u.coord) = *u.chunk;
        markDirty(u.coord);
        const ChunkCoord ns[] = {{u.coord.x - 1, u.coord.z}, {u.coord.x + 1, u.coord.z},
                                 {u.coord.x, u.coord.z - 1}, {u.coord.x, u.coord.z + 1}};
        for (const ChunkCoord& n : ns)
            if (m_world.getChunk(n)) markDirty(n);
    }

    // Apply authoritative block deltas from the server.
    for (const auto& bu : m_net.takeBlockUpdates()) {
        if (!m_world.getChunk(World::worldToChunk(bu.x, bu.z))) continue;
        m_world.setBlock(bu.x, bu.y, bu.z, bu.block);
        onBlockChanged(bu.x, bu.y, bu.z);
    }

    // Spread meshing cost across frames.
    processDirtyMeshes(kMeshesPerFrame);

    // Unload distant chunks when the player crosses a chunk boundary.
    const ChunkCoord current = playerChunk();
    if (!(current == m_lastChunk)) {
        unloadFar(current);
        m_lastChunk = current;
    }
}

ChunkCoord CubeWorldLayer::playerChunk() const {
    const glm::vec3 p = m_player.position();
    return World::worldToChunk(static_cast<int>(std::floor(p.x)),
                               static_cast<int>(std::floor(p.z)));
}

void CubeWorldLayer::markDirty(ChunkCoord c) { m_dirty.insert(c); }

void CubeWorldLayer::processDirtyMeshes(int maxPerFrame) {
    int built = 0;
    for (auto it = m_dirty.begin(); it != m_dirty.end() && built < maxPerFrame;) {
        const ChunkCoord c = *it;
        if (m_world.getChunk(c)) {
            m_meshes[c] = ChunkMesher::build(m_world, c);
            ++built;
        }
        it = m_dirty.erase(it);
    }
}

void CubeWorldLayer::unloadFar(ChunkCoord center) {
    const auto far = [&](ChunkCoord c, int max) {
        return std::max(std::abs(c.x - center.x), std::abs(c.z - center.z)) > max;
    };

    for (auto it = m_meshes.begin(); it != m_meshes.end();)
        it = far(it->first, kViewDistance) ? m_meshes.erase(it) : std::next(it);

    std::vector<ChunkCoord> stale;
    for (const auto& [coord, chunk] : m_world.chunks())
        if (far(coord, kViewDistance + 1)) stale.push_back(coord);
    for (ChunkCoord c : stale) m_world.removeChunk(c);
}

void CubeWorldLayer::onRender() {
    ce::Renderer::beginScene(m_player.camera());

    m_shader->bind();
    m_shader->setInt("u_texture", 0);
    m_texture->bind(0);

    for (const auto& [coord, mesh] : m_meshes) {
        const glm::mat4 transform = glm::translate(
            glm::mat4(1.0f),
            glm::vec3(coord.x * Chunk::SIZE_X, 0.0f, coord.z * Chunk::SIZE_Z));
        ce::Renderer::submit(*m_shader, *mesh, transform);
    }

    // Remote players/entities (placeholder cubes).
    m_entityRenderer.render(m_net.entities());

    ce::Renderer::endScene();

    // 2D HUD as a separate (orthographic) pass.
    m_hud.render(*m_texture, m_net.inventory(), m_selectedSlot, m_net.health(),
                 m_net.food(), m_screenW, m_screenH);
    if (m_crafting) m_hud.renderCrafting(*m_texture, m_screenW, m_screenH);

    // Chat log (+ input line when typing); player list while Tab is held.
    m_hud.renderChat(m_net.chatLog(), m_chatInput, m_chatOpen, m_screenW, m_screenH);
    if (m_player.captured() && ce::Input::isKeyDown(ce::Key::Tab))
        m_hud.renderPlayerList(m_net.playerNames(), m_screenW, m_screenH);
}

void CubeWorldLayer::onEvent(ce::Event& e) {
    ce::EventDispatcher dispatcher(e);

    dispatcher.dispatch<ce::WindowResizeEvent>([this](ce::WindowResizeEvent& r) {
        updateProjection(r.width(), r.height());
        return false;
    });

    // Chat text entry consumes typed characters while open.
    dispatcher.dispatch<ce::CharTypedEvent>([this](ce::CharTypedEvent& c) {
        if (!m_chatOpen) return false;
        if (m_swallowChar) { m_swallowChar = false; return true; }
        if (c.codepoint() >= 32 && c.codepoint() < 127 && m_chatInput.size() < 100)
            m_chatInput.push_back(static_cast<char>(c.codepoint()));
        return true;
    });

    dispatcher.dispatch<ce::KeyPressedEvent>([this](ce::KeyPressedEvent& k) {
        // ---- Chat input mode ----
        if (m_chatOpen) {
            if (k.keyCode() == ce::Key::Enter) {
                if (!m_chatInput.empty()) m_net.sendChat(m_chatInput);
                m_chatInput.clear();
                m_chatOpen = false;
                m_player.setCaptured(true);
            } else if (k.keyCode() == ce::Key::Escape) {
                m_chatInput.clear();
                m_chatOpen = false;
                m_player.setCaptured(true);
            } else if (k.keyCode() == ce::Key::Backspace && !m_chatInput.empty()) {
                m_chatInput.pop_back();
            }
            return true;
        }

        if (k.keyCode() == ce::Key::Escape) m_player.setCaptured(false);
        // Number keys 1..9 select a hotbar slot.
        if (k.keyCode() >= ce::Key::D1 && k.keyCode() <= ce::Key::D9)
            m_selectedSlot = k.keyCode() - ce::Key::D1;
        // F eats the food in the selected slot.
        if (k.keyCode() == ce::Key::F)
            m_net.sendEat(static_cast<uint8_t>(m_selectedSlot));
        // E toggles the crafting overlay (frees/recaptures the cursor).
        if (k.keyCode() == ce::Key::E) {
            m_crafting = !m_crafting;
            m_player.setCaptured(!m_crafting);
        }
        // T (or /) opens chat.
        if (k.keyCode() == ce::Key::T || k.keyCode() == ce::Key::Slash) {
            m_chatOpen     = true;
            m_swallowChar  = true;
            m_chatInput    = (k.keyCode() == ce::Key::Slash) ? "/" : "";
            m_player.setCaptured(false);
        }
        return false;
    });

    dispatcher.dispatch<ce::MouseButtonPressedEvent>([this](ce::MouseButtonPressedEvent& m) {
        if (m_chatOpen) return true;
        if (m_crafting) { // clicking a recipe crafts it
            if (m.button() == ce::Mouse::ButtonLeft) {
                const glm::vec2 mp = ce::Input::mousePosition();
                const int recipe = m_hud.craftHitTest(mp.x, mp.y, m_screenW, m_screenH);
                if (recipe >= 0) m_net.sendCraft(static_cast<uint16_t>(recipe));
            }
            return true;
        }
        if (!m_player.captured()) {           // first click re-grabs the cursor
            m_player.setCaptured(true);
            return true;
        }
        // Left click attacks a mob in the crosshair; otherwise breaking is
        // progressive (handled in updateMining while LMB is held).
        if (m.button() == ce::Mouse::ButtonLeft) {
            if (const EntityId target = pickTargetMob()) m_net.sendAttack(target);
        }
        if (m.button() == ce::Mouse::ButtonRight) placeBlock();
        return true;
    });

    dispatcher.dispatch<ce::MouseScrolledEvent>([this](ce::MouseScrolledEvent& s) {
        const int n = Inventory::kHotbar;
        m_selectedSlot = (m_selectedSlot + (s.yOffset() > 0 ? 1 : -1) + n) % n;
        return false;
    });
}

// Break/place only REQUEST the edit; the server validates and echoes the
// authoritative delta, which is applied in onUpdate. (Never trust the client.)
//
// Mining is progressive: holding LMB accumulates time on the targeted block,
// scaled by the held tool's speed vs the block's hardness; the break request is
// sent once enough time has elapsed.
EntityId CubeWorldLayer::pickTargetMob() const {
    const glm::vec3 origin = m_player.eyePosition();
    const glm::vec3 dir    = m_player.forward();
    EntityId best     = 0;
    float    bestT    = kReach;
    for (const auto& [id, e] : m_net.entities()) {
        if (e.type != static_cast<uint8_t>(EntityType::Mob)) continue;
        const glm::vec3 center = e.position + glm::vec3(0.0f, 0.6f, 0.0f);
        const float t = glm::dot(center - origin, dir);
        if (t <= 0.0f || t > kReach) continue;
        if (glm::length((origin + dir * t) - center) <= 0.7f && t < bestT) {
            bestT = t;
            best  = id;
        }
    }
    return best;
}

void CubeWorldLayer::updateMining(float dt) {
    if (!m_player.captured() || m_crafting ||
        !ce::Input::isMouseButtonDown(ce::Mouse::ButtonLeft)) {
        m_hasMineTarget = false;
        return;
    }
    // Don't mine through a mob that's in the crosshair (that's an attack target).
    if (pickTargetMob() != 0) { m_hasMineTarget = false; return; }
    const RaycastHit hit =
        raycastVoxel(m_world, m_player.eyePosition(), m_player.forward(), kReach);
    if (!hit.hit) { m_hasMineTarget = false; return; }

    if (!m_hasMineTarget || hit.block != m_mineTarget) {
        m_mineTarget    = hit.block;
        m_mineProgress  = 0.0f;
        m_hasMineTarget = true;
    }

    const BlockType b = m_world.getBlock(m_mineTarget.x, m_mineTarget.y, m_mineTarget.z);
    if (!isSolid(b)) { m_hasMineTarget = false; return; }

    const ItemType held = m_net.inventory().slot(m_selectedSlot).type;
    const float    time = blockHardness(b) / toolSpeed(held);

    m_mineProgress += dt;
    if (m_mineProgress >= time) {
        m_net.sendBlockEdit(BlockEditAction::Break, m_mineTarget.x, m_mineTarget.y,
                            m_mineTarget.z, static_cast<uint8_t>(m_selectedSlot));
        m_mineProgress  = 0.0f;
        m_hasMineTarget = false; // re-acquire next frame after the block is gone
    }
}

void CubeWorldLayer::placeBlock() {
    const RaycastHit hit =
        raycastVoxel(m_world, m_player.eyePosition(), m_player.forward(), kReach);
    if (!hit.hit) return;

    const glm::ivec3 p = hit.block + hit.normal; // empty cell against the hit face
    if (p.y < 0 || p.y >= Chunk::SIZE_Y) return;
    if (isOpaque(m_world.getBlock(p.x, p.y, p.z))) return; // local pre-check only
    // Server reads the item from this hotbar slot; the block byte is the slot index.
    m_net.sendBlockEdit(BlockEditAction::Place, p.x, p.y, p.z,
                        static_cast<uint8_t>(m_selectedSlot));
}

void CubeWorldLayer::onBlockChanged(int wx, int wy, int wz) {
    const ChunkCoord c = World::worldToChunk(wx, wz);
    markDirty(c);

    // If the edit touched a chunk border, the neighbour's seam faces change too.
    const int lx = floorMod(wx, Chunk::SIZE_X);
    const int lz = floorMod(wz, Chunk::SIZE_Z);
    if (lx == 0)                 markDirty({c.x - 1, c.z});
    if (lx == Chunk::SIZE_X - 1) markDirty({c.x + 1, c.z});
    if (lz == 0)                 markDirty({c.x, c.z - 1});
    if (lz == Chunk::SIZE_Z - 1) markDirty({c.x, c.z + 1});
}

void CubeWorldLayer::updateProjection(uint32_t width, uint32_t height) {
    m_screenW = width;
    m_screenH = height;
    const float aspect = height == 0 ? 1.0f : static_cast<float>(width) / height;
    m_player.setProjection(45.0f, aspect, 0.1f, 400.0f);
}

} // namespace cw
