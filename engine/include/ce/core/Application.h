#pragma once

#include "ce/core/Layer.h"
#include "ce/core/Window.h"

#include <memory>
#include <vector>

namespace ce {

class Event;
class WindowCloseEvent;

// Owns the window and runs the main loop, driving a stack of Layers.
// The game subclasses nothing here — it just pushes its own Layer.
class Application {
public:
    explicit Application(const WindowProps& props = {});
    ~Application();

    Application(const Application&)            = delete;
    Application& operator=(const Application&) = delete;

    void run();

    // Takes ownership of the layer and calls its onAttach().
    void pushLayer(std::unique_ptr<Layer> layer);

    Window& window() { return *m_window; }

private:
    void onEvent(Event& e);
    bool onWindowClose(WindowCloseEvent& e);

    std::unique_ptr<Window>             m_window;
    std::vector<std::unique_ptr<Layer>> m_layers;
    bool                                m_running = true;
};

} // namespace ce
