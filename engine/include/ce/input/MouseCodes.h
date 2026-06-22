#pragma once

#include <cstdint>

// Engine mouse-button codes. Values match GLFW's GLFW_MOUSE_BUTTON_*.
namespace ce {

using MouseCode = uint16_t;

namespace Mouse {
enum : MouseCode {
    Button0 = 0, Button1, Button2, Button3, Button4, Button5, Button6, Button7,

    ButtonLeft   = Button0,
    ButtonRight  = Button1,
    ButtonMiddle = Button2
};
} // namespace Mouse

} // namespace ce
