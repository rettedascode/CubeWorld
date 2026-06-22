#pragma once

#include "ce/input/KeyCodes.h"
#include "ce/input/MouseCodes.h"

#include <glm/vec2.hpp>

// Forward-declared so this public header doesn't leak <GLFW/glfw3.h>.
struct GLFWwindow;

namespace ce {

// Polling-style input. Query current state any time (typically in onUpdate),
// as opposed to the event system which pushes discrete events.
class Input {
public:
    // Engine-internal: Application calls this once with the active window.
    static void setContext(GLFWwindow* window);

    // Capture (hide + lock) the cursor for mouse-look, or release it.
    static void setCursorCaptured(bool captured);

    static bool isKeyDown(KeyCode key);
    static bool isMouseButtonDown(MouseCode button);

    static glm::vec2 mousePosition();
    static float     mouseX();
    static float     mouseY();

private:
    static GLFWwindow* s_window;
};

} // namespace ce
