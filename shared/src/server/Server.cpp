#include <cw/server/Server.h>

#include <cw/craft/Crafting.h>
#include <cw/proto/Protocol.h>
#include <cw/sim/DayCycle.h>
#include <cw/sim/PlayerMovement.h>

#include <glm/glm.hpp>

#include <chrono>
#include <cmath>
#include <iostream>
#include <iterator>
#include <sstream>
#include <thread>
#include <vector>

namespace cw {

namespace {
// Build "<id><payload>" and send reliably on the reliable channel.
template <typename T>
void sendTo(net::Host& host, uint32_t peer, PacketId id, const T& msg) {
    net::ByteBuffer b;
    writePacketId(b, id);
    msg.serialize(b);
    host.send(peer, net::kChannelReliable, b, true);
}

template <typename T>
void broadcastPkt(net::Host& host, PacketId id, const T& msg) {
    net::ByteBuffer b;
    writePacketId(b, id);
    msg.serialize(b);
    host.broadcast(net::kChannelReliable, b, true);
}

const glm::vec3 kSpawnPos{0.0f, 50.0f, 0.0f};
constexpr float kFallSafe         = 3.0f; // blocks of free fall before damage
constexpr float kFallDamage       = 1.0f; // health per block past kFallSafe
constexpr int   kHungerEveryTicks = 200;  // food -1 each (10 s)
constexpr int   kStarveEveryTicks = 40;   // health -1 while starving
constexpr int   kRegenEveryTicks  = 60;   // health +1 while well-fed

S2CSpawnEntity makeSpawn(const Entity& e) {
    S2CSpawnEntity s;
    s.id        = e.id;
    s.type      = static_cast<uint8_t>(e.type);
    s.position  = e.position;
    s.yaw       = e.yaw;
    s.name      = e.name;
    s.item      = static_cast<uint16_t>(e.item.type);
    s.itemCount = e.item.count;
    s.hostile   = e.hostile ? 1 : 0;
    return s;
}
} // namespace

bool Server::start(uint16_t port) {
    if (!m_host.createServer(port, 32 /*maxClients*/)) {
        std::cerr << "[server] failed to bind port " << port << "\n";
        return false;
    }
    std::cout << "[server] listening on port " << port
              << " (protocol v" << kProtocolVersion
              << ", " << Simulation::kTickRate << " Hz)\n";
    return true;
}

void Server::run() {
    using clock = std::chrono::steady_clock;
    const auto tickDuration =
        std::chrono::duration_cast<clock::duration>(
            std::chrono::duration<double>(Simulation::kTickSeconds));

    m_running          = true;
    auto     nextTick  = clock::now();
    uint32_t lastLogTick = 0;

    while (m_running) {
        pollNetwork();
        m_sim.step();
        updateSurvival();
        spawnHostiles();
        pickupItems();
        streamChunks();
        sendSnapshots();

        // Once a second: heartbeat + broadcast world time to all clients.
        if (m_sim.tick() - lastLogTick >= static_cast<uint32_t>(Simulation::kTickRate)) {
            lastLogTick = m_sim.tick();
            std::cout << "[server] tick " << m_sim.tick()
                      << ", players online: " << m_playerEntity.size() << "\n";
            S2CTime tp{m_sim.tick(), kDayLengthTicks};
            for (const auto& [peer, eid] : m_playerEntity)
                sendTo(m_host, peer, PacketId::S2C_Time, tp);
        }

        // Autosave every 30 s.
        if (m_sim.tick() % (Simulation::kTickRate * 30) == 0) {
            saveAll();
            std::cout << "[server] autosaved\n";
        }

        nextTick += tickDuration;
        std::this_thread::sleep_until(nextTick);
    }

    saveAll(); // final save on shutdown
    std::cout << "[server] saved on shutdown\n";
}

void Server::stop() { m_running = false; }

void Server::pollNetwork() {
    net::Event e;
    while (m_host.poll(e, 0)) { // drain all pending events without blocking
        switch (e.type) {
            case net::EventType::Connect:
                std::cout << "[server] peer " << e.peerId << " connected\n";
                break;
            case net::EventType::Receive:
                handlePacket(e.peerId, e.data);
                break;
            case net::EventType::Disconnect:
                onDisconnect(e.peerId);
                break;
            default:
                break;
        }
    }
}

void Server::handlePacket(uint32_t peer, net::ByteBuffer& data) {
    const PacketId id = readPacketId(data);
    switch (id) {
        case PacketId::C2S_Hello: {
            C2SHello hello;
            hello.deserialize(data);
            if (!data.ok()) return; // malformed: ignore

            if (hello.protocolVersion != kProtocolVersion) {
                S2CDisconnect d{"protocol version mismatch"};
                sendTo(m_host, peer, PacketId::S2C_Disconnect, d);
                m_host.disconnect(peer);
                std::cout << "[server] peer " << peer
                          << " rejected (protocol v" << hello.protocolVersion << ")\n";
                return;
            }

            const std::string name = hello.name.empty() ? "anon" : hello.name;

            // Spawn this connection's authoritative player entity on the ground.
            const glm::vec3 spawn = findSpawnPosition(static_cast<int>(kSpawnPos.x),
                                                      static_cast<int>(kSpawnPos.z));
            const EntityId  eid   = m_sim.spawnPlayer(name, spawn);
            m_playerEntity[peer]  = eid;
            m_playerName[peer]   = name;

            // Restore saved data if this player has played before.
            PlayerData saved;
            const bool returning = m_playerStore.load(name, saved);
            if (returning) {
                Entity* e   = m_sim.getEntity(eid);
                e->position = saved.position;
                e->yaw      = saved.yaw;
                e->pitch    = saved.pitch;
                e->health   = saved.health;
                e->food     = saved.food;
                *m_sim.inventory(eid) = saved.inventory;
            }

            // Accept the join; the player's own entity id doubles as its player id.
            S2CWelcome w;
            w.playerId = eid;
            w.tickRate = Simulation::kTickRate;
            sendTo(m_host, peer, PacketId::S2C_Welcome, w);

            // Tell the newcomer about every already-present entity...
            for (const auto& [id, e] : m_sim.entities())
                if (id != eid) sendTo(m_host, peer, PacketId::S2C_SpawnEntity, makeSpawn(e));

            // ...and tell everyone (including the newcomer) about the new entity.
            broadcastPkt(m_host, PacketId::S2C_SpawnEntity,
                         makeSpawn(*m_sim.getEntity(eid)));

            // New players get a few starter apples; returning players keep theirs.
            if (Inventory* inv = m_sim.inventory(eid)) {
                if (!returning) inv->add(ItemStack{ItemType::Apple, 5});
                S2CInventory ip;
                ip.inventory = *inv;
                sendTo(m_host, peer, PacketId::S2C_Inventory, ip);
            }
            sendStats(peer, eid);

            // Spawn a few ambient passive mobs once, near the world spawn.
            if (!m_mobsSpawned) {
                m_mobsSpawned = true;
                for (int i = 0; i < 4; ++i) {
                    const glm::vec3 mpos{kSpawnPos.x + (i - 2) * 3.0f, kSpawnPos.y,
                                         kSpawnPos.z + 4.0f};
                    const EntityId mid = m_sim.spawnMob(mpos, false /*passive*/);
                    broadcastPkt(m_host, PacketId::S2C_SpawnEntity,
                                 makeSpawn(*m_sim.getEntity(mid)));
                }
            }

            std::cout << "[server] '" << name << "' joined as entity " << eid << "\n";
            broadcastChat("", name + " joined the game");
            broadcastPlayerList();
            break;
        }
        case PacketId::C2S_Ping: {
            C2SPing ping;
            ping.deserialize(data);
            if (!data.ok()) return;
            S2CPong pong{ping.clientTimeMs, m_sim.tick()};
            sendTo(m_host, peer, PacketId::S2C_Pong, pong);
            break;
        }
        case PacketId::C2S_Input: {
            InputCommand cmd;
            cmd.deserialize(data);
            if (!data.ok()) return;

            auto it = m_playerEntity.find(peer);
            if (it == m_playerEntity.end()) return; // not joined yet
            Entity* e = m_sim.getEntity(it->second);
            if (!e) return;

            // Authoritative movement with gravity + collision against the world.
            applyInput(*e, cmd, m_sim.world());
            m_lastInputSeq[peer] = cmd.sequence;

            // Fall damage on landing.
            if (e->pendingFall > kFallSafe) {
                e->health -= (e->pendingFall - kFallSafe) * kFallDamage;
                if (e->health <= 0.0f) respawn(*e);
                sendStats(peer, it->second);
            }
            e->pendingFall = 0.0f;
            break;
        }
        case PacketId::C2S_Eat: {
            C2SEat eat;
            eat.deserialize(data);
            if (!data.ok()) return;
            auto it = m_playerEntity.find(peer);
            if (it == m_playerEntity.end()) return;
            const EntityId eid = it->second;
            Entity*    e   = m_sim.getEntity(eid);
            Inventory* inv = m_sim.inventory(eid);
            if (!e || !inv || eat.slot >= Inventory::kHotbar) return;

            const ItemStack st = inv->slot(eat.slot);
            if (!isFood(st.type) || e->food >= 20.0f) return;
            e->food = std::min(20.0f, e->food + static_cast<float>(foodValue(st.type)));
            inv->removeOne(eat.slot);

            S2CInventory ip;
            ip.inventory = *inv;
            sendTo(m_host, peer, PacketId::S2C_Inventory, ip);
            sendStats(peer, eid);
            break;
        }
        case PacketId::C2S_Craft: {
            C2SCraft craft;
            craft.deserialize(data);
            if (!data.ok()) return;
            auto it = m_playerEntity.find(peer);
            if (it == m_playerEntity.end()) return;
            Inventory* inv = m_sim.inventory(it->second);
            if (inv && tryCraft(*inv, craft.recipe)) {
                S2CInventory ip;
                ip.inventory = *inv;
                sendTo(m_host, peer, PacketId::S2C_Inventory, ip);
            }
            break;
        }
        case PacketId::C2S_Chat: {
            C2SChat chat;
            chat.deserialize(data);
            if (!data.ok() || chat.text.empty()) return;
            if (chat.text[0] == '/') {
                handleCommand(peer, chat.text);
            } else {
                auto nit = m_playerName.find(peer);
                broadcastChat(nit != m_playerName.end() ? nit->second : "?", chat.text);
            }
            break;
        }
        case PacketId::C2S_Attack: {
            C2SAttack atk;
            atk.deserialize(data);
            if (!data.ok()) return;
            auto it = m_playerEntity.find(peer);
            if (it == m_playerEntity.end()) return;
            const Entity* player = m_sim.getEntity(it->second);
            Entity*       mob    = m_sim.getEntity(atk.target);
            if (!player || !mob || mob->type != EntityType::Mob) return;

            // Validate reach, then apply melee damage.
            if (glm::length(mob->position - player->position) > kReachDistance + 1.5f) return;
            mob->health -= 4.0f;
            if (mob->health <= 0.0f) {
                const EntityId dead = mob->id;
                m_sim.removeEntity(dead);
                S2CDespawnEntity d{dead};
                broadcastPkt(m_host, PacketId::S2C_DespawnEntity, d);
            }
            break;
        }
        case PacketId::C2S_BlockEdit: {
            C2SBlockEdit edit;
            edit.deserialize(data);
            if (!data.ok()) return;

            auto it = m_playerEntity.find(peer);
            if (it == m_playerEntity.end()) return;
            const EntityId eid = it->second;
            const Entity*  e   = m_sim.getEntity(eid);
            if (!e) return;
            if (edit.y < 0 || edit.y >= Chunk::SIZE_Y) return;

            // Validate reach: never trust the client's claimed target.
            const glm::vec3 center(edit.x + 0.5f, edit.y + 0.5f, edit.z + 0.5f);
            if (glm::length(center - e->position) > kReachDistance + 2.0f) return;

            const ChunkCoord cc = World::worldToChunk(edit.x, edit.z);
            if (!m_sim.world().getChunk(cc)) return; // not a loaded area

            const BlockType cur = m_sim.world().getBlock(edit.x, edit.y, edit.z);

            if (edit.action == BlockEditAction::Break) {
                if (!isSolid(cur)) return;
                m_sim.world().setBlock(edit.x, edit.y, edit.z, BlockType::Air);
                m_regions.saveChunk(cc, *m_sim.world().getChunk(cc));
                broadcastBlockUpdate(cc, edit.x, edit.y, edit.z, BlockType::Air);

                // Drop an item entity the player can pick up.
                const ItemType drop = itemForBlock(cur);
                if (drop != ItemType::None) {
                    const EntityId iid = m_sim.spawnItem(center, ItemStack{drop, 1});
                    broadcastPkt(m_host, PacketId::S2C_SpawnEntity,
                                 makeSpawn(*m_sim.getEntity(iid)));
                }

                // Wear down the tool used to break (sent in `slot`).
                if (Inventory* inv = m_sim.inventory(eid);
                    inv && edit.slot < Inventory::kHotbar) {
                    ItemStack& s = inv->slot(edit.slot);
                    if (isTool(s.type) && s.durability > 0) {
                        if (--s.durability == 0) inv->removeOne(edit.slot);
                        S2CInventory ip;
                        ip.inventory = *inv;
                        sendTo(m_host, peer, PacketId::S2C_Inventory, ip);
                    }
                }
            } else { // Place from a hotbar slot
                Inventory* inv = m_sim.inventory(eid);
                if (!inv || edit.slot >= Inventory::kHotbar) return;
                const ItemStack st = inv->slot(edit.slot);
                if (st.empty() || !isBlockItem(st.type)) return;
                if (isOpaque(cur)) return; // only into air/water

                const BlockType nb = blockForItem(st.type);
                inv->removeOne(edit.slot);
                m_sim.world().setBlock(edit.x, edit.y, edit.z, nb);
                m_regions.saveChunk(cc, *m_sim.world().getChunk(cc));
                broadcastBlockUpdate(cc, edit.x, edit.y, edit.z, nb);

                S2CInventory ip;
                ip.inventory = *inv;
                sendTo(m_host, peer, PacketId::S2C_Inventory, ip);
            }
            break;
        }
        default:
            // Unknown/unexpected packet from a client — ignore (never trust input).
            break;
    }
}

void Server::onDisconnect(uint32_t peer) {
    auto it = m_playerEntity.find(peer);
    if (it == m_playerEntity.end()) {
        std::cout << "[server] peer " << peer << " disconnected (pre-join)\n";
        return;
    }
    savePlayer(peer); // persist before removing

    auto nit = m_playerName.find(peer);
    const std::string name = nit != m_playerName.end() ? nit->second : std::string{};

    const EntityId eid = it->second;
    m_playerEntity.erase(it);
    m_lastInputSeq.erase(peer);
    m_sentChunks.erase(peer);
    m_playerName.erase(peer);
    m_sim.removeEntity(eid);

    S2CDespawnEntity d{eid};
    broadcastPkt(m_host, PacketId::S2C_DespawnEntity, d);
    std::cout << "[server] entity " << eid << " disconnected\n";

    if (!name.empty()) broadcastChat("", name + " left the game");
    broadcastPlayerList();
}

void Server::streamChunks() {
    constexpr int kChunksPerPlayerPerTick = 8; // throttle bandwidth/CPU

    for (const auto& [peer, eid] : m_playerEntity) {
        const Entity* e = m_sim.getEntity(eid);
        if (!e) continue;

        const ChunkCoord center = World::worldToChunk(
            static_cast<int>(std::floor(e->position.x)),
            static_cast<int>(std::floor(e->position.z)));

        auto& sent   = m_sentChunks[peer];
        int   budget = kChunksPerPlayerPerTick;

        // Nearest-first: walk outward in square rings up to the view distance.
        for (int r = 0; r <= kViewDistance && budget > 0; ++r) {
            for (int dz = -r; dz <= r && budget > 0; ++dz) {
                for (int dx = -r; dx <= r && budget > 0; ++dx) {
                    if (std::max(std::abs(dx), std::abs(dz)) != r) continue; // ring only
                    const ChunkCoord c{center.x + dx, center.z + dz};
                    if (sent.count(c)) continue;

                    // Load (edited from disk, or generated from seed) then send.
                    ensureChunk(c);

                    net::ByteBuffer b;
                    writePacketId(b, PacketId::S2C_ChunkData);
                    b.writeI32(c.x);
                    b.writeI32(c.z);
                    m_sim.world().getChunk(c)->serialize(b);
                    m_host.send(peer, net::kChannelReliable, b, true);

                    sent.insert(c);
                    --budget;
                }
            }
        }

        // Forget chunks the player has left (with hysteresis) so they re-stream
        // if the player returns — the client unloads them on its side.
        for (auto sit = sent.begin(); sit != sent.end();) {
            if (std::max(std::abs(sit->x - center.x), std::abs(sit->z - center.z))
                > kViewDistance + 2)
                sit = sent.erase(sit);
            else
                ++sit;
        }
    }
}

void Server::sendStats(uint32_t peer, EntityId eid) {
    const Entity* e = m_sim.getEntity(eid);
    if (!e) return;
    S2CStats s{e->health, e->food};
    sendTo(m_host, peer, PacketId::S2C_Stats, s);
}

void Server::respawn(Entity& e) {
    e.position    = findSpawnPosition(static_cast<int>(kSpawnPos.x),
                                      static_cast<int>(kSpawnPos.z));
    e.velocity    = glm::vec3(0.0f);
    e.health      = 20.0f;
    e.food        = 20.0f;
    e.onGround    = false;
    e.pendingFall = 0.0f;
    std::cout << "[server] entity " << e.id << " respawned\n";
}

void Server::updateSurvival() {
    const uint32_t t = m_sim.tick();
    for (const auto& [peer, eid] : m_playerEntity) {
        Entity* e = m_sim.getEntity(eid);
        if (!e) continue;
        bool changed = false;

        if (t % kHungerEveryTicks == 0 && e->food > 0.0f) { e->food -= 1.0f; changed = true; }
        if (e->food <= 0.0f && t % kStarveEveryTicks == 0) { e->health -= 1.0f; changed = true; }
        if (e->food >= 18.0f && e->health < 20.0f && t % kRegenEveryTicks == 0) {
            e->health += 1.0f; changed = true;
        }
        if (e->health <= 0.0f) { respawn(*e); changed = true; }

        // Periodic resync so clients stay accurate even if a delta was missed.
        if (changed || t % 20 == 0) sendStats(peer, eid);
    }
}

void Server::spawnHostiles() {
    // Every few seconds, top up hostile mobs near a random player (D3 will gate
    // this on darkness; for now they spawn regardless of time).
    constexpr int kSpawnEveryTicks = 100; // 5 s
    constexpr int kMaxHostiles     = 6;
    if (m_sim.tick() % kSpawnEveryTicks != 0 || m_playerEntity.empty()) return;
    if (!isNight(m_sim.tick(), kDayLengthTicks)) return; // hostiles spawn in darkness

    int hostiles = 0;
    for (const auto& [id, e] : m_sim.entities())
        if (e.type == EntityType::Mob && e.hostile) ++hostiles;
    if (hostiles >= kMaxHostiles) return;

    // Pick a player and offset horizontally; the mob falls to the ground.
    auto it = m_playerEntity.begin();
    std::advance(it, m_sim.tick() / kSpawnEveryTicks % m_playerEntity.size());
    const Entity* p = m_sim.getEntity(it->second);
    if (!p) return;

    const glm::vec3 pos{p->position.x + 12.0f, p->position.y + 6.0f, p->position.z};
    const EntityId  mid = m_sim.spawnMob(pos, true /*hostile*/);
    broadcastPkt(m_host, PacketId::S2C_SpawnEntity, makeSpawn(*m_sim.getEntity(mid)));
}

Chunk* Server::ensureChunk(ChunkCoord c) {
    if (Chunk* existing = m_sim.world().getChunk(c)) return existing;
    Chunk& chunk = m_sim.world().createChunk(c);
    if (!m_regions.loadChunk(c, chunk)) m_worldGen.generate(chunk, c); // disk or seed
    return &chunk;
}

glm::vec3 Server::findSpawnPosition(int x, int z) {
    // Make sure the spawn column exists, then drop the player onto the highest
    // solid (opaque) block so they don't fall and die from spawn-height damage.
    ensureChunk(World::worldToChunk(x, z));
    for (int y = Chunk::SIZE_Y - 2; y >= 0; --y) {
        if (isOpaque(m_sim.world().getBlock(x, y, z)))
            return glm::vec3(x + 0.5f, static_cast<float>(y + 1), z + 0.5f);
    }
    return glm::vec3(x + 0.5f, 64.0f, z + 0.5f); // fallback: empty column
}

void Server::savePlayer(uint32_t peer) {
    auto nit = m_playerName.find(peer);
    auto eit = m_playerEntity.find(peer);
    if (nit == m_playerName.end() || eit == m_playerEntity.end()) return;
    const Entity*    e   = m_sim.getEntity(eit->second);
    const Inventory* inv = m_sim.inventory(eit->second);
    if (!e || !inv) return;

    PlayerData d;
    d.position  = e->position;
    d.yaw       = e->yaw;
    d.pitch     = e->pitch;
    d.health    = e->health;
    d.food      = e->food;
    d.inventory = *inv;
    m_playerStore.save(nit->second, d);
}

void Server::saveAll() {
    for (const auto& [peer, eid] : m_playerEntity) savePlayer(peer);
    m_regions.flush();
}

void Server::broadcastChat(const std::string& sender, const std::string& text) {
    S2CChat c{sender, text};
    broadcastPkt(m_host, PacketId::S2C_Chat, c);
    std::cout << "[chat] " << (sender.empty() ? "*" : sender) << ": " << text << "\n";
}

void Server::sendChat(uint32_t peer, const std::string& text) {
    S2CChat c{"", text};
    sendTo(m_host, peer, PacketId::S2C_Chat, c);
}

void Server::broadcastPlayerList() {
    S2CPlayerList pl;
    for (const auto& [peer, name] : m_playerName) pl.names.push_back(name);
    broadcastPkt(m_host, PacketId::S2C_PlayerList, pl);
}

namespace {
ItemType itemFromName(const std::string& n) {
    if (n == "grass") return ItemType::Grass;
    if (n == "dirt") return ItemType::Dirt;
    if (n == "stone") return ItemType::Stone;
    if (n == "sand") return ItemType::Sand;
    if (n == "apple") return ItemType::Apple;
    if (n == "pickaxe") return ItemType::StonePickaxe;
    return ItemType::None;
}
} // namespace

void Server::handleCommand(uint32_t peer, const std::string& line) {
    std::istringstream ss(line);
    std::string cmd;
    ss >> cmd;

    auto eit = m_playerEntity.find(peer);
    if (eit == m_playerEntity.end()) return;
    const EntityId eid = eit->second;

    if (cmd == "/help") {
        sendChat(peer, "commands: /list /time day|night /give <item> <n> /tp <x> <y> <z> /save /kill");
    } else if (cmd == "/list") {
        std::string names;
        for (const auto& [p, n] : m_playerName) names += (names.empty() ? "" : ", ") + n;
        sendChat(peer, "players: " + names);
    } else if (cmd == "/time") {
        std::string when;
        ss >> when;
        if (when == "day")        { m_sim.setTick(kDayLengthTicks / 2); sendChat(peer, "time set to day"); }
        else if (when == "night") { m_sim.setTick(0); sendChat(peer, "time set to night"); }
        else                      sendChat(peer, "usage: /time day|night");
    } else if (cmd == "/give") {
        std::string item; int n = 1;
        ss >> item >> n;
        const ItemType type = itemFromName(item);
        if (type == ItemType::None) { sendChat(peer, "unknown item"); return; }
        if (n < 1) n = 1;
        if (Inventory* inv = m_sim.inventory(eid)) {
            ItemStack st{type, static_cast<uint8_t>(n < 64 ? n : 64)};
            if (isTool(type)) st.durability = static_cast<uint16_t>(toolDurability(type));
            inv->add(st);
            S2CInventory ip; ip.inventory = *inv;
            sendTo(m_host, peer, PacketId::S2C_Inventory, ip);
            sendChat(peer, "gave " + std::to_string(n) + " " + item);
        }
    } else if (cmd == "/tp") {
        float x, y, z;
        if (ss >> x >> y >> z) {
            if (Entity* e = m_sim.getEntity(eid)) {
                e->position = {x, y, z};
                e->velocity = glm::vec3(0.0f);
                sendChat(peer, "teleported");
            }
        } else {
            sendChat(peer, "usage: /tp <x> <y> <z>");
        }
    } else if (cmd == "/save") {
        saveAll();
        sendChat(peer, "saved");
    } else if (cmd == "/kill") {
        if (Entity* e = m_sim.getEntity(eid)) { e->health = 0.0f; sendChat(peer, "ouch"); }
    } else {
        sendChat(peer, "unknown command (try /help)");
    }
}

void Server::broadcastBlockUpdate(ChunkCoord cc, int x, int y, int z, BlockType block) {
    S2CBlockUpdate up{x, y, z, static_cast<uint8_t>(block)};
    for (const auto& [p, eid] : m_playerEntity) {
        auto sit = m_sentChunks.find(p);
        if (sit != m_sentChunks.end() && sit->second.count(cc))
            sendTo(m_host, p, PacketId::S2C_BlockUpdate, up);
    }
}

void Server::pickupItems() {
    constexpr float kPickupRange = 1.6f;

    for (const auto& [peer, eid] : m_playerEntity) {
        const Entity* player = m_sim.getEntity(eid);
        Inventory*    inv    = m_sim.inventory(eid);
        if (!player || !inv) continue;

        // Collect nearby item-entity ids first (don't mutate the map while iterating).
        std::vector<EntityId> nearby;
        for (const auto& [id, e] : m_sim.entities())
            if (e.type == EntityType::Item &&
                glm::length(e.position - player->position) <= kPickupRange)
                nearby.push_back(id);

        bool changed = false;
        for (EntityId id : nearby) {
            Entity* item = m_sim.getEntity(id);
            if (!item) continue;
            const uint8_t before    = item->item.count;
            const uint8_t leftover  = inv->add(item->item);
            if (leftover == before) continue; // inventory full for this item
            changed = true;
            if (leftover == 0) {
                m_sim.removeEntity(id);
                S2CDespawnEntity d{id};
                broadcastPkt(m_host, PacketId::S2C_DespawnEntity, d);
            } else {
                item->item.count = leftover;
            }
        }

        if (changed) {
            S2CInventory ip;
            ip.inventory = *inv;
            sendTo(m_host, peer, PacketId::S2C_Inventory, ip);
        }
    }
}

void Server::sendSnapshots() {
    if (m_playerEntity.empty()) return;

    // Build the shared entity list once (all entities); only lastInputSeq differs
    // per client. Sent UNRELIABLE-SEQUENCED — stale snapshots are simply dropped.
    S2CSnapshot snap;
    snap.serverTick = m_sim.tick();
    snap.entities.reserve(m_sim.entities().size());
    for (const auto& [id, e] : m_sim.entities())
        snap.entities.push_back({id, e.position, e.yaw});

    for (const auto& [peer, eid] : m_playerEntity) {
        snap.lastInputSeq = m_lastInputSeq.count(peer) ? m_lastInputSeq[peer] : 0;
        net::ByteBuffer b;
        writePacketId(b, PacketId::S2C_Snapshot);
        snap.serialize(b);
        m_host.send(peer, net::kChannelUnreliable, b, false);
    }
}

} // namespace cw
