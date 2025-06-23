#pragma once
#include <iostream>
#include <string>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include "General.h"

class Logger {
public:
    Logger() {
        // Проверка судествования логгера по имени.
        file_logger = spdlog::get("file_logger");

        // Если file_logger все еще nullptr, значит, его нужно создать.
        if (!file_logger) {
            try {
                spdlog::set_pattern("%Y-%m-%d %H:%M:%S %n [%l] %v");//ôîðìàò çàïèñè â ôàéë  2025-06-18 18:28:42 file_logger [info] Received message: Message 1
                file_logger = spdlog::basic_logger_mt("file_logger", LOG_FILE);
                file_logger->set_level(spdlog::level::debug);
                file_logger->info("LOGGER INITIALIZED AND STARTED");
            }
            catch (const spdlog::spdlog_ex& ex) {
                // Обработка возможной ошибки при создании файла лога
                std::cerr << "Log initialization failed: " << ex.what() << std::endl;
            }
        }
        // Если логгер уже существовал, мы ничего не делаем,
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

