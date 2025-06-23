#pragma once
#include <iostream>
#include <string>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include "General.h"

class Logger {
public:
    Logger() {
        spdlog::set_pattern("%Y-%m-%d %H:%M:%S %n [%l] %v"); //ôîðìàò çàïèñè â ôàéë  2025-06-18 18:28:42 file_logger [info] Received message: Message 1
        file_logger = spdlog::basic_logger_mt("file_logger", LOG_FILE);
        file_logger->set_level(spdlog::level::debug);
        file_logger->info("LOGGER  START");
       // std::cout << "LOGGER  START\n";
    }

    void logMessage(const std::string& message) {
        file_logger->info("Received message: {}", message);
        //std::cout << message << std::endl;
    }

    void logError(const std::string& errorMessage) {
        file_logger->error("Error occurred: {}", errorMessage);
        //std::cout << errorMessage << std::endl;
    }

    void logEvent(const std::string& eventMessage) {
        file_logger->info(eventMessage);
        //std::cout << eventMessage << std::endl;
    }

private:
    std::shared_ptr<spdlog::logger> file_logger;
};

