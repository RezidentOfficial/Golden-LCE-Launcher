#pragma once
#include <string>

struct Config {
    std::string nickname = "Player";
    std::string ip = "26.0.0.1";
    std::string lastVersion = "";
    std::string language = "en_US";
};

Config LoadConfig();
void SaveConfig(const Config& cfg);