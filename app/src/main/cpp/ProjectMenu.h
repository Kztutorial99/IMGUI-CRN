#pragma once

#include "ImGui/imgui.h"

namespace ProjectMenu {

inline bool hipAimAssist = true;
inline bool sightAimAssist = true;
inline bool verticalAssist = false;
inline bool recoilFeedback = true;
inline bool crosshair = true;
inline float aimSensitivity = 0.62f;
inline float scopeSensitivity = 0.48f;
inline float crosshairSize = 0.56f;
inline float crosshairOpacity = 0.88f;

inline bool localPlayerInfo = true;
inline bool teammateMarkers = true;
inline bool enemyMarkers = false;
inline bool playerNames = true;
inline bool playerDistance = true;
inline bool playerStatus = true;

inline bool enemyFireHint = true;
inline bool enemyFootstepHint = true;
inline bool hitMarker = true;
inline bool screenEffects = true;
inline bool outlinePreview = false;
inline float effectIntensity = 0.72f;
inline int outlineMode = 0;
inline int activeTab = 0;

inline void SetupTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(22.0f, 22.0f);
    style.FramePadding = ImVec2(14.0f, 11.0f);
    style.ItemSpacing = ImVec2(12.0f, 12.0f);
    style.ItemInnerSpacing = ImVec2(8.0f, 8.0f);
    style.TouchExtraPadding = ImVec2(4.0f, 4.0f);
    style.ScrollbarSize = 14.0f;
    style.WindowRounding = 0.0f;
    style.ChildRounding = 14.0f;
    style.FrameRounding = 10.0f;
    style.PopupRounding = 12.0f;
    style.GrabRounding = 10.0f;
    style.TabRounding = 10.0f;
    style.WindowBorderSize = 0.0f;
    style.ChildBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text] = ImVec4(0.90f, 0.94f, 0.95f, 1.0f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.47f, 0.53f, 0.56f, 1.0f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.035f, 0.055f, 0.075f, 1.0f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.055f, 0.080f, 0.105f, 1.0f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.070f, 0.095f, 0.120f, 1.0f);
    colors[ImGuiCol_Border] = ImVec4(0.14f, 0.20f, 0.22f, 0.65f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.09f, 0.125f, 0.15f, 1.0f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.13f, 0.19f, 0.20f, 1.0f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.16f, 0.24f, 0.24f, 1.0f);
    colors[ImGuiCol_TitleBg] = colors[ImGuiCol_WindowBg];
    colors[ImGuiCol_Button] = ImVec4(0.08f, 0.13f, 0.15f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.12f, 0.22f, 0.22f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.17f, 0.31f, 0.30f, 1.0f);
    colors[ImGuiCol_Header] = ImVec4(0.08f, 0.14f, 0.16f, 1.0f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.12f, 0.22f, 0.22f, 1.0f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.17f, 0.31f, 0.30f, 1.0f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.388f, 0.902f, 0.839f, 1.0f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.388f, 0.902f, 0.839f, 1.0f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.52f, 1.0f, 0.92f, 1.0f);
}

inline void Pill(const char* text) {
    const ImVec4 accent(0.388f, 0.902f, 0.839f, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, accent);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(accent.x, accent.y, accent.z, 0.12f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(accent.x, accent.y, accent.z, 0.18f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 20.0f);
    ImGui::SmallButton(text);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);
}

inline void Sidebar() {
    const ImVec4 accent(0.388f, 0.902f, 0.839f, 1.0f);
    const ImVec4 accentSoft(0.388f, 0.902f, 0.839f, 0.16f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.045f, 0.07f, 0.09f, 1.0f));
    ImGui::BeginChild("Sidebar", ImVec2(220.0f, 0.0f), false);
    ImGui::SetCursorPosY(28.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, accent);
    ImGui::TextUnformatted("CRN");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::TextDisabled("LAB");
    ImGui::TextDisabled("  IN-GAME DEBUG");
    ImGui::Dummy(ImVec2(0.0f, 30.0f));

    const char* labels[] = {"01  Aim", "02  Player", "03  Visuals"};
    for (int i = 0; i < 3; ++i) {
        const bool selected = activeTab == i;
        if (selected) {
            ImGui::PushStyleColor(ImGuiCol_Button, accentSoft);
            ImGui::PushStyleColor(ImGuiCol_Text, accent);
        }
        if (ImGui::Button(labels[i], ImVec2(-1.0f, 52.0f))) {
            activeTab = i;
        }
        if (selected) {
            ImGui::PopStyleColor(2);
        }
        ImGui::Spacing();
    }

    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 82.0f);
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextDisabled("BUILD");
    ImGui::Text("v1.0.0  •  LIBSDK");
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

inline void AimTab() {
    ImGui::TextDisabled("GAMEPLAY / AIM CONFIGURATION");
    ImGui::Text("Aim");
    ImGui::SameLine(ImGui::GetWindowWidth() - 128.0f);
    Pill("AIM READY");
    ImGui::TextDisabled("Tune the game's native aim and crosshair settings.");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Columns(2, "aim_columns", false);
    ImGui::Text("Aim assist");
    ImGui::TextDisabled("Use the game's native aim configuration.");
    ImGui::Spacing();
    ImGui::Checkbox("Hip aim assist", &hipAimAssist);
    ImGui::Checkbox("Sight aim assist", &sightAimAssist);
    ImGui::Checkbox("Vertical assist", &verticalAssist);
    ImGui::Checkbox("Recoil feedback", &recoilFeedback);
    ImGui::Spacing();
    ImGui::SliderFloat("Aim sensitivity", &aimSensitivity, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Scope sensitivity", &scopeSensitivity, 0.0f, 1.0f, "%.2f");

    ImGui::NextColumn();
    ImGui::Text("Crosshair");
    ImGui::TextDisabled("Live preview for the in-game reticle.");
    ImGui::Spacing();
    ImGui::Checkbox("Show crosshair", &crosshair);
    ImGui::SliderFloat("Crosshair size", &crosshairSize, 0.2f, 1.0f, "%.2f");
    ImGui::SliderFloat("Opacity", &crosshairOpacity, 0.1f, 1.0f, "%.2f");
    ImGui::Columns(1);
}

inline void PlayerTab() {
    ImGui::TextDisabled("PLAYER / DEBUG OVERLAY");
    ImGui::Text("Player");
    ImGui::TextDisabled("Connect these switches to the game's own settings adapter.");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Checkbox("Local player info", &localPlayerInfo);
    ImGui::Checkbox("Teammate markers", &teammateMarkers);
    ImGui::Checkbox("Enemy debug markers", &enemyMarkers);
    ImGui::Checkbox("Player names", &playerNames);
    ImGui::Checkbox("Distance", &playerDistance);
    ImGui::Checkbox("Status", &playerStatus);
}

inline void VisualsTab() {
    ImGui::TextDisabled("VISUALS / FEEDBACK");
    ImGui::Text("Visuals");
    ImGui::TextDisabled("Runtime hooks and offsets are intentionally kept outside the menu.");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Checkbox("Enemy fire hint", &enemyFireHint);
    ImGui::Checkbox("Enemy footstep hint", &enemyFootstepHint);
    ImGui::Checkbox("Hit marker", &hitMarker);
    ImGui::Checkbox("Screen effects", &screenEffects);
    ImGui::Checkbox("Outline preview", &outlinePreview);
    ImGui::SliderFloat("Effect intensity", &effectIntensity, 0.0f, 1.0f, "%.2f");
    const char* modes[] = {"Soft", "Strong", "Disabled"};
    ImGui::Combo("Outline mode", &outlineMode, modes, 3);
}

inline void Draw() {
    ImGui::SetNextWindowSize(ImVec2(820.0f, 560.0f), ImGuiCond_Once);
    ImGui::SetNextWindowBgAlpha(1.0f);
    if (ImGui::Begin("CRN LAB", nullptr, ImGuiWindowFlags_NoCollapse)) {
        Sidebar();
        ImGui::SameLine();
        ImGui::BeginChild("Content", ImVec2(0.0f, 0.0f), false);
        if (activeTab == 0) {
            AimTab();
        } else if (activeTab == 1) {
            PlayerTab();
        } else {
            VisualsTab();
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

}  // namespace ProjectMenu