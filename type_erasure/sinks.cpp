#include "sinks.hpp"

#include <fstream>
#include <iostream>
#include <utility>

FileSink::FileSink(std::filesystem::path path)
    : m_path(std::move(path)) {}

void ConsoleSink::log(LogLevel level, std::string_view message)
{
    std::cout << '[' << to_string(level) << "] " << message << std::endl;
}

void FileSink::log(LogLevel level, std::string_view message)
{
    std::ofstream file(m_path);
    file << '[' << to_string(level) << "] " << message << std::endl;
}