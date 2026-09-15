#include "logger.hpp"
#include "sinks.hpp"
#include "log_level.hpp"

int main()
{
    Logger logger{ConsoleSink{}};

    logger.log(LogLevel::info, "Application started");
    logger.log(LogLevel::error, "Configuration file not found");

    // A different, unrelated type can be stored in the same Logger.
    logger = Logger{FileSink{"application.log"}};

    logger.log(LogLevel::info, "Switched to file logging");
    logger.log(LogLevel::error, "Connection failed");

    return 0;
}