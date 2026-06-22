#pragma once

namespace ce {

class Event;

// A Layer is the engine's unit of game logic. The Application drives a stack
// of Layers each frame. The game is just a Layer pushed onto the Application,
// which keeps the engine decoupled from any specific game.
class Layer {
public:
    virtual ~Layer() = default;

    virtual void onAttach() {}             // called when pushed onto the stack
    virtual void onDetach() {}             // called when removed / on shutdown
    virtual void onUpdate(float /*dt*/) {} // per-frame logic; dt in seconds
    virtual void onRender() {}             // per-frame rendering
    virtual void onEvent(Event& /*e*/) {}  // window/input events
};

} // namespace ce
