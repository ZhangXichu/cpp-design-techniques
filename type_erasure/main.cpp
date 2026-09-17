#include "logger.hpp"
#include "sinks.hpp"
#include "log_level.hpp"

int main()
{
    Logger logger{ConsoleSink{}, LogLevel::Info};

    logger.log(LogLevel::Debug, "Connecting to database");
    logger.log(LogLevel::Info, "Application started");
    logger.log(LogLevel::Error, "Configuration file not found");

    // A different, unrelated type can be stored in the same Logger.
    logger = Logger{FileSink{"application.log"}, LogLevel::Error};

    logger.log(LogLevel::Info, "Switched to file logging");
    logger.log(LogLevel::Error, "Connection failed");

    return 0;
}
