#include "logger.h"
#include "core/config/config.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include <filesystem>

Logger logger;

void Logger::init(Configuration& config)
{
    JSON_GET(LoggerConfiguration, logger_cfg, config, "logger");

    spdlog::set_pattern("[%^%L%$] [%m/%d/%C %H:%M:%S:%f] [thread %t] [%s:%#]\n%v");

    if (std::filesystem::exists(logger_cfg.output)) {
        std::filesystem::remove(logger_cfg.output);
    }
    file = spdlog::basic_logger_mt("file", logger_cfg.output);
    console = spdlog::stdout_color_mt("console");

    spdlog::set_level(spdlog::level::trace);
    file->set_level(spdlog::level::trace);
    console->set_level(spdlog::level::trace);
    if (logger_cfg.level == "trace") {
        spdlog::set_level(spdlog::level::trace);
        file->set_level(spdlog::level::trace);
        console->set_level(spdlog::level::trace);
    } else if (logger_cfg.level == "debug") {
        spdlog::set_level(spdlog::level::debug);
        file->set_level(spdlog::level::debug);
        console->set_level(spdlog::level::debug);
    } else if (logger_cfg.level == "info") {
        spdlog::set_level(spdlog::level::info);
        file->set_level(spdlog::level::info);
        console->set_level(spdlog::level::info);
    } else if (logger_cfg.level == "warn") {
        spdlog::set_level(spdlog::level::warn);
        file->set_level(spdlog::level::warn);
        console->set_level(spdlog::level::warn);
    } else if (logger_cfg.level == "error") {
        spdlog::set_level(spdlog::level::err);
        file->set_level(spdlog::level::err);
        console->set_level(spdlog::level::err);
    } else if (logger_cfg.level == "critical") {
        spdlog::set_level(spdlog::level::critical);
        file->set_level(spdlog::level::critical);
        console->set_level(spdlog::level::critical);
    } else if (logger_cfg.level == "off") {
        spdlog::set_level(spdlog::level::off);
        file->set_level(spdlog::level::off);
        console->set_level(spdlog::level::off);
    } else {
        throw std::runtime_error("Invalid log level: " + logger_cfg.level);
    }
}
