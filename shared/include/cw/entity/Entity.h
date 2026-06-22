#pragma once

#include <cw/item/Item.h>

#include <glm/glm.hpp>

#include <cstdint>
#include <string>

namespace cw {

using EntityId = uint32_t;

enum class EntityType : uint8_t {
    Player = 0,
    Mob,
    Item,
};

// A simulated entity. SHARED headless state — no rendering. The client mirrors a
// lightweight copy of these for remote entities; the server owns the truth.
struct Entity {
    EntityId    id   = 0;
    EntityType  type = EntityType::Player;

    glm::vec3 position{0.0f}; // feet position (bottom-centre of the AABB)
    glm::vec3 velocity{0.0f};
    float     yaw   = 0.0f; // radians
    float     pitch = 0.0f;

    std::string name; // players

    // Survival stats (players/mobs). 20 = full.
    float health = 20.0f;
    float food   = 20.0f;

    // Physics state used by movement + fall damage.
    bool  onGround    = false;
    float fallPeakY   = 0.0f; // highest Y reached since leaving the ground
    float pendingFall = 0.0f; // fall distance on the last landing (server reads + clears)

    // Jump-feel state (players). Transient and NOT replicated — both client
    // prediction and server authority evolve it through applyInput identically.
    float coyoteTimer     = 0.0f; // grace window to still jump after leaving ground
    float jumpBufferTimer = 0.0f; // remembers a jump press made just before landing
    bool  jumpHeld        = false;// jump key state last tick (for variable height)

    // Item entities (type == Item): what's lying on the ground.
    ItemStack item;

    // Mob AI scratch (type == Mob).
    float aiTimer        = 0.0f;  // seconds until the next decision
    bool  aiMoving       = false; // currently walking toward `yaw`
    bool  hostile        = false; // D2
    float attackCooldown = 0.0f;  // seconds until this mob can melee again
};

} // namespace cw
