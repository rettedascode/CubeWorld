#pragma once

#include <cmath>
#include <cstdint>

namespace cw {

constexpr uint32_t kDayLengthTicks = 4800; // 4 minutes @ 20 Hz

// Daylight amount: 0 at midnight, 1 at noon, 0 again at midnight. Shared so the
// server's spawn rules and the client's sky colour use the same curve.
inline float dayBrightness(uint32_t worldTick, uint32_t dayLength) {
    if (dayLength == 0) return 1.0f;
    const float frac = static_cast<float>(worldTick % dayLength) / static_cast<float>(dayLength);
    const float v    = std::sin(3.14159265f * frac);
    return v < 0.0f ? 0.0f : v;
}

inline bool isNight(uint32_t worldTick, uint32_t dayLength) {
    return dayBrightness(worldTick, dayLength) < 0.3f;
}

} // namespace cw
