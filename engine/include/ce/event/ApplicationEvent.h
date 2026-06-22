#pragma once

#include "ce/event/Event.h"

#include <cstdint>
#include <sstream>

namespace ce {

// Reports the new framebuffer size (what the renderer needs for glViewport).
class WindowResizeEvent : public Event {
public:
    WindowResizeEvent(uint32_t width, uint32_t height)
        : m_width(width), m_height(height) {}

    uint32_t width() const { return m_width; }
    uint32_t height() const { return m_height; }

    std::string toString() const override {
        std::ostringstream ss;
        ss << "WindowResize: " << m_width << "x" << m_height;
        return ss.str();
    }

    CE_EVENT_TYPE(WindowResize)

private:
    uint32_t m_width, m_height;
};

class WindowCloseEvent : public Event {
public:
    WindowCloseEvent() = default;
    CE_EVENT_TYPE(WindowClose)
};

} // namespace ce
