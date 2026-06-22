#include "player/PlayerController.h"

#include <ce/input/Input.h>

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace cw {

namespace {
constexpr float  kSensitivity = 0.1f; // degrees per pixel
constexpr size_t kMaxPending  = 1024; // safety cap on un-acked inputs
const glm::vec3  kSpawn{0.0f, 50.0f, 0.0f};
} // namespace

PlayerController::PlayerController() {
    m_entity.position = kSpawn; // server spawns here too; reconciliation corrects
    updateLook();
    updateCamera();
}

glm::vec3 PlayerController::eyePosition() const {
    return m_entity.position + glm::vec3(0.0f, kEyeHeight, 0.0f);
}

void PlayerController::updateLook() {
    m_front = glm::normalize(glm::vec3(std::cos(m_pitch) * std::cos(m_yaw),
                                       std::sin(m_pitch),
                                       std::cos(m_pitch) * std::sin(m_yaw)));
}

void PlayerController::updateCamera() {
    const glm::vec3 eye = eyePosition();
    m_camera.setViewLookAt(eye, eye + m_front);
}

uint8_t PlayerController::sampleButtons() const {
    uint8_t b = 0;
    if (ce::Input::isKeyDown(ce::Key::W))         b |= InputButton::Forward;
    if (ce::Input::isKeyDown(ce::Key::S))         b |= InputButton::Back;
    if (ce::Input::isKeyDown(ce::Key::A))         b |= InputButton::Left;
    if (ce::Input::isKeyDown(ce::Key::D))         b |= InputButton::Right;
    if (ce::Input::isKeyDown(ce::Key::Space))     b |= InputButton::Jump;
    if (ce::Input::isKeyDown(ce::Key::LeftShift)) b |= InputButton::Sneak;
    return b;
}

InputCommand PlayerController::update(float dt, const World& world) {
    if (m_captured) {
        const glm::vec2 mouse = ce::Input::mousePosition();
        if (m_firstMouse) {
            m_lastMouse  = mouse;
            m_firstMouse = false;
        }
        const glm::vec2 delta = mouse - m_lastMouse;
        m_lastMouse = mouse;

        m_yaw   += glm::radians(delta.x * kSensitivity);
        m_pitch -= glm::radians(delta.y * kSensitivity);
        m_pitch  = std::clamp(m_pitch, glm::radians(-89.0f), glm::radians(89.0f));
    }
    updateLook();

    InputCommand cmd;
    cmd.sequence = ++m_sequence;
    cmd.dt       = dt;
    cmd.buttons  = m_captured ? sampleButtons() : 0;
    cmd.yaw      = m_yaw;
    cmd.pitch    = m_pitch;

    // Predict immediately (once we know our authoritative position) and remember
    // the input for reconciliation. Before the first snapshot we don't move — the
    // server places us on the ground and the first reconcile snaps us there.
    if (m_authoritative) applyInput(m_entity, cmd, world);
    m_pending.push_back(cmd);
    if (m_pending.size() > kMaxPending) m_pending.pop_front();

    updateCamera();
    return cmd;
}

void PlayerController::reconcile(const glm::vec3& authPosition, uint32_t ackSequence,
                                const World& world) {
    m_entity.position = authPosition;
    m_authoritative   = true;

    // Drop inputs the server has already applied...
    while (!m_pending.empty() && m_pending.front().sequence <= ackSequence)
        m_pending.pop_front();

    // ...and replay the rest on top of the authoritative position.
    for (const auto& cmd : m_pending) applyInput(m_entity, cmd, world);

    updateCamera();
}

void PlayerController::setProjection(float fovDegrees, float aspect, float nearZ, float farZ) {
    m_camera.setPerspective(fovDegrees, aspect, nearZ, farZ);
}

void PlayerController::setCaptured(bool captured) {
    m_captured   = captured;
    m_firstMouse = true;
    ce::Input::setCursorCaptured(captured);
}

} // namespace cw
