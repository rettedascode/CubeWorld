#pragma once

#include "ce/event/Event.h"
#include "ce/input/KeyCodes.h"

#include <sstream>

namespace ce {

class KeyEvent : public Event {
public:
    KeyCode keyCode() const { return m_key; }

protected:
    explicit KeyEvent(KeyCode key) : m_key(key) {}
    KeyCode m_key;
};

class KeyPressedEvent : public KeyEvent {
public:
    KeyPressedEvent(KeyCode key, bool repeat) : KeyEvent(key), m_repeat(repeat) {}

    bool isRepeat() const { return m_repeat; }

    std::string toString() const override {
        std::ostringstream ss;
        ss << "KeyPressed: " << m_key << (m_repeat ? " (repeat)" : "");
        return ss.str();
    }

    CE_EVENT_TYPE(KeyPressed)

private:
    bool m_repeat;
};

class KeyReleasedEvent : public KeyEvent {
public:
    explicit KeyReleasedEvent(KeyCode key) : KeyEvent(key) {}

    std::string toString() const override {
        std::ostringstream ss;
        ss << "KeyReleased: " << m_key;
        return ss.str();
    }

    CE_EVENT_TYPE(KeyReleased)
};

// A typed character (Unicode codepoint) for text input.
class CharTypedEvent : public Event {
public:
    explicit CharTypedEvent(unsigned int codepoint) : m_codepoint(codepoint) {}
    unsigned int codepoint() const { return m_codepoint; }
    CE_EVENT_TYPE(CharTyped)
private:
    unsigned int m_codepoint;
};

} // namespace ce
