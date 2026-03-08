#include "GameLauncher.h"
#include <windows.h>
#include <vector>

void LaunchGame(const std::string& exePath, const std::string& workingDir, const std::string& nickname, const std::string& ipAddress) {
    std::string commandLine = "\"" + exePath + "\" -name " + nickname;
    if (!ipAddress.empty()) {
        commandLine += " -ip " + ipAddress;
    }

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    std::vector<char> cmdBuffer(commandLine.begin(), commandLine.end());
    cmdBuffer.push_back('\0');

    if (CreateProcessA(exePath.c_str(), cmdBuffer.data(), NULL, NULL, FALSE, 0, NULL, workingDir.c_str(), &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}