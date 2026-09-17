#include "logger.hpp"
#include "sinks.hpp"
#include "log_level.hpp"

int main()
{
    Logger logger{ConsoleSink{}};

    logger.log(LogLevel::Info, "Application started");
    logger.log(LogLevel::Error, "Configuration file not found");

    // A different, unrelated type can be stored in the same Logger.
    logger = Logger{FileSink{"application.log"}};

    logger.log(LogLevel::Info, "Switched to file logging");
    logger.log(LogLevel::Error, "Connection failed");

    return 0;
}