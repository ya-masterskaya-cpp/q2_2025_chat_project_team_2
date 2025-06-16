// libdb/include/logger.hpp
#pragma once

#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include "spdlog/sinks/stdout_color_sinks.h"

namespace db {

class Logger {
public:

    Logger() = delete;
    ~Logger() = delete;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // Получение общего логгера для БД
    static std::shared_ptr<spdlog::logger> get() {
        static std::shared_ptr<spdlog::logger> instance = create();
        return instance;
    }

private:



    static std::shared_ptr<spdlog::logger> create() {
        try {
            auto logger = spdlog::basic_logger_mt("db_logger", "db.log");
            logger->set_level(spdlog::level::debug); // можно изменить уровень
            logger->set_pattern("[%Y-%m-%d %H:%M:%S] [%l] %v");
            return logger;
        } catch (const spdlog::spdlog_ex& ex) {
            // fallback: лог в stderr
            auto fallback = spdlog::stderr_color_mt("db_logger_fallback");
            fallback->set_level(spdlog::level::debug);
            return fallback;
        }
    }
};

} // namespace db
