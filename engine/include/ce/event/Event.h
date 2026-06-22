#pragma once

#include <ostream>
#include <string>

// Engine event system. Events are created by the Window (from GLFW callbacks)
// and flow into Application::onEvent, which dispatches them down the layer stack.
// This header is game-agnostic and public.
namespace ce {

enum class EventType {
    None = 0,
    WindowClose, WindowResize,
    KeyPressed, KeyReleased, CharTyped,
    MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled
};

class Event {
public:
    virtual ~Event() = default;

    // Set true by a handler to stop further propagation down the stack.
    bool handled = false;

    virtual EventType   type() const = 0;
    virtual const char* name() const = 0;
    virtual std::string toString() const { return name(); }
};

// Boilerplate for each concrete event: a compile-time type id + the overrides.
#define CE_EVENT_TYPE(t)                                            \
    static EventType    staticType() { return EventType::t; }       \
    EventType           type() const override { return staticType(); } \
    const char*         name() const override { return #t; }

// Type-safe dispatch: calls func only if the event matches T, and ORs the
// handler's return value into the event's handled flag.
class EventDispatcher {
public:
    explicit EventDispatcher(Event& event) : m_event(event) {}

    template <typename T, typename F>
    bool dispatch(const F& func) {
        if (m_event.type() == T::staticType()) {
            m_event.handled |= func(static_cast<T&>(m_event));
            return true;
        }
        return false;
    }

private:
    Event& m_event;
};

inline std::ostream& operator<<(std::ostream& os, const Event& e) {
    return os << e.toString();
}

} // namespace ce
