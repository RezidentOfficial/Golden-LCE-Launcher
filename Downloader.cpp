#include "Downloader.h"
#include <windows.h>
#include <wininet.h>
#include <fstream>
#include <vector>
#include <string>
#include <iostream>

#pragma comment(lib, "wininet.lib")

// Функція для отримання прямого лінка
std::string GetMediaFireDirectLink(const std::string& pageUrl) {
    // Встановлюємо User-Agent як у браузера, щоб MediaFire віддав сторінку
    HINTERNET hInternet = InternetOpenA("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) return "";

    HINTERNET hFile = InternetOpenUrlA(hInternet, pageUrl.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hFile) {
        InternetCloseHandle(hInternet);
        return "";
    }

    std::string html = "";
    char buffer[16384];
    DWORD bytesRead;

    while (InternetReadFile(hFile, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        html += buffer;
    }

    InternetCloseHandle(hFile);
    InternetCloseHandle(hInternet);

    // Шукаємо пряме посилання за ключовим словом "https://download"
    // MediaFire зазвичай ховає його в атрибуті href кнопки завантаження
    size_t startPos = html.find("https://download");
    if (startPos == std::string::npos) return "";

    size_t endPos = html.find("\"", startPos);
    if (endPos == std::string::npos) return "";

    return html.substr(startPos, endPos - startPos);
}

void DownloadFile(std::string url, std::string savePath, DownloadProgress* progress) {
    progress->finished = false;
    progress->failed = false;
    progress->current = 0;
    progress->total = 0;

    std::string finalUrl = url;

    // Якщо посилання на MediaFire — скрапимо прямий лінк
    if (url.find("mediafire.com/file/") != std::string::npos) {
        finalUrl = GetMediaFireDirectLink(url);
        if (finalUrl.empty()) {
            progress->failed = true;
            progress->finished = true;
            return;
        }
    }

    HINTERNET hInternet = InternetOpenA("Mozilla/5.0", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) { progress->failed = true; progress->finished = true; return; }

    HINTERNET hFile = InternetOpenUrlA(hInternet, finalUrl.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hFile) {
        InternetCloseHandle(hInternet);
        progress->failed = true;
        progress->finished = true;
        return;
    }

    // Отримуємо розмір файлу
    char szContentLength[32];
    DWORD dwBufLen = sizeof(szContentLength);
    if (HttpQueryInfoA(hFile, HTTP_QUERY_CONTENT_LENGTH, szContentLength, &dwBufLen, NULL)) {
        progress->total = std::stold(szContentLength);
    }

    CreateDirectoryA("versions", NULL);

    std::ofstream out(savePath, std::ios::binary);
    if (out.is_open()) {
        char buffer[32768];
        DWORD bytesRead;
        while (InternetReadFile(hFile, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
            out.write(buffer, bytesRead);
            progress->current = progress->current + bytesRead;
        }
        out.close();
    }
    else {
        progress->failed = true;
    }

    InternetCloseHandle(hFile);
    InternetCloseHandle(hInternet);

    if (progress->current < 100000) {
        progress->failed = true;
    }

    progress->finished = true;
}