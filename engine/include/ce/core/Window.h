#pragma once

#include "ce/event/Event.h"

#include <cstdint>
#include <functional>
#include <string>

// Forward-declare the GLFW window so this public header never leaks <GLFW/glfw3.h>
// into game code. GLFW stays an engine implementation detail.
struct GLFWwindow;

namespace ce {

struct WindowProps {
    std::string title  = "CubeEngine";
    uint32_t    width  = 1280;
    uint32_t    height = 720;
};

// RAII wrapper over a GLFW window with an OpenGL 3.3 core context.
// Owns a unique GPU/window resource, so it is non-copyable.
class Window {
public:
    explicit Window(const WindowProps& props = {});
    ~Window();

    Window(const Window&)            = delete;
    Window& operator=(const Window&) = delete;

    using EventCallbackFn = std::function<void(Event&)>;

    void onUpdate();          // poll events + swap buffers (called once per frame)
    bool shouldClose() const; // true once the user closes the window

    // The Application sets this to receive events emitted from GLFW callbacks.
    void setEventCallback(const EventCallbackFn& callback) {
        m_data.eventCallback = callback;
    }

    uint32_t    width()  const { return m_data.width; }
    uint32_t    height() const { return m_data.height; }
    GLFWwindow* nativeHandle() const { return m_window; }

private:
    void init(const WindowProps& props);
    void installCallbacks();
    void shutdown();

    GLFWwindow* m_window = nullptr;

    // Stored behind the GLFW window user pointer so static GLFW callbacks can
    // reach back to the owning window's state and event callback.
    struct Data {
        std::string     title;
        uint32_t        width  = 0;
        uint32_t        height = 0;
        EventCallbackFn eventCallback;
    } m_data;
};

} // namespace ce
