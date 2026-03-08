#pragma once
#include <string>
#include <map>
#include <fstream>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

struct LangInfo {
    std::string fileName;    // Наприклад: uk_UA
    std::string displayName; // Наприклад: Ukrainian
};

class LocManager {
private:
    std::map<std::string, std::string> strings;

public:
    std::vector<LangInfo> availableLanguages;

    void DiscoverLanguages() {
        availableLanguages.clear();
        std::string locPath = "assets/loc/";

        if (!fs::exists(locPath)) return;

        for (const auto& entry : fs::directory_iterator(locPath)) {
            if (entry.path().extension() == ".ini") {
                std::ifstream file(entry.path());
                std::string line, reg, lang;

                while (std::getline(file, line)) {
                    if (line.find("reg=") == 0) reg = line.substr(4);
                    if (line.find("lang=") == 0) lang = line.substr(5);
                    if (!reg.empty() && !lang.empty()) break;
                }

                if (!lang.empty()) {
                    std::string fName = reg.empty() ? entry.path().stem().string() : reg;
                    availableLanguages.push_back({ fName, lang });
                }
            }
        }
    }

    bool LoadLanguage(const std::string& langFileName) {
        std::string path = "assets/loc/" + langFileName + ".ini";
        std::ifstream file(path);
        if (!file.is_open()) return false;

        strings.clear();
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == ';' || line[0] == '[') continue;

            size_t delimiterPos = line.find('=');
            if (delimiterPos != std::string::npos) {
                std::string key = line.substr(0, delimiterPos);
                std::string value = line.substr(delimiterPos + 1);

                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                strings[key] = value;
            }
        }
        return true;
    }

    const char* Get(const std::string& key) {
        auto it = strings.find(key);
        return (it != strings.end()) ? it->second.c_str() : key.c_str();
    }
};

inline LocManager Loc;