#pragma once

#include "ce/event/Event.h"
#include "ce/input/MouseCodes.h"

#include <sstream>

namespace ce {

class MouseMovedEvent : public Event {
public:
    MouseMovedEvent(float x, float y) : m_x(x), m_y(y) {}

    float x() const { return m_x; }
    float y() const { return m_y; }

    std::string toString() const override {
        std::ostringstream ss;
        ss << "MouseMoved: " << m_x << ", " << m_y;
        return ss.str();
    }

    CE_EVENT_TYPE(MouseMoved)

private:
    float m_x, m_y;
};

class MouseScrolledEvent : public Event {
public:
    MouseScrolledEvent(float xOffset, float yOffset)
        : m_xOffset(xOffset), m_yOffset(yOffset) {}

    float xOffset() const { return m_xOffset; }
    float yOffset() const { return m_yOffset; }

    std::string toString() const override {
        std::ostringstream ss;
        ss << "MouseScrolled: " << m_xOffset << ", " << m_yOffset;
        return ss.str();
    }

    CE_EVENT_TYPE(MouseScrolled)

private:
    float m_xOffset, m_yOffset;
};

class MouseButtonEvent : public Event {
public:
    MouseCode button() const { return m_button; }

protected:
    explicit MouseButtonEvent(MouseCode button) : m_button(button) {}
    MouseCode m_button;
};

class MouseButtonPressedEvent : public MouseButtonEvent {
public:
    explicit MouseButtonPressedEvent(MouseCode button) : MouseButtonEvent(button) {}

    std::string toString() const override {
        std::ostringstream ss;
        ss << "MouseButtonPressed: " << m_button;
        return ss.str();
    }

    CE_EVENT_TYPE(MouseButtonPressed)
};

class MouseButtonReleasedEvent : public MouseButtonEvent {
public:
    explicit MouseButtonReleasedEvent(MouseCode button) : MouseButtonEvent(button) {}

    std::string toString() const override {
        std::ostringstream ss;
        ss << "MouseButtonReleased: " << m_button;
        return ss.str();
    }

    CE_EVENT_TYPE(MouseButtonReleased)
};

} // namespace ce
