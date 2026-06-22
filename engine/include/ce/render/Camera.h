#pragma once

#include <glm/glm.hpp>

namespace ce {

// Holds projection + view matrices and their product. Game-agnostic: the game
// (or a higher-level controller) decides how to position it.
class Camera {
public:
    Camera() = default;

    void setPerspective(float fovDegrees, float aspect, float nearZ, float farZ);
    void setOrthographic(float left, float right, float bottom, float top,
                         float nearZ = -1.0f, float farZ = 1.0f);

    void setView(const glm::mat4& view);
    void setViewLookAt(const glm::vec3& eye, const glm::vec3& center,
                       const glm::vec3& up = {0.0f, 1.0f, 0.0f});

    const glm::mat4& projection() const { return m_projection; }
    const glm::mat4& view() const { return m_view; }
    const glm::mat4& viewProjection() const { return m_viewProjection; }

private:
    void recalc() { m_viewProjection = m_projection * m_view; }

    glm::mat4 m_projection{1.0f};
    glm::mat4 m_view{1.0f};
    glm::mat4 m_viewProjection{1.0f};
};

} // namespace ce
