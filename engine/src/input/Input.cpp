#include "ce/input/Input.h"

#include <GLFW/glfw3.h>

namespace ce {

GLFWwindow* Input::s_window = nullptr;

void Input::setContext(GLFWwindow* window) { s_window = window; }

void Input::setCursorCaptured(bool captured) {
    if (!s_window) return;
    glfwSetInputMode(s_window, GLFW_CURSOR,
                     captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}

bool Input::isKeyDown(KeyCode key) {
    if (!s_window) return false;
    return glfwGetKey(s_window, static_cast<int>(key)) == GLFW_PRESS;
}

bool Input::isMouseButtonDown(MouseCode button) {
    if (!s_window) return false;
    return glfwGetMouseButton(s_window, static_cast<int>(button)) == GLFW_PRESS;
}

glm::vec2 Input::mousePosition() {
    if (!s_window) return {0.0f, 0.0f};
    double x = 0.0, y = 0.0;
    glfwGetCursorPos(s_window, &x, &y);
    return {static_cast<float>(x), static_cast<float>(y)};
}

float Input::mouseX() { return mousePosition().x; }
float Input::mouseY() { return mousePosition().y; }

} // namespace ce
