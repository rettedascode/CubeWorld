#pragma once

#include <cw/entity/Entity.h>
#include <net/ByteBuffer.h>

#include <cstdint>

namespace cw {

class World;

// Movement intent bitmask carried by an InputCommand.
namespace InputButton {
enum : uint8_t {
    Forward = 1 << 0,
    Back    = 1 << 1,
    Left    = 1 << 2,
    Right   = 1 << 3,
    Jump    = 1 << 4,
    Sneak   = 1 << 5,
};
}

// One tick of player input. The client tags each with a monotonic sequence and
// the dt it used; client (prediction) and server (authority) feed it through
// applyInput() with the same world data — identical math = identical result.
struct InputCommand {
    uint32_t sequence = 0;
    float    dt       = 0.0f;
    uint8_t  buttons  = 0;
    float    yaw      = 0.0f; // radians
    float    pitch    = 0.0f;

    void serialize(net::ByteBuffer& b) const;
    void deserialize(net::ByteBuffer& b);
};

// Player physics constants (shared so server/client agree).
constexpr float kWalkSpeed       = 5.5f;
constexpr float kJumpSpeed       = 8.8f;  // initial upward speed (apex ~1.5 blocks)
constexpr float kEyeHeight       = 1.62f;
constexpr float kPlayerHalfWidth = 0.3f;
constexpr float kPlayerHeight    = 1.8f;

// Smoother jump feel. Asymmetric gravity (rise slower, fall faster) avoids the
// floaty "moon jump"; releasing jump while rising applies extra gravity for a
// shorter hop (variable height). All dt-integrated, so prediction (any client
// frame rate) and the fixed-tick server stay in agreement.
constexpr float kRiseGravity       = 26.0f; // ascending, jump held
constexpr float kLowJumpMultiplier = 2.6f;  // ascending, jump released -> cut short
constexpr float kFallGravity       = 44.0f; // descending
constexpr float kTerminalVelocity  = 55.0f; // max fall speed

// Forgiveness windows + horizontal easing.
constexpr float kCoyoteTime      = 0.10f; // jump still works briefly after a ledge
constexpr float kJumpBufferTime  = 0.12f; // a press just before landing still fires
constexpr float kGroundAccel     = 55.0f; // m/s^2 easing toward target on the ground
constexpr float kAirAccel        = 16.0f; // reduced air control

constexpr float kMobSpeed = 2.5f;

// Apply gravity to e.velocity.y, then sweep-move by velocity with AABB voxel
// collision, updating onGround and fall tracking. Caller sets velocity.x/z (and
// velocity.y for a jump) first. Shared by players and mobs.
void moveWithCollision(Entity& e, const World& world, float dt);

// Advance a player by one input command (sets horizontal velocity + jump, then
// moveWithCollision). Deterministic and rendering-free.
void applyInput(Entity& e, const InputCommand& cmd, const World& world);

} // namespace cw
