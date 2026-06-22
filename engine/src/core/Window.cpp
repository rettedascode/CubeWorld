#include "ce/core/Window.h"

#include "ce/event/ApplicationEvent.h"
#include "ce/event/KeyEvent.h"
#include "ce/event/MouseEvent.h"
#include "ce/util/Log.h"

// GLAD must be included before GLFW so it can hook the GL function pointers.
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace ce {

// Tracks live windows so we glfwInit() once and glfwTerminate() when the last
// window is destroyed.
static uint8_t s_glfwWindowCount = 0;

Window::Window(const WindowProps& props) { init(props); }
Window::~Window() { shutdown(); }

void Window::init(const WindowProps& props) {
    m_data.title  = props.title;
    m_data.width  = props.width;
    m_data.height = props.height;

    if (s_glfwWindowCount == 0) {
        if (!glfwInit()) {
            CE_LOG_ERROR("Failed to initialize GLFW");
            return;
        }
    }

    // Request an OpenGL 3.3 core profile context (no deprecated fixed-function GL).
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    m_window = glfwCreateWindow(static_cast<int>(props.width),
                                static_cast<int>(props.height),
                                m_data.title.c_str(), nullptr, nullptr);
    if (!m_window) {
        CE_LOG_ERROR("Failed to create GLFW window");
        return;
    }
    ++s_glfwWindowCount;

    glfwMakeContextCurrent(m_window);

    // Load OpenGL function pointers for this context via GLAD.
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        CE_LOG_ERROR("Failed to initialize GLAD");
        return;
    }

    glfwSwapInterval(1); // enable vsync

    // Make m_data reachable from the static GLFW callbacks below.
    glfwSetWindowUserPointer(m_window, &m_data);
    installCallbacks();

    CE_LOG_INFO("Created window '", m_data.title, "' (", m_data.width, "x",
                m_data.height, ") | OpenGL ", glGetString(GL_VERSION));
}

// Translate GLFW C callbacks into engine events and forward them to the
// Application via the stored event callback. The window's Data lives behind the
// GLFW user pointer.
void Window::installCallbacks() {
    glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* w, int width, int height) {
        auto& data  = *static_cast<Data*>(glfwGetWindowUserPointer(w));
        data.width  = static_cast<uint32_t>(width);
        data.height = static_cast<uint32_t>(height);
        glViewport(0, 0, width, height); // keep the GL viewport matched to the window
        if (data.eventCallback) {
            WindowResizeEvent e(data.width, data.height);
            data.eventCallback(e);
        }
    });

    glfwSetWindowCloseCallback(m_window, [](GLFWwindow* w) {
        auto& data = *static_cast<Data*>(glfwGetWindowUserPointer(w));
        if (data.eventCallback) {
            WindowCloseEvent e;
            data.eventCallback(e);
        }
    });

    glfwSetKeyCallback(m_window, [](GLFWwindow* w, int key, int, int action, int) {
        auto& data = *static_cast<Data*>(glfwGetWindowUserPointer(w));
        if (!data.eventCallback) return;
        switch (action) {
            case GLFW_PRESS: {
                KeyPressedEvent e(static_cast<KeyCode>(key), false);
                data.eventCallback(e);
                break;
            }
            case GLFW_REPEAT: {
                KeyPressedEvent e(static_cast<KeyCode>(key), true);
                data.eventCallback(e);
                break;
            }
            case GLFW_RELEASE: {
                KeyReleasedEvent e(static_cast<KeyCode>(key));
                data.eventCallback(e);
                break;
            }
        }
    });

    glfwSetCharCallback(m_window, [](GLFWwindow* w, unsigned int codepoint) {
        auto& data = *static_cast<Data*>(glfwGetWindowUserPointer(w));
        if (data.eventCallback) {
            CharTypedEvent e(codepoint);
            data.eventCallback(e);
        }
    });

    glfwSetMouseButtonCallback(m_window, [](GLFWwindow* w, int button, int action, int) {
        auto& data = *static_cast<Data*>(glfwGetWindowUserPointer(w));
        if (!data.eventCallback) return;
        if (action == GLFW_PRESS) {
            MouseButtonPressedEvent e(static_cast<MouseCode>(button));
            data.eventCallback(e);
        } else if (action == GLFW_RELEASE) {
            MouseButtonReleasedEvent e(static_cast<MouseCode>(button));
            data.eventCallback(e);
        }
    });

    glfwSetScrollCallback(m_window, [](GLFWwindow* w, double xOffset, double yOffset) {
        auto& data = *static_cast<Data*>(glfwGetWindowUserPointer(w));
        if (data.eventCallback) {
            MouseScrolledEvent e(static_cast<float>(xOffset), static_cast<float>(yOffset));
            data.eventCallback(e);
        }
    });

    glfwSetCursorPosCallback(m_window, [](GLFWwindow* w, double x, double y) {
        auto& data = *static_cast<Data*>(glfwGetWindowUserPointer(w));
        if (data.eventCallback) {
            MouseMovedEvent e(static_cast<float>(x), static_cast<float>(y));
            data.eventCallback(e);
        }
    });
}

void Window::shutdown() {
    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
        --s_glfwWindowCount;
    }
    if (s_glfwWindowCount == 0) {
        glfwTerminate();
    }
}

void Window::onUpdate() {
    glfwPollEvents();
    glfwSwapBuffers(m_window);
}

bool Window::shouldClose() const {
    return m_window == nullptr || glfwWindowShouldClose(m_window);
}

} // namespace ce
