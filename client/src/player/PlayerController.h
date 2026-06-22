#pragma once

#include <cw/entity/Entity.h>
#include <cw/sim/PlayerMovement.h>

#include <ce/render/Camera.h>

#include <glm/glm.hpp>

#include <cstdint>
#include <deque>

namespace cw {

class World;

// Local player: samples input, PREDICTS movement immediately (so the local view
// is lag-free), and RECONCILES against authoritative server snapshots by snapping
// to the confirmed position and replaying still-unacknowledged inputs. Movement
// now uses shared gravity + collision, so prediction needs the world.
class PlayerController {
public:
    PlayerController();

    // Sample input, predict one step, and return the command to send to the server.
    InputCommand update(float dt, const World& world);

    // Apply the server's authoritative position for input `ackSequence`, then
    // replay all inputs the server hasn't processed yet.
    void reconcile(const glm::vec3& authPosition, uint32_t ackSequence, const World& world);

    void setProjection(float fovDegrees, float aspect, float nearZ, float farZ);
    void setCaptured(bool captured);
    bool captured() const { return m_captured; }

    const ce::Camera& camera() const { return m_camera; }
    glm::vec3 position() const { return m_entity.position; }     // feet
    glm::vec3 eyePosition() const;                                // camera/raycast origin
    glm::vec3 forward() const { return m_front; }

private:
    void    updateLook();
    void    updateCamera();
    uint8_t sampleButtons() const;

    ce::Camera m_camera;
    Entity     m_entity;        // local predicted state
    glm::vec3  m_front{0.0f, 0.0f, -1.0f};
    float      m_yaw   = -1.5708f; // -90deg: facing -Z
    float      m_pitch = -0.30f;

    uint32_t                 m_sequence = 0;
    std::deque<InputCommand> m_pending;       // inputs awaiting server acknowledgement
    bool                     m_authoritative = false; // got a server position yet?

    bool      m_captured   = false;
    bool      m_firstMouse = true;
    glm::vec2 m_lastMouse{0.0f};
};

} // namespace cw
