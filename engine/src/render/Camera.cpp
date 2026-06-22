#include "ce/render/Camera.h"

#include <glm/gtc/matrix_transform.hpp>

namespace ce {

void Camera::setPerspective(float fovDegrees, float aspect, float nearZ, float farZ) {
    m_projection = glm::perspective(glm::radians(fovDegrees), aspect, nearZ, farZ);
    recalc();
}

void Camera::setOrthographic(float left, float right, float bottom, float top,
                             float nearZ, float farZ) {
    m_projection = glm::ortho(left, right, bottom, top, nearZ, farZ);
    recalc();
}

void Camera::setView(const glm::mat4& view) {
    m_view = view;
    recalc();
}

void Camera::setViewLookAt(const glm::vec3& eye, const glm::vec3& center,
                           const glm::vec3& up) {
    m_view = glm::lookAt(eye, center, up);
    recalc();
}

} // namespace ce
