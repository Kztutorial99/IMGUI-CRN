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
android_app* gApp = nullptr;
bool gInitialized = false;
int gActiveMenu = 0;
float gProgress = 0.68f;
bool gLivePreview = true;
bool gNotifications = true;
bool gCompactMode = false;
bool gAutoRefresh = true;

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
    ImGui::TextUnformatted("SDK");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::TextDisabled("CONTROL");
    ImGui::TextDisabled("  ANDROID / NATIVE");
    ImGui::Dummy(ImVec2(0.0f, 30.0f));

    const char* labels[] = {"01  Overview", "02  Controls", "03  Settings"};
    for (int i = 0; i < 3; ++i) {
        const bool selected = gActiveMenu == i;
        if (selected) {
            ImGui::PushStyleColor(ImGuiCol_Button, kAccentSoft);
            ImGui::PushStyleColor(ImGuiCol_Text, kAccent);
        }
        if (ImGui::Button(labels[i], ImVec2(-1.0f, 52.0f))) {
            gActiveMenu = i;
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
    ImGui::Text("v1.0.0  •  ARM64");
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void DrawOverview() {
    ImGui::TextDisabled("WEDNESDAY, 23 SEPTEMBER 2026");
    ImGui::SameLine(ImGui::GetWindowWidth() - 130.0f);
    DrawPill("●  ONLINE", kAccent);
    ImGui::Spacing();
    ImGui::Text("Good evening.");
    ImGui::TextDisabled("Your native interface is ready for action.");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Columns(3, "stats", false);
    DrawStatCard("68%", "SYSTEM LOAD", kAccent);
    ImGui::NextColumn();
    DrawStatCard("1.2 ms", "FRAME TIME", ImVec4(0.62f, 0.72f, 1.0f, 1.0f));
    ImGui::NextColumn();
    DrawStatCard("Stable", "CONNECTION", ImVec4(0.70f, 0.90f, 0.62f, 1.0f));
    ImGui::Columns(1);

    ImGui::Spacing();
    ImGui::Text("Activity");
    ImGui::SameLine(ImGui::GetWindowWidth() - 150.0f);
    ImGui::TextDisabled("LIVE TELEMETRY");
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.065f, 0.095f, 0.12f, 1.0f));
    ImGui::BeginChild("Activity", ImVec2(0.0f, 180.0f), false);
    const float width = ImGui::GetContentRegionAvail().x;
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const float height = 112.0f;
    for (int i = 0; i < 31; ++i) {
        const float wave = std::sin(static_cast<float>(i) * 0.55f) * 0.22f;
        const float second = std::cos(static_cast<float>(i) * 0.22f) * 0.12f;
        const float y = origin.y + height * (0.52f + wave + second);
        const float x = origin.x + width * static_cast<float>(i) / 30.0f;
        if (i > 0) {
            const float previous = origin.y + height *
                (0.52f + std::sin(static_cast<float>(i - 1) * 0.55f) * 0.22f +
                 std::cos(static_cast<float>(i - 1) * 0.22f) * 0.12f);
            const float previousX = origin.x + width * static_cast<float>(i - 1) / 30.0f;
            draw->AddLine(ImVec2(previousX, previous), ImVec2(x, y), kAccent, 3.0f);
        }
    }
    ImGui::Dummy(ImVec2(width, height + 18.0f));
    ImGui::TextDisabled("Last 30 frames");
    ImGui::SameLine();
    ImGui::Text("  smooth / 60 FPS");
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void DrawControls() {
    ImGui::TextDisabled("RUNTIME CONFIGURATION");
    ImGui::Text("Controls");
    ImGui::TextDisabled("Tune the live preview without leaving the app.");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.065f, 0.095f, 0.12f, 1.0f));
    ImGui::BeginChild("ControlPanel", ImVec2(0.0f, 0.0f), false);
    ImGui::Text("Render pipeline");
    ImGui::TextDisabled("OpenGL ES 3 / NativeActivity");
    ImGui::Spacing();
    ImGui::SliderFloat("Preview intensity", &gProgress, 0.0f, 1.0f, "%.0f%%");
    ImGui::Checkbox("Live preview", &gLivePreview);
    ImGui::Checkbox("Compact mode", &gCompactMode);
    ImGui::Spacing();
    ImGui::Text("Actions");
    ImGui::Spacing();
    if (ImGui::Button("APPLY CHANGES", ImVec2(190.0f, 48.0f))) {
        gLivePreview = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("RESET", ImVec2(120.0f, 48.0f))) {
        gProgress = 0.68f;
        gLivePreview = true;
        gCompactMode = false;
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void DrawSettings() {
    ImGui::TextDisabled("PREFERENCES");
    ImGui::Text("Settings");
    ImGui::TextDisabled("Personalize the SDK shell for your workflow.");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.065f, 0.095f, 0.12f, 1.0f));
    ImGui::BeginChild("SettingsPanel", ImVec2(0.0f, 0.0f), false);
    ImGui::Text("General");
    ImGui::Spacing();
    ImGui::Checkbox("Enable notifications", &gNotifications);
    ImGui::Checkbox("Auto refresh data", &gAutoRefresh);
    ImGui::Spacing();
    ImGui::Text("About this build");
    ImGui::TextDisabled("Dear ImGui v1.92.9b");
    ImGui::TextDisabled("Android NativeActivity + OpenGL ES 3");
    ImGui::TextDisabled("Output: libSdk.so");
    ImGui::EndChild();
    ImGui::PopStyleColor();
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
    if (gActiveMenu == 0) {
        DrawOverview();
    } else if (gActiveMenu == 1) {
        DrawControls();
    } else {
        DrawSettings();
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
    gApp = app;
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