// Заміни вміст Theme.h на цей:
#pragma once
#include <imgui.h>

inline void SetupModernTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    style.WindowRounding = 0.0f;
    style.FrameRounding = 3.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.WindowPadding = ImVec2(25.0f, 25.0f);
    style.ItemSpacing = ImVec2(8.0f, 8.0f);

    colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.12f, 0.45f);
    colors[ImGuiCol_Border] = ImVec4(1.0f, 1.0f, 1.0f, 0.15f); 

    colors[ImGuiCol_FrameBg] = ImVec4(0.2f, 0.2f, 0.2f, 0.5f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.3f, 0.3f, 0.3f, 0.6f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.4f, 0.4f, 0.4f, 0.7f);

    colors[ImGuiCol_Button] = ImVec4(0.25f, 0.25f, 0.25f, 0.6f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.35f, 0.35f, 0.7f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.45f, 0.45f, 0.45f, 0.8f);

    colors[ImGuiCol_Text] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
    colors[ImGuiCol_Separator] = ImVec4(1.0f, 1.0f, 1.0f, 0.2f);

    style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.7f);
    colors[ImGuiCol_PopupBg] = colors[ImGuiCol_WindowBg];
    colors[ImGuiCol_Border] = ImVec4(1.0f, 1.0f, 1.0f, 0.15f);
}