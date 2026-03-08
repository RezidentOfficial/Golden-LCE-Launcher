#include "Config.h"
#include <fstream>

Config LoadConfig() {
    Config cfg;
    std::ifstream file("launcher_config.txt");
    if (file.is_open()) {
        std::getline(file, cfg.nickname);
        std::getline(file, cfg.ip);
        std::getline(file, cfg.lastVersion);
        std::getline(file, cfg.language);
    }
    return cfg;
}

void SaveConfig(const Config& cfg) {
    std::ofstream file("launcher_config.txt");
    if (file.is_open()) {
        file << cfg.nickname << "\n" << cfg.ip << "\n" << cfg.lastVersion << "\n" << cfg.language << "\n";
    }
}