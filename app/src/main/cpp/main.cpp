// Modern Dear ImGui Android shell.
// Upstream base: https://github.com/ocornut/imgui/tree/v1.92.9b/examples/example_android_opengl3

#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <android/log.h>
#include <android_native_app_glue.h>

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace {

constexpr char kLogTag[] = "ModernImGuiSdk";
const ImVec4 kAccent(0.388f, 0.902f, 0.839f, 1.0f);
const ImVec4 kAccentSoft(0.388f, 0.902f, 0.839f, 0.16f);

EGLDisplay gDisplay = EGL_NO_DISPLAY;
EGLSurface gSurface = EGL_NO_SURFACE;
EGLContext gContext = EGL_NO_CONTEXT;
bool gInitialized = false;

enum class Menu {
    Aim = 0,
    Player = 1,
    Visuals = 2,
};

Menu gActiveMenu = Menu::Aim;

// These values are UI-facing debug/configuration state. Connect them to the
// game's own settings adapter when integrating with the runtime.
bool gHipAimAssist = true;
bool gSightAimAssist = true;
bool gAimAssistVertical = false;
bool gRecoilFeedback = true;
bool gCrosshair = true;
float gAimSensitivity = 0.62f;
float gScopeSensitivity = 0.48f;
float gCrosshairSize = 0.56f;
float gCrosshairOpacity = 0.88f;

bool gLocalPlayerInfo = true;
bool gTeammateMarkers = true;
bool gEnemyMarkers = false;
bool gPlayerNames = true;
bool gPlayerDistance = true;
bool gPlayerStatus = true;

bool gEnemyFireHint = true;
bool gEnemyFootstepHint = true;
bool gHitMarker = true;
bool gScreenEffects = true;
bool gOutlinePreview = false;
float gEffectIntensity = 0.72f;
int gOutlineMode = 0;

void LogError(const char* message) {
    __android_log_print(ANDROID_LOG_ERROR, kLogTag, "%s", message);
}

void SetupTheme() {
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
    colors[ImGuiCol_CheckMark] = kAccent;
    colors[ImGuiCol_SliderGrab] = kAccent;
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.52f, 1.0f, 0.92f, 1.0f);
    colors[ImGuiCol_Separator] = ImVec4(0.14f, 0.20f, 0.22f, 0.7f);
    colors[ImGuiCol_SeparatorHovered] = kAccent;
    colors[ImGuiCol_SeparatorActive] = kAccent;
}

void DrawPill(const char* label, const ImVec4& color) {
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(color.x, color.y, color.z, 0.12f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(color.x, color.y, color.z, 0.18f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 20.0f);
    ImGui::SmallButton(label);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);
}

void DrawStatCard(const char* value, const char* label, const ImVec4& accent) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.065f, 0.095f, 0.12f, 1.0f));
    ImGui::BeginChild(label, ImVec2(0.0f, 112.0f), false, ImGuiWindowFlags_NoScrollbar);
    ImGui::PushStyleColor(ImGuiCol_Text, accent);
    ImGui::TextUnformatted(value);
    ImGui::PopStyleColor();
    ImGui::Spacing();
    ImGui::TextDisabled("%s", label);
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void DrawSidebar() {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.045f, 0.07f, 0.09f, 1.0f));
    ImGui::BeginChild("Sidebar", ImVec2(220.0f, 0.0f), false);

    ImGui::SetCursorPosY(28.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, kAccent);
    ImGui::TextUnformatted("CRN");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::TextDisabled("LAB");
    ImGui::TextDisabled("  IN-GAME DEBUG");
    ImGui::Dummy(ImVec2(0.0f, 30.0f));

    const char* labels[] = {"01  Aim", "02  Player", "03  Visuals"};
    for (int i = 0; i < 3; ++i) {
        const bool selected = static_cast<int>(gActiveMenu) == i;
        if (selected) {
            ImGui::PushStyleColor(ImGuiCol_Button, kAccentSoft);
            ImGui::PushStyleColor(ImGuiCol_Text, kAccent);
        }
        if (ImGui::Button(labels[i], ImVec2(-1.0f, 52.0f))) {
            gActiveMenu = static_cast<Menu>(i);
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

void DrawAimMenu() {
    ImGui::TextDisabled("GAMEPLAY / AIM CONFIGURATION");
    ImGui::Text("Aim");
    ImGui::SameLine(ImGui::GetWindowWidth() - 128.0f);
    DrawPill("●  AIM READY", kAccent);
    ImGui::TextDisabled("Tune the game's official aim and crosshair settings.");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Columns(2, "aim_columns", false);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.065f, 0.095f, 0.12f, 1.0f));
    ImGui::BeginChild("AimAssistCard", ImVec2(0.0f, 272.0f), false);
    ImGui::Text("Aim assist");
    ImGui::TextDisabled("Use the game's native aim configuration.");
    ImGui::Spacing();
    ImGui::Checkbox("Hip aim assist", &gHipAimAssist);
    ImGui::Checkbox("Sight aim assist", &gSightAimAssist);
    ImGui::Checkbox("Vertical assist", &gAimAssistVertical);
    ImGui::Checkbox("Recoil feedback", &gRecoilFeedback);
    ImGui::Spacing();
    ImGui::SliderFloat("Aim sensitivity", &gAimSensitivity, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Scope sensitivity", &gScopeSensitivity, 0.0f, 1.0f, "%.2f");
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::NextColumn();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.065f, 0.095f, 0.12f, 1.0f));
    ImGui::BeginChild("CrosshairCard", ImVec2(0.0f, 272.0f), false);
    ImGui::Text("Crosshair");
    ImGui::TextDisabled("Live preview for the in-game reticle.");
    ImGui::Spacing();
    ImGui::Checkbox("Show crosshair", &gCrosshair);
    ImGui::SliderFloat("Crosshair size", &gCrosshairSize, 0.2f, 1.0f, "%.2f");
    ImGui::SliderFloat("Opacity", &gCrosshairOpacity, 0.1f, 1.0f, "%.2f");
    ImGui::Spacing();

    ImVec2 center = ImGui::GetCursorScreenPos() + ImVec2(116.0f, 54.0f);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const float arm = 15.0f * gCrosshairSize;
    const ImU32 crosshairColor = ImGui::GetColorU32(
        ImVec4(kAccent.x, kAccent.y, kAccent.z, gCrosshairOpacity));
    if (gCrosshair) {
        draw->AddLine(center - ImVec2(arm + 8.0f, 0.0f),
                      center - ImVec2(8.0f, 0.0f), crosshairColor, 3.0f);
        draw->AddLine(center + ImVec2(8.0f, 0.0f),
                      center + ImVec2(arm + 8.0f, 0.0f), crosshairColor, 3.0f);
        draw->AddLine(center - ImVec2(0.0f, arm + 8.0f),
                      center - ImVec2(0.0f, 8.0f), crosshairColor, 3.0f);
        draw->AddLine(center + ImVec2(0.0f, 8.0f),
                      center + ImVec2(0.0f, arm + 8.0f), crosshairColor, 3.0f);
        draw->AddCircleFilled(center, 2.5f, crosshairColor);
    }
    ImGui::Dummy(ImVec2(232.0f, 106.0f));
    ImGui::TextDisabled("Crosshair preview / native renderer");
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::Columns(1);
}

void DrawPlayerMenu() {
    ImGui::TextDisabled("GAMEPLAY / PLAYER DEBUG");
    ImGui::SameLine(ImGui::GetWindowWidth() - 130.0f);
    DrawPill("●  LOCAL", kAccent);
    ImGui::Text("Player");
    ImGui::TextDisabled("Inspect local and team presentation in the current session.");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Columns(2, "player_columns", false);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.065f, 0.095f, 0.12f, 1.0f));
    ImGui::BeginChild("PlayerInfoCard", ImVec2(0.0f, 270.0f), false);
    ImGui::Text("Local player");
    ImGui::TextDisabled("COW.GamePlay.Player / ZLH.LocalPlayer");
    ImGui::Spacing();
    ImGui::Checkbox("Local player info", &gLocalPlayerInfo);
    ImGui::Checkbox("Player status", &gPlayerStatus);
    ImGui::Checkbox("Player names", &gPlayerNames);
    ImGui::Checkbox("Distance labels", &gPlayerDistance);
    ImGui::Spacing();
    ImGui::TextDisabled("SESSION");
    ImGui::Text("Team mode     %s", gPlayerStatus ? "ACTIVE" : "HIDDEN");
    ImGui::Text("Camera        %s", gLocalPlayerInfo ? "TRACKING" : "PAUSED");
    ImGui::Text("Player state  %s", gPlayerStatus ? "READY" : "OFF");
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::NextColumn();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.065f, 0.095f, 0.12f, 1.0f));
    ImGui::BeginChild("TeamInfoCard", ImVec2(0.0f, 270.0f), false);
    ImGui::Text("Team presentation");
    ImGui::TextDisabled("Markers follow the game's normal visibility rules.");
    ImGui::Spacing();
    ImGui::Checkbox("Teammate markers", &gTeammateMarkers);
    ImGui::Checkbox("Enemy debug markers", &gEnemyMarkers);
    ImGui::Spacing();
    ImGui::Text("Marker preview");
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.50f, 0.88f, 1.0f, 1.0f));
    ImGui::BulletText("TEAMMATE  •  24 m");
    ImGui::PopStyleColor();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.54f, 0.42f, 1.0f));
    ImGui::BulletText("ENEMY DEBUG  •  38 m");
    ImGui::PopStyleColor();
    ImGui::TextDisabled("Visibility: %s", gEnemyMarkers ? "debug preview" : "native only");
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::Columns(1);
}

void DrawVisualsMenu() {
    ImGui::TextDisabled("GAMEPLAY / EFFECTS & FEEDBACK");
    ImGui::Text("Visuals");
    ImGui::TextDisabled("Configure combat feedback and visual effect previews.");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Columns(2, "visual_columns", false);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.065f, 0.095f, 0.12f, 1.0f));
    ImGui::BeginChild("FeedbackCard", ImVec2(0.0f, 272.0f), false);
    ImGui::Text("Combat feedback");
    ImGui::TextDisabled("Signals already represented by the game's HUD.");
    ImGui::Spacing();
    ImGui::Checkbox("Enemy fire hint", &gEnemyFireHint);
    ImGui::Checkbox("Enemy footstep hint", &gEnemyFootstepHint);
    ImGui::Checkbox("Hit marker", &gHitMarker);
    ImGui::Checkbox("Screen effects", &gScreenEffects);
    ImGui::Spacing();
    ImGui::SliderFloat("Effect intensity", &gEffectIntensity, 0.0f, 1.0f, "%.0f%%");
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::NextColumn();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.065f, 0.095f, 0.12f, 1.0f));
    ImGui::BeginChild("OutlineCard", ImVec2(0.0f, 272.0f), false);
    ImGui::Text("Effect preview");
    ImGui::TextDisabled("Renderer/material preview for development builds.");
    ImGui::Spacing();
    ImGui::Checkbox("Outline preview", &gOutlinePreview);
    const char* outlineModes[] = {"Native", "Fresnel", "Silhouette"};
    ImGui::Combo("Outline mode", &gOutlineMode, outlineModes, IM_ARRAYSIZE(outlineModes));
    ImGui::Spacing();
    ImGui::Text("Runtime");
    ImGui::Text("Renderer       OpenGL ES 3");
    ImGui::Text("Effect state   %s", gScreenEffects ? "ACTIVE" : "DISABLED");
    ImGui::Text("Feedback       %s", gHitMarker ? "ENABLED" : "DISABLED");
    ImGui::Spacing();
    DrawPill(gOutlinePreview ? "●  PREVIEW ON" : "○  PREVIEW OFF",
             gOutlinePreview ? kAccent : ImVec4(0.47f, 0.53f, 0.56f, 1.0f));
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::Columns(1);
}

void DrawUi() {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::Begin("Modern SDK", nullptr,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus);

    DrawSidebar();
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::BeginChild("Content", ImVec2(0.0f, 0.0f), false);
    if (gActiveMenu == Menu::Aim) {
        DrawAimMenu();
    } else if (gActiveMenu == Menu::Player) {
        DrawPlayerMenu();
    } else {
        DrawVisualsMenu();
    }
    ImGui::EndChild();
    ImGui::End();
}

void InitEgl(android_app* app) {
    gDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (gDisplay == EGL_NO_DISPLAY || eglInitialize(gDisplay, nullptr, nullptr) != EGL_TRUE) {
        LogError("Unable to initialize EGL");
        return;
    }

    const EGLint configAttributes[] = {
        EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8,
        EGL_DEPTH_SIZE, 24, EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_NONE
    };
    EGLConfig config = nullptr;
    EGLint numConfigs = 0;
    eglChooseConfig(gDisplay, configAttributes, &config, 1, &numConfigs);
    if (numConfigs == 0) {
        LogError("No compatible EGL configuration");
        return;
    }

    const EGLint contextAttributes[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    gContext = eglCreateContext(gDisplay, config, EGL_NO_CONTEXT, contextAttributes);
    gSurface = eglCreateWindowSurface(gDisplay, config, app->window, nullptr);
    if (gContext == EGL_NO_CONTEXT || gSurface == EGL_NO_SURFACE ||
        eglMakeCurrent(gDisplay, gSurface, gSurface, gContext) != EGL_TRUE) {
        LogError("Unable to create the OpenGL ES context");
        return;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    SetupTheme();
    ImGui_ImplAndroid_Init(app->window);
    ImGui_ImplOpenGL3_Init("#version 300 es");
    gInitialized = true;
}

void ShutdownEgl() {
    if (!gInitialized) {
        return;
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplAndroid_Shutdown();
    ImGui::DestroyContext();

    if (gDisplay != EGL_NO_DISPLAY) {
        eglMakeCurrent(gDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (gContext != EGL_NO_CONTEXT) {
            eglDestroyContext(gDisplay, gContext);
        }
        if (gSurface != EGL_NO_SURFACE) {
            eglDestroySurface(gDisplay, gSurface);
        }
        eglTerminate(gDisplay);
    }
    gDisplay = EGL_NO_DISPLAY;
    gSurface = EGL_NO_SURFACE;
    gContext = EGL_NO_CONTEXT;
    gInitialized = false;
}

void HandleAppCommand(android_app* app, int32_t command) {
    switch (command) {
        case APP_CMD_INIT_WINDOW:
            if (app->window != nullptr) {
                InitEgl(app);
            }
            break;
        case APP_CMD_TERM_WINDOW:
            ShutdownEgl();
            break;
        default:
            break;
    }
}

int32_t HandleInputEvent(android_app*, AInputEvent* event) {
    return ImGui_ImplAndroid_HandleInputEvent(event);
}

void RenderFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame();
    ImGui::NewFrame();
    DrawUi();
    ImGui::Render();

    const ImVec4 clearColor(0.035f, 0.055f, 0.075f, 1.0f);
    glViewport(0, 0, static_cast<int>(ImGui::GetIO().DisplaySize.x),
               static_cast<int>(ImGui::GetIO().DisplaySize.y));
    glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    eglSwapBuffers(gDisplay, gSurface);
}

}  // namespace

void android_main(android_app* app) {
    app->onAppCmd = HandleAppCommand;
    app->onInputEvent = HandleInputEvent;

    while (true) {
        int events = 0;
        android_poll_source* source = nullptr;
        while (ALooper_pollOnce(gInitialized ? 0 : -1, nullptr, &events,
                                reinterpret_cast<void**>(&source)) >= 0) {
            if (source != nullptr) {
                source->process(app, source);
            }
            if (app->destroyRequested != 0) {
                ShutdownEgl();
                return;
            }
        }
        if (gInitialized) {
            RenderFrame();
        }
    }
}