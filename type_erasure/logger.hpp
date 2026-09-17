#pragma once

#include <concepts>
#include <memory>
#include <utility>

#include "sinks.hpp"

class LoggerBase {
public:
    virtual ~LoggerBase() = default;
    virtual std::unique_ptr<LoggerBase> clone() const = 0;
    virtual void log(LogLevel level, std::string_view message) const = 0;
};

template <typename Sink>
concept SinkConcept = std::copyable<Sink> && requires(Sink sink,
    LogLevel level,
    std::string_view message) {
        sink.log(level, message);
    };
template <SinkConcept Sink>
class LoggerWrpper : public LoggerBase {
public:
    explicit LoggerWrpper(Sink sink)
        : m_sink(std::move(sink)) {}

    std::unique_ptr<LoggerBase> clone() const override
    {
        return std::make_unique<LoggerWrpper>(*this);
    }

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
        : m_logger(std::make_unique<LoggerWrpper<Sink>>(std::move(sink))) {}

    Logger(const Logger& other)
        : m_logger(other.m_logger ? other.m_logger->clone() : nullptr) {}

    Logger(Logger&&) noexcept = default;

    Logger& operator=(const Logger& other)
    {
        Logger tmp(other);
        *this = std::move(tmp);
        return *this;
    }

    Logger& operator=(Logger&&) noexcept = default;

    void log(LogLevel level, std::string_view message) const
    {
        m_logger->log(level, message);
    }
private:
    std::unique_ptr<LoggerBase> m_logger;
};
