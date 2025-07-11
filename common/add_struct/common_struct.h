#pragma once

#include <string>
#include <vector>
#include "../chat_server_lib/src/json.h"

struct MesBuilder
{
    MesBuilder(int type) {
        j["type"] = type;
    }
    MesBuilder& add(const std::string& key, const std::string& value) {
        j[key] = value;
        return *this;
    }
    MesBuilder& add(const std::string& key, const std::vector<std::string>& value) {
        j[key] = value;
        return *this;
    }
    std::string toString() {
        return j.dump();
    }
    nlohmann::json j;
};