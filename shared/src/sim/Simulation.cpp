#include <cw/sim/Simulation.h>

#include <cw/sim/PlayerMovement.h>

#include <glm/glm.hpp>

#include <cmath>
#include <vector>

namespace cw {

void Simulation::step() {
    ++m_tick;
    updateMobs(static_cast<float>(kTickSeconds));
}

void Simulation::updateMobs(float dt) {
    constexpr float kDetectRange = 16.0f;
    constexpr float kMeleeRange  = 1.6f;
    constexpr float kMobDamage   = 3.0f;

    std::uniform_real_distribution<float> chance(0.0f, 1.0f);
    std::uniform_real_distribution<float> angle(-3.14159f, 3.14159f);

    // Snapshot player pointers so hostile mobs can target the nearest one.
    std::vector<Entity*> players;
    for (auto& [id, e] : m_entities)
        if (e.type == EntityType::Player) players.push_back(&e);

    for (auto& [id, e] : m_entities) {
        if (e.type != EntityType::Mob) continue;
        if (e.attackCooldown > 0.0f) e.attackCooldown -= dt;

        Entity* target = nullptr;
        if (e.hostile) {
            float best = kDetectRange * kDetectRange;
            for (Entity* p : players) {
                const glm::vec3 d = p->position - e.position;
                const float     d2 = glm::dot(d, d);
                if (d2 < best) { best = d2; target = p; }
            }
        }

        if (target) {
            // Greedy steering: face the player and walk toward them until in range.
            const glm::vec3 to = target->position - e.position;
            const float     dist = std::sqrt(to.x * to.x + to.z * to.z);
            e.yaw = std::atan2(to.z, to.x);
            if (dist > kMeleeRange) {
                e.velocity.x = std::cos(e.yaw) * kMobSpeed;
                e.velocity.z = std::sin(e.yaw) * kMobSpeed;
            } else {
                e.velocity.x = e.velocity.z = 0.0f;
                if (e.attackCooldown <= 0.0f) {
                    target->health -= kMobDamage; // server syncs stats + handles death
                    e.attackCooldown = 1.0f;
                }
            }
        } else {
            // Passive (or no target): periodic wander.
            e.aiTimer -= dt;
            if (e.aiTimer <= 0.0f) {
                e.aiTimer  = 2.0f + chance(m_rng) * 3.0f;
                e.aiMoving = chance(m_rng) < 0.6f;
                if (e.aiMoving) e.yaw = angle(m_rng);
            }
            if (e.aiMoving) {
                e.velocity.x = std::cos(e.yaw) * kMobSpeed;
                e.velocity.z = std::sin(e.yaw) * kMobSpeed;
            } else {
                e.velocity.x = e.velocity.z = 0.0f;
            }
        }

        moveWithCollision(e, m_world, dt);
    }
}

EntityId Simulation::spawnPlayer(const std::string& name, const glm::vec3& pos) {
    const EntityId id = m_nextEntityId++;
    Entity e;
    e.id       = id;
    e.type     = EntityType::Player;
    e.position = pos;
    e.name     = name;
    m_entities[id]    = e;
    m_inventories[id] = Inventory{};
    return id;
}

EntityId Simulation::spawnItem(const glm::vec3& pos, ItemStack item) {
    const EntityId id = m_nextEntityId++;
    Entity e;
    e.id       = id;
    e.type     = EntityType::Item;
    e.position = pos;
    e.item     = item;
    m_entities[id] = e;
    return id;
}

EntityId Simulation::spawnMob(const glm::vec3& pos, bool hostile) {
    const EntityId id = m_nextEntityId++;
    Entity e;
    e.id       = id;
    e.type     = EntityType::Mob;
    e.position = pos;
    e.health   = 10.0f;
    e.hostile  = hostile;
    e.name     = hostile ? "zombie" : "sheep";
    m_entities[id] = e;
    return id;
}

void Simulation::removeEntity(EntityId id) {
    m_entities.erase(id);
    m_inventories.erase(id);
}

Entity* Simulation::getEntity(EntityId id) {
    auto it = m_entities.find(id);
    return it == m_entities.end() ? nullptr : &it->second;
}

Inventory* Simulation::inventory(EntityId player) {
    auto it = m_inventories.find(player);
    return it == m_inventories.end() ? nullptr : &it->second;
}

const Inventory* Simulation::inventory(EntityId player) const {
    auto it = m_inventories.find(player);
    return it == m_inventories.end() ? nullptr : &it->second;
}

} // namespace cw
