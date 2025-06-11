#pragma once

#include <chrono>
#include <cstdint>

namespace utime {
    inline int64_t GetUnixTimeNs() {
        using namespace std::chrono;
        return duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count();
    }
} // utime
