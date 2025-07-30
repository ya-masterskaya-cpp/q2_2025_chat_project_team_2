#pragma once
#include <iostream>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
#include <string>

#include "General.h"

class Logger {
public:
    Logger() {
        file_logger = spdlog::get("file_logger");

        if (!file_logger) {
            try {
                spdlog::set_pattern("%Y-%m-%d %H:%M:%S %n [%l] %v");
                file_logger = spdlog::basic_logger_mt("file_logger", LOG_FILE);
                file_logger->set_level(spdlog::level::debug);
                file_logger->info("LOGGER INITIALIZED AND STARTED");
            }
            catch (const spdlog::spdlog_ex& ex) {
                std::cerr << "Log initialization failed: " << ex.what() << std::endl;
            }
        }
    }

    void logMessage(const std::string& message) {
        file_logger->info("Received message: {}", message);
    }

    void logError(const std::string& errorMessage) {
        file_logger->error("Error occurred: {}", errorMessage);
    }

    void logEvent(const std::string& eventMessage) {
        file_logger->info(eventMessage);
    }

private:
    std::shared_ptr<spdlog::logger> file_logger;
};

