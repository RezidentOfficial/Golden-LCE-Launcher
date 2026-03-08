#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include <windows.h>
#include <filesystem>
#include <vector>
#include <optional>
#include <cctype>
#include <shellapi.h>
#include <thread>
#include <atomic>
#include <string>

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include "Config.h"
#include "GameLauncher.h"
#include "Theme.h"
#include "Localization.h"
#include "Downloader.h" 

using json = nlohmann::json;
namespace fs = std::filesystem;

enum class AppState { Launcher, Settings };

struct RemoteVersion {
    std::string name;
    std::string url;
};

DownloadProgress dlProgress;
std::atomic<bool> isDownloading = false;
std::atomic<bool> isExtracting = false;

bool isOfflineMode = false;

void UpdateBackgroundScale(sf::Sprite& sprite, const sf::Vector2u& windowSize) {
    sf::Vector2f textureSize = sf::Vector2f(sprite.getTexture().getSize());
    float scaleX = (float)windowSize.x / textureSize.x;
    float scaleY = (float)windowSize.y / textureSize.y;
    float scale = std::max(scaleX, scaleY);
    sprite.setScale({ scale, scale });
    float posX = (windowSize.x - textureSize.x * scale) / 2.0f;
    float posY = (windowSize.y - textureSize.y * scale) / 2.0f;
    sprite.setPosition({ posX, posY });
}

int EnglishOnlyFilter(ImGuiInputTextCallbackData* data) {
    if (data->EventChar < 256 && (isalnum((unsigned char)data->EventChar) || data->EventChar == '_')) return 0;
    return 1;
}

int main() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    fs::path exeDir = fs::path(buffer).parent_path();
    if (!fs::exists(exeDir / "versions")) fs::create_directories(exeDir / "versions");
    fs::current_path(exeDir);

    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    sf::RenderWindow window(desktop, "Golden LCE", sf::Style::Default);
    window.setFramerateLimit(60);
    ShowWindow(window.getNativeHandle(), SW_MAXIMIZE);

    sf::Texture bgTexture;
    sf::Sprite* bgSprite = nullptr;
    bool hasBg = false;
    if (bgTexture.loadFromFile("assets/bg/1.png")) {
        bgSprite = new sf::Sprite(bgTexture);
        hasBg = true;
        UpdateBackgroundScale(*bgSprite, window.getSize());
    }

    (void)ImGui::SFML::Init(window);
    SetupModernTheme();

    ImGuiIO& io = ImGui::GetIO();
    ImFont* mainFont = io.Fonts->AddFontFromFileTTF("assets/fonts/main.ttf", 20.0f, NULL, io.Fonts->GetGlyphRangesCyrillic());
    if (mainFont) io.FontDefault = mainFont;
    (void)ImGui::SFML::UpdateFontTexture();

    Config cfg = LoadConfig();
    char nicknameBuf[64];
    char ipBuf[64];
    strncpy_s(nicknameBuf, cfg.nickname.c_str(), sizeof(nicknameBuf));
    strncpy_s(ipBuf, cfg.ip.c_str(), sizeof(ipBuf));

    Loc.DiscoverLanguages();
    int selectedLanguageIndex = 0;
    if (!Loc.availableLanguages.empty()) {
        for (int i = 0; i < (int)Loc.availableLanguages.size(); i++) {
            if (Loc.availableLanguages[i].fileName == cfg.language) {
                selectedLanguageIndex = i; break;
            }
        }
        Loc.LoadLanguage(Loc.availableLanguages[selectedLanguageIndex].fileName);
    }

    std::vector<RemoteVersion> availableVersions;
    int selectedVersion = 0;

    auto FetchManifest = [&]() {
        availableVersions.clear();
        isOfflineMode = false;
        bool fetchSuccess = false;

        std::string url = "https://pastebin.com/raw/SG168yPr"; //pastebin json version metadata.
        auto response = cpr::Get(cpr::Url{ url }, cpr::Timeout{ 5000 });

        if (response.status_code == 200) {
            try {
                auto data = json::parse(response.text);
                if (data.contains("versions")) {
                    for (auto& [name, durl] : data["versions"].items()) {
                        availableVersions.push_back({ name, durl.get<std::string>() });
                    }
                    fetchSuccess = true;
                }
            }
            catch (...) { fetchSuccess = false; }
        }

        if (!fetchSuccess) {
            isOfflineMode = true;
            if (fs::exists("versions")) {
                for (const auto& entry : fs::directory_iterator("versions")) {
                    if (entry.is_directory()) {
                        availableVersions.push_back({ entry.path().filename().string(), "" });
                    }
                }
            }
        }

        selectedVersion = 0;
        if (!availableVersions.empty()) {
            for (int i = 0; i < (int)availableVersions.size(); ++i) {
                if (availableVersions[i].name == cfg.lastVersion) {
                    selectedVersion = i;
                    break;
                }
            }
        }
        };

    FetchManifest();

    AppState currentState = AppState::Launcher;
    sf::Clock deltaClock;
    static bool forceUpdate = false;
    sf::Image appIcon;
    if (appIcon.loadFromFile("assets/icon.png")) {
        window.setIcon(appIcon);
    }

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *event);
            if (event->is<sf::Event::Closed>()) window.close();
            if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                sf::FloatRect visibleArea({ 0.f, 0.f }, { (float)resized->size.x, (float)resized->size.y });
                window.setView(sf::View(visibleArea));
                if (hasBg && bgSprite) UpdateBackgroundScale(*bgSprite, resized->size);
            }
        }

        ImGui::SFML::Update(window, deltaClock.restart());
        ImGui::SetNextWindowPos(ImVec2(window.getSize().x / 2.0f, window.getSize().y / 2.0f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

        if (currentState == AppState::Launcher) {
            ImGui::SetNextWindowSize(ImVec2(350, 0));
            ImGui::Begin("Launcher", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

            if (isOfflineMode) {
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), Loc.Get("st_offline"));
                ImGui::Separator();
                ImGui::Spacing();
            }

            ImGui::SetNextItemWidth(-1);
            ImGui::InputTextWithHint("##Nickname", Loc.Get("input_nick_hint"), nicknameBuf, sizeof(nicknameBuf), ImGuiInputTextFlags_CallbackCharFilter, EnglishOnlyFilter);

            ImGui::SetNextItemWidth(-1);
            const char* currentLabel = availableVersions.empty() ? Loc.Get("combo_version_no") : availableVersions[selectedVersion].name.c_str();

            ImGui::BeginDisabled(isDownloading || isExtracting);
            if (ImGui::BeginCombo("##Version", currentLabel)) {
                for (int n = 0; n < (int)availableVersions.size(); n++) {
                    if (ImGui::Selectable(availableVersions[n].name.c_str(), selectedVersion == n)) selectedVersion = n;
                }
                ImGui::EndCombo();
            }
            ImGui::Checkbox(Loc.Get("chk_force_upd"), &forceUpdate);
            ImGui::EndDisabled();

            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

            if (!availableVersions.empty()) {
                std::string targetFolder = "versions/" + availableVersions[selectedVersion].name;
                bool isInstalled = fs::exists(targetFolder);

                if (isDownloading) {
                    float fraction = (dlProgress.total > 0) ? (float)(dlProgress.current / (double)dlProgress.total) : 0.0f;
                    ImGui::ProgressBar(fraction, ImVec2(-1, 45), Loc.Get("st_downloading"));

                    if (dlProgress.finished) {
                        isDownloading = false;
                        isExtracting = true;
                        std::string vName = availableVersions[selectedVersion].name;
                        std::thread([vName]() {
                            std::string cmd = "powershell -Command \"Expand-Archive -Path 'versions/temp.zip' -DestinationPath 'versions/" + vName + "' -Force; Remove-Item 'versions/temp.zip'\"";
                            system(cmd.c_str());
                            isExtracting = false;
                            }).detach();
                    }
                }
                else if (isExtracting) {
                    std::string extractLabel = std::string(Loc.Get("st_extracting")) + "##extracting_stat";
                    ImGui::Button(extractLabel.c_str(), ImVec2(-1, 45));
                }
                else if (!isInstalled || forceUpdate) {
                    if (isOfflineMode) {
                        ImGui::BeginDisabled();
                        ImGui::Button("Мережа недоступна для завантаження", ImVec2(-1, 45));
                        ImGui::EndDisabled();
                    }
                    else {
                        const char* langTag = forceUpdate ? "btn_reinstall" : "btn_download";
                        std::string buttonLabel = std::string(Loc.Get(langTag)) + "##main_action_btn";

                        if (ImGui::Button(buttonLabel.c_str(), ImVec2(-1, 45))) {
                            if (forceUpdate) ImGui::OpenPopup("ConfirmAction");
                            else {
                                isDownloading = true;
                                std::thread(DownloadFile, availableVersions[selectedVersion].url, "versions/temp.zip", &dlProgress).detach();
                            }
                        }

                        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
                        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                        ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.0f, 0.0f, 0.0f, 0.8f));

                        if (ImGui::BeginPopupModal("ConfirmAction", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize)) {
                            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", Loc.Get("warn_title"));
                            ImGui::Separator();
                            ImGui::Spacing();
                            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 20.0f);
                            ImGui::Text("%s", Loc.Get("warn_text"));
                            ImGui::PopTextWrapPos();
                            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

                            if (ImGui::Button(Loc.Get("btn_yes"), ImVec2(120, 35))) {
                                fs::remove_all(targetFolder);
                                isDownloading = true;
                                std::thread(DownloadFile, availableVersions[selectedVersion].url, "versions/temp.zip", &dlProgress).detach();
                                forceUpdate = false;
                                ImGui::CloseCurrentPopup();
                            }
                            ImGui::SameLine();
                            if (ImGui::Button(Loc.Get("btn_no"), ImVec2(120, 35))) ImGui::CloseCurrentPopup();
                            ImGui::EndPopup();
                        }
                        ImGui::PopStyleColor();
                    }
                }
                else {
                    if (ImGui::Button(Loc.Get("btn_launch"), ImVec2(-1, 45))) {
                        cfg.nickname = nicknameBuf;
                        cfg.lastVersion = availableVersions[selectedVersion].name;
                        SaveConfig(cfg);
                        LaunchGame(fs::current_path().string() + "\\" + targetFolder + "\\Minecraft.Client.exe", fs::current_path().string() + "\\" + targetFolder, cfg.nickname, cfg.ip);
                    }
                }
            }
            else {
                ImGui::BeginDisabled();
                ImGui::Button(Loc.Get("st_noinstalledversion"), ImVec2(-1, 45));
                ImGui::EndDisabled();
            }

            ImGui::Spacing();
            float availWidth = ImGui::GetContentRegionAvail().x;
            float spacingX = ImGui::GetStyle().ItemSpacing.x;
            float smallBtnW = (availWidth - spacingX * 2) * 0.25f;
            float bigBtnW = (availWidth - spacingX * 2) * 0.5f;

            ImGui::BeginDisabled(isDownloading || isExtracting);
            if (ImGui::Button(Loc.Get("btn_refresh"), ImVec2(smallBtnW, 35))) FetchManifest();
            ImGui::SameLine();
            if (ImGui::Button(Loc.Get("btn_dir"), ImVec2(smallBtnW, 35))) {
                if (!availableVersions.empty()) {
                    std::string vDir = (fs::current_path() / "versions" / availableVersions[selectedVersion].name).string();
                    if (fs::exists(vDir)) ShellExecuteA(NULL, "open", vDir.c_str(), NULL, NULL, SW_SHOWDEFAULT);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button(Loc.Get("btn_settings"), ImVec2(bigBtnW, 35))) currentState = AppState::Settings;
            ImGui::EndDisabled();

            ImGui::End();
        }
        else if (currentState == AppState::Settings) {
            ImGui::SetNextWindowSize(ImVec2(350, 0));
            ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

            float windowWidth = ImGui::GetWindowSize().x;
            float textWidth = ImGui::CalcTextSize(Loc.Get("title_settings")).x;
            ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
            ImGui::Text("%s", Loc.Get("title_settings"));
            ImGui::Separator(); ImGui::Spacing();

            ImGui::Text("%s", Loc.Get("lbl_lang"));
            ImGui::SetNextItemWidth(-1);
            const char* currentLangName = Loc.availableLanguages.empty() ? "N/A" : Loc.availableLanguages[selectedLanguageIndex].displayName.c_str();

            if (ImGui::BeginCombo("##Lang", currentLangName)) {
                for (int i = 0; i < (int)Loc.availableLanguages.size(); i++) {
                    if (ImGui::Selectable(Loc.availableLanguages[i].displayName.c_str(), selectedLanguageIndex == i)) {
                        selectedLanguageIndex = i;
                        Loc.LoadLanguage(Loc.availableLanguages[selectedLanguageIndex].fileName);
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::Spacing();
            ImGui::Text("%s", Loc.Get("lbl_ip"));
            ImGui::SetNextItemWidth(-1);
            ImGui::InputText("##IP", ipBuf, sizeof(ipBuf));

            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

            if (ImGui::Button(Loc.Get("btn_save"), ImVec2(-1, 40))) {
                cfg.ip = ipBuf;
                if (!Loc.availableLanguages.empty()) cfg.language = Loc.availableLanguages[selectedLanguageIndex].fileName;
                SaveConfig(cfg);
                currentState = AppState::Launcher;
            }
            ImGui::End();
        }

        window.clear();
        if (hasBg && bgSprite) window.draw(*bgSprite);
        else window.clear(sf::Color(20, 20, 20));

        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    if (bgSprite) delete bgSprite;
    return 0;
}