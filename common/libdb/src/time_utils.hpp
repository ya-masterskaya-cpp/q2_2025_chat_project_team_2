#pragma once

#include <chrono>
#include <cstdint>
#include <sstream>
#include <iomanip>

namespace utime {
    inline int64_t GetUnixTimeNs() {
        using namespace std::chrono;
        return duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count();
    }
    
    inline std::pair<std::string, std::string> UnixTimeToDateTime(int64_t unix_time_ns) {
        std::time_t t = static_cast<time_t>(unix_time_ns / 1'000'000'000);
        std::tm tm_struct{};

#if defined(_WIN32)
        if (localtime_s(&tm_struct, &t) != 0) {
            return { "error", "error" };
        }
#else
        if (localtime_r(&t, &tm_struct) == nullptr) {
            return { "error", "error" };
        }
#endif
        char date_buf[11], time_buf[9];
        strftime(date_buf, sizeof(date_buf), "%Y-%m-%d", &tm_struct);  // "2023-11-15"
        strftime(time_buf, sizeof(time_buf), "%H:%M:%S", &tm_struct);  // "14:30:45"

        return { date_buf, time_buf };
    }
} // utime
