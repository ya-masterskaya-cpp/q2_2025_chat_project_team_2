#pragma once
#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>


struct IncomingMessage {
    std::string sender;
    std::string room;
    std::string text;
    std::chrono::system_clock::time_point timestamp;

    std::string formatted_time() const {
        auto t = std::chrono::system_clock::to_time_t(timestamp);
        auto tm = *std::localtime(&t);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%H:%M:%S");
        return oss.str();
    }

    std::string to_string() const{
        return formatted_time() + " - " + sender + " - " + text;
    }
};

struct OutgoingMessage {
    std::string room;
    std::string text;

    std::string serialize() const {
        return room + "|" + text;
    }

    static OutgoingMessage deserialize(const std::string& str) {
        size_t pos = str.find('|');
        if (pos == std::string::npos) {
            return { "",str };
        }
        return { str.substr(0, pos), str.substr(pos + 1) };
    }
};

//для контекстного меню 1-32767
enum CustomIDs {
    ID_CREATE_PRIVATE_CHAT = 10000,
    ID_TRAY_RESTORE = 10100,
    ID_TRAY_EXIT = 10101
};

