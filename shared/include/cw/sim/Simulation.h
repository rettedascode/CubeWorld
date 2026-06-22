#pragma once

#include <cw/entity/Entity.h>
#include <cw/item/Inventory.h>
#include <cw/world/World.h>

#include <cstdint>
#include <random>
#include <unordered_map>

namespace cw {

// The authoritative, headless game simulation: owns the world and all entities,
// advanced exactly once per fixed server tick. SHARED — never references rendering.
class Simulation {
public:
    static constexpr int    kTickRate    = 20;            // ticks per second
    static constexpr double kTickSeconds = 1.0 / kTickRate;

    void     step();                       // advance one fixed tick
    uint32_t tick() const { return m_tick; }
    void     setTick(uint32_t t) { m_tick = t; } // used by the /time command

    EntityId spawnPlayer(const std::string& name, const glm::vec3& pos);
    EntityId spawnItem(const glm::vec3& pos, ItemStack item);
    EntityId spawnMob(const glm::vec3& pos, bool hostile);
    void     removeEntity(EntityId id);
    Entity*  getEntity(EntityId id);

    const std::unordered_map<EntityId, Entity>& entities() const { return m_entities; }

    Inventory*       inventory(EntityId player);
    const Inventory* inventory(EntityId player) const;

    World&       world() { return m_world; }
    const World& world() const { return m_world; }

private:
    void updateMobs(float dt);

    World                                   m_world;
    std::unordered_map<EntityId, Entity>    m_entities;
    std::unordered_map<EntityId, Inventory> m_inventories; // per player entity
    EntityId                                m_nextEntityId = 1;
    uint32_t                                m_tick         = 0;
    std::mt19937                            m_rng{12345};
};

} // namespace cw
