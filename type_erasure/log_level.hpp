#pragma once

#include <compare>
#include <string_view>
#include <utility>

enum class LogLevel {
    Debug,
    Info,
    Error,
};

constexpr std::strong_ordering operator<=>(LogLevel lhs, LogLevel rhs) noexcept
{
    return std::to_underlying(lhs) <=> std::to_underlying(rhs);
}

constexpr std::string_view to_string(LogLevel level) noexcept
{
    switch (level) {
    case LogLevel::Debug: return "DEBUG";
    case LogLevel::Info:  return "INFO";
    case LogLevel::Error: return "ERROR";
    }
    return "UNKNOWN";
}
