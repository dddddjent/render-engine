#pragma once

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include <spdlog/spdlog.h>

struct Configuration;

class Logger {
    std::shared_ptr<spdlog::logger> console;
    std::shared_ptr<spdlog::logger> file;

public:
    Logger() = default;
    void init(Configuration& config);

    constexpr auto& Console() const
    {
        return console;
    }

    constexpr auto& File() const
    {
        return file;
    }
};

extern Logger logger;

#define TRACE_FILE(...) \
    SPDLOG_LOGGER_TRACE(logger.File(), __VA_ARGS__)
#define DEBUG_FILE(...) \
    SPDLOG_LOGGER_DEBUG(logger.File(), __VA_ARGS__)
#define INFO_FILE(...) \
    SPDLOG_LOGGER_INFO(logger.File(), __VA_ARGS__)
#define WARN_FILE(...) \
    SPDLOG_LOGGER_WARN(logger.File(), __VA_ARGS__)
#define ERROR_FILE(...) \
    SPDLOG_LOGGER_ERROR(logger.File(), __VA_ARGS__)
#define CRITICAL_FILE(...) \
    SPDLOG_LOGGER_CRITICAL(logger.File(), __VA_ARGS__)

#define TRACE_CONSOLE(...) \
    SPDLOG_LOGGER_TRACE(logger.Console(), __VA_ARGS__)
#define DEBUG_CONSOLE(...) \
    SPDLOG_LOGGER_DEBUG(logger.Console(), __VA_ARGS__)
#define INFO_CONSOLE(...) \
    SPDLOG_LOGGER_INFO(logger.Console(), __VA_ARGS__)
#define WARN_CONSOLE(...) \
    SPDLOG_LOGGER_WARN(logger.Console(), __VA_ARGS__)
#define ERROR_CONSOLE(...) \
    SPDLOG_LOGGER_ERROR(logger.Console(), __VA_ARGS__)
#define CRITICAL_CONSOLE(...) \
    SPDLOG_LOGGER_CRITICAL(logger.Console(), __VA_ARGS__)

#define TRACE_ALL(...)                                 \
    SPDLOG_LOGGER_TRACE(logger.Console(), __VA_ARGS__); \
    SPDLOG_LOGGER_TRACE(logger.File(), __VA_ARGS__)
#define DEBUG_ALL(...)                                 \
    SPDLOG_LOGGER_DEBUG(logger.Console(), __VA_ARGS__); \
    SPDLOG_LOGGER_DEBUG(logger.File(), __VA_ARGS__)
#define INFO_ALL(...)                                 \
    SPDLOG_LOGGER_INFO(logger.Console(), __VA_ARGS__); \
    SPDLOG_LOGGER_INFO(logger.File(), __VA_ARGS__)
#define WARN_ALL(...)                                 \
    SPDLOG_LOGGER_WARN(logger.Console(), __VA_ARGS__); \
    SPDLOG_LOGGER_WARN(logger.File(), __VA_ARGS__)
#define ERROR_ALL(...)                                 \
    SPDLOG_LOGGER_ERROR(logger.Console(), __VA_ARGS__); \
    SPDLOG_LOGGER_ERROR(logger.File(), __VA_ARGS__)
#define CRITICAL_ALL(...)                                 \
    SPDLOG_LOGGER_CRITICAL(logger.Console(), __VA_ARGS__); \
    SPDLOG_LOGGER_CRITICAL(logger.File(), __VA_ARGS__)
