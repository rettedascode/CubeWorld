#pragma once

// Minimal logging utility for the engine. Uses a C++17 fold expression to
// stream any number of arguments to a single line. This is intentionally tiny;
// it can be swapped for a real logging backend later without touching callers.
#include <iostream>

namespace ce::log {

template <typename... Args>
void line(std::ostream& os, const char* level, Args&&... args) {
    os << level;
    (os << ... << args);   // C++17 fold expression over operator<<
    os << '\n';
}

} // namespace ce::log

#define CE_LOG_INFO(...)  ::ce::log::line(std::cout, "[CE INFO]  ", __VA_ARGS__)
#define CE_LOG_WARN(...)  ::ce::log::line(std::cout, "[CE WARN]  ", __VA_ARGS__)
#define CE_LOG_ERROR(...) ::ce::log::line(std::cerr, "[CE ERROR] ", __VA_ARGS__)
