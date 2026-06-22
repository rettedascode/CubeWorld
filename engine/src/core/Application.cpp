#include "ce/core/Application.h"

#include "ce/event/ApplicationEvent.h"
#include "ce/input/Input.h"
#include "ce/render/Renderer.h"
#include "ce/util/Log.h"

#include <GLFW/glfw3.h> // glfwGetTime for the frame clock

namespace ce {

Application::Application(const WindowProps& props)
    : m_window(std::make_unique<Window>(props)) {
    m_window->setEventCallback([this](Event& e) { onEvent(e); });
    Input::setContext(m_window->nativeHandle());
    Renderer::init();
}

Application::~Application() {
    // Detach in reverse order of attachment.
    for (auto it = m_layers.rbegin(); it != m_layers.rend(); ++it) {
        (*it)->onDetach();
    }
}

void Application::pushLayer(std::unique_ptr<Layer> layer) {
    layer->onAttach();
    m_layers.push_back(std::move(layer));
}

void Application::onEvent(Event& e) {
    EventDispatcher dispatcher(e);
    dispatcher.dispatch<WindowCloseEvent>(
        [this](WindowCloseEvent& wc) { return onWindowClose(wc); });

    // Propagate top-down (most recently pushed layer first); stop if consumed.
    for (auto it = m_layers.rbegin(); it != m_layers.rend(); ++it) {
        if (e.handled) break;
        (*it)->onEvent(e);
    }
}

bool Application::onWindowClose(WindowCloseEvent&) {
    m_running = false;
    return true;
}

void Application::run() {
    float lastTime = static_cast<float>(glfwGetTime());

    while (m_running && !m_window->shouldClose()) {
        const float now = static_cast<float>(glfwGetTime());
        const float dt   = now - lastTime;
        lastTime         = now;

        Renderer::clear(); // clear color set by a layer (e.g. sky blue)

        for (auto& layer : m_layers) layer->onUpdate(dt);
        for (auto& layer : m_layers) layer->onRender();

        m_window->onUpdate();
    }
}

} // namespace ce
