#pragma once

#include <filesystem>
#include <string_view>

#include "log_level.hpp"

struct ConsoleSink {
    void log(LogLevel level, std::string_view message);
};

struct FileSink {
    explicit FileSink(std::filesystem::path path);

    void log(LogLevel level, std::string_view message);
};