#pragma once
#include <string>
#include <atomic>

struct DownloadProgress {
    std::atomic<double> current = 0;
    std::atomic<double> total = 0;
    std::atomic<bool> finished = false;
    std::atomic<bool> failed = false;
};

void DownloadFile(std::string url, std::string savePath, DownloadProgress* progress);