#pragma once

#include <cstdint>

// Engine key codes. Values intentionally match GLFW's GLFW_KEY_* so the engine
// can pass them straight through, while game code never sees the GLFW header.
namespace ce {

using KeyCode = uint16_t;

namespace Key {
enum : KeyCode {
    Space        = 32,
    Apostrophe   = 39,  // '
    Comma        = 44,  // ,
    Minus        = 45,  // -
    Period       = 46,  // .
    Slash        = 47,  // /

    D0 = 48, D1, D2, D3, D4, D5, D6, D7, D8, D9,

    Semicolon = 59,     // ;
    Equal     = 61,     // =

    A = 65, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

    LeftBracket  = 91,  // [
    Backslash    = 92,  // \ //
    RightBracket = 93,  // ]
    GraveAccent  = 96,  // `

    Escape = 256, Enter, Tab, Backspace, Insert, Delete,
    Right, Left, Down, Up,
    PageUp, PageDown, Home, End,

    CapsLock = 280, ScrollLock, NumLock, PrintScreen, Pause,

    F1 = 290, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,

    LeftShift = 340, LeftControl, LeftAlt, LeftSuper,
    RightShift, RightControl, RightAlt, RightSuper
};
} // namespace Key

} // namespace ce
