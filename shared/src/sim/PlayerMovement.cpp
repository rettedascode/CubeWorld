#include <cw/sim/PlayerMovement.h>

#include <cw/world/World.h>

#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>

namespace cw {

void InputCommand::serialize(net::ByteBuffer& b) const {
    b.writeU32(sequence);
    b.writeFloat(dt);
    b.writeU8(buttons);
    b.writeFloat(yaw);
    b.writeFloat(pitch);
}

void InputCommand::deserialize(net::ByteBuffer& b) {
    sequence = b.readU32();
    dt       = b.readFloat();
    buttons  = b.readU8();
    yaw      = b.readFloat();
    pitch    = b.readFloat();
}

namespace {
constexpr float kEps = 1.0e-3f;
constexpr float kHW  = kPlayerHalfWidth;
constexpr float kH   = kPlayerHeight;

bool blocksMovement(const World& w, int x, int y, int z) {
    return isOpaque(w.getBlock(x, y, z)); // air + water are passable
}

// Move along X by dx, clamping to the first solid block hit (sweep < 1 block).
void moveX(Entity& e, const World& w, float dx) {
    e.position.x += dx;
    if (dx == 0.0f) return;
    const int y0 = static_cast<int>(std::floor(e.position.y));
    const int y1 = static_cast<int>(std::floor(e.position.y + kH - kEps));
    const int z0 = static_cast<int>(std::floor(e.position.z - kHW));
    const int z1 = static_cast<int>(std::floor(e.position.z + kHW));
    if (dx > 0.0f) {
        const int x = static_cast<int>(std::floor(e.position.x + kHW));
        for (int y = y0; y <= y1; ++y)
            for (int z = z0; z <= z1; ++z)
                if (blocksMovement(w, x, y, z)) {
                    e.position.x = x - kHW - kEps; e.velocity.x = 0.0f; return;
                }
    } else {
        const int x = static_cast<int>(std::floor(e.position.x - kHW));
        for (int y = y0; y <= y1; ++y)
            for (int z = z0; z <= z1; ++z)
                if (blocksMovement(w, x, y, z)) {
                    e.position.x = (x + 1) + kHW + kEps; e.velocity.x = 0.0f; return;
                }
    }
}

void moveZ(Entity& e, const World& w, float dz) {
    e.position.z += dz;
    if (dz == 0.0f) return;
    const int y0 = static_cast<int>(std::floor(e.position.y));
    const int y1 = static_cast<int>(std::floor(e.position.y + kH - kEps));
    const int x0 = static_cast<int>(std::floor(e.position.x - kHW));
    const int x1 = static_cast<int>(std::floor(e.position.x + kHW));
    if (dz > 0.0f) {
        const int z = static_cast<int>(std::floor(e.position.z + kHW));
        for (int y = y0; y <= y1; ++y)
            for (int x = x0; x <= x1; ++x)
                if (blocksMovement(w, x, y, z)) {
                    e.position.z = z - kHW - kEps; e.velocity.z = 0.0f; return;
                }
    } else {
        const int z = static_cast<int>(std::floor(e.position.z - kHW));
        for (int y = y0; y <= y1; ++y)
            for (int x = x0; x <= x1; ++x)
                if (blocksMovement(w, x, y, z)) {
                    e.position.z = (z + 1) + kHW + kEps; e.velocity.z = 0.0f; return;
                }
    }
}

// Move along Y by dy; downward collision sets onGround.
void moveY(Entity& e, const World& w, float dy) {
    e.position.y += dy;
    if (dy == 0.0f) return;
    const int x0 = static_cast<int>(std::floor(e.position.x - kHW));
    const int x1 = static_cast<int>(std::floor(e.position.x + kHW));
    const int z0 = static_cast<int>(std::floor(e.position.z - kHW));
    const int z1 = static_cast<int>(std::floor(e.position.z + kHW));
    if (dy < 0.0f) {
        const int y = static_cast<int>(std::floor(e.position.y));
        for (int x = x0; x <= x1; ++x)
            for (int z = z0; z <= z1; ++z)
                if (blocksMovement(w, x, y, z)) {
                    e.position.y = (y + 1) + kEps; e.velocity.y = 0.0f;
                    e.onGround = true; return;
                }
    } else {
        const int y = static_cast<int>(std::floor(e.position.y + kH));
        for (int x = x0; x <= x1; ++x)
            for (int z = z0; z <= z1; ++z)
                if (blocksMovement(w, x, y, z)) {
                    e.position.y = y - kH - kEps; e.velocity.y = 0.0f; return;
                }
    }
}
} // namespace

void moveWithCollision(Entity& e, const World& world, float dt) {
    // Asymmetric gravity: rise slower than you fall, and fall even faster when a
    // jump was released early (variable height). jumpHeld defaults false, so mobs
    // — which never jump — simply use the fall gravity once airborne.
    float g;
    if (e.velocity.y > 0.0f)
        g = e.jumpHeld ? kRiseGravity : kRiseGravity * kLowJumpMultiplier;
    else
        g = kFallGravity;
    e.velocity.y -= g * dt;
    if (e.velocity.y < -kTerminalVelocity) e.velocity.y = -kTerminalVelocity;

    const bool wasGround = e.onGround;
    e.onGround = false;

    moveX(e, world, e.velocity.x * dt);
    moveZ(e, world, e.velocity.z * dt);
    moveY(e, world, e.velocity.y * dt);

    // Track fall distance for fall damage (server reads pendingFall on landing).
    if (!e.onGround) {
        e.fallPeakY = wasGround ? e.position.y : std::max(e.fallPeakY, e.position.y);
    } else if (!wasGround) {
        e.pendingFall = std::max(0.0f, e.fallPeakY - e.position.y);
    }
}

void applyInput(Entity& e, const InputCommand& cmd, const World& world) {
    e.yaw   = cmd.yaw;
    e.pitch = cmd.pitch;

    // Horizontal wish direction from yaw + WASD.
    const glm::vec3 flatFront{std::cos(cmd.yaw), 0.0f, std::sin(cmd.yaw)};
    const glm::vec3 right{-std::sin(cmd.yaw), 0.0f, std::cos(cmd.yaw)};
    glm::vec3 wish{0.0f};
    if (cmd.buttons & InputButton::Forward) wish += flatFront;
    if (cmd.buttons & InputButton::Back)    wish -= flatFront;
    if (cmd.buttons & InputButton::Right)   wish += right;
    if (cmd.buttons & InputButton::Left)    wish -= right;
    if (glm::length(wish) > 0.0001f) wish = glm::normalize(wish);

    // Ease horizontal velocity toward the target instead of snapping, so starts
    // and stops are smooth. Less control while airborne. A zero target eases the
    // speed back down (acts as friction).
    const glm::vec2 target{wish.x * kWalkSpeed, wish.z * kWalkSpeed};
    const float     accel = e.onGround ? kGroundAccel : kAirAccel;
    glm::vec2       vel{e.velocity.x, e.velocity.z};
    const glm::vec2 diff = target - vel;
    const float     diffLen = glm::length(diff);
    const float     step    = accel * cmd.dt;
    vel = (diffLen <= step || diffLen < 1.0e-6f) ? target : vel + diff * (step / diffLen);
    e.velocity.x = vel.x;
    e.velocity.z = vel.y;

    // Jump with coyote time + input buffering for a forgiving, smooth feel.
    const bool jumpDown = (cmd.buttons & InputButton::Jump) != 0;
    e.jumpHeld = jumpDown;
    e.coyoteTimer     = e.onGround ? kCoyoteTime
                                   : std::max(0.0f, e.coyoteTimer - cmd.dt);
    e.jumpBufferTimer = jumpDown ? kJumpBufferTime
                                 : std::max(0.0f, e.jumpBufferTimer - cmd.dt);
    if (e.coyoteTimer > 0.0f && e.jumpBufferTimer > 0.0f) {
        e.velocity.y      = kJumpSpeed;
        e.coyoteTimer     = 0.0f; // consume both so we don't double-jump in air
        e.jumpBufferTimer = 0.0f;
    }

    moveWithCollision(e, world, cmd.dt);
}

} // namespace cw
