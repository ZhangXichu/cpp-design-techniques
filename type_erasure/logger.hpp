#pragma once

#include <memory>
#include <utility>

#include "sinks.hpp"

class LoggerBase {
public:
    virtual ~LoggerBase() = default;
    virtual void log(LogLevel level, std::string_view message) const = 0;
};

template <typename Sink>
concept SinkConcept = requires(Sink sink,
    LogLevel level,
    std::string_view message) {
        sink.log(level, message);
    };
template <SinkConcept Sink>
class LoggerWrpper : public LoggerBase {
public:
    explicit LoggerWrpper(Sink sink)
        : m_sink(std::move(sink)) {}

    void log(LogLevel level, std::string_view message) const override
    {
        m_sink.log(level, message);
    }

private:
    mutable Sink m_sink;
};

class Logger
{
public:
    template <SinkConcept Sink>
    Logger(Sink sink)
        : m_logger(std::make_shared<LoggerWrpper<Sink>>(std::move(sink))) {}

    void log(LogLevel level, std::string_view message) const
    {
        m_logger->log(level, message);
    }
private:
    std::shared_ptr<LoggerBase> m_logger;
};

