#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <android_native_app_glue.h>
#include <KittyMemory/KittyMemory.h>
#include <KittyMemory/MemoryPatch.h>
#include <KittyMemory/KittyScanner.h>
#include <KittyMemory/KittyUtils.h>
#include "Includes/obfuscate.h"
#include "Includes/Logger.h"
#include "Includes/Macros.h"
#include "Includes/JNIStuff.h"
#include "Includes/Utils.h"
#include "TuanMeta/Call_Me.h"
#include "Hook.h"
#include "ImGui/Toggle.h"
#include "ImGui/Comic_Sans.h"
#include "monster.h"
#include "style.h"
#include "OpenGL.h"
#include "ProjectMenu.h"

//=========!!!==========

static bool chams = false; 
static bool shading = false; 
static bool wireframe = false;
static bool glow = false;
static bool outline = false;
static bool rainbow = false;
int as = 0;

uintptr_t address = 0;
uintptr_t touuu;
uintptr_t tto;
struct UnityEngine_Vector2_Fields {
    float x;
    float y;
};

struct UnityEngine_Vector2_o {
    UnityEngine_Vector2_Fields fields;
};

enum TouchPhase {
    Began = 0,
    Moved = 1,
    Stationary = 2,
    Ended = 3,
    Canceled = 4
};

struct UnityEngine_Touch_Fields {
    int32_t m_FingerId;
    struct UnityEngine_Vector2_o m_Position;
    struct UnityEngine_Vector2_o m_RawPosition;
    struct UnityEngine_Vector2_o m_PositionDelta;
    float m_TimeDelta;
    int32_t m_TapCount;
    int32_t m_Phase;
    int32_t m_Type;
    float m_Pressure;
    float m_maximumPossiblePressure;
    float m_Radius;
    float m_RadiusVariance;
    float m_AltitudeAngle;
    float m_AzimuthAngle;
};


// The original ZIP menu is replaced by the project menu adapter.
void DrawMenu()
{
    ProjectMenu::Draw();
}

//=========!!!==========

static bool setup = false;
EGLBoolean (*orig_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
EGLBoolean _eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
	
	eglQuerySurface(dpy, surface, EGL_WIDTH, &glWidth);
	eglQuerySurface(dpy, surface, EGL_HEIGHT, &glHeight);
    
	if (glWidth <= 0 || glHeight <= 0) {
        return orig_eglSwapBuffers != nullptr
                   ? orig_eglSwapBuffers(dpy, surface)
                   : EGL_FALSE;
	}

    if (!setup) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ProjectMenu::SetupTheme();
    io.Fonts->AddFontFromMemoryTTF(const_cast<std::uint8_t*>(comic_sans), sizeof(comic_sans), 25.f, NULL, io.Fonts->GetGlyphRangesVietnamese());
    ImGui_ImplOpenGL3_Init("#version 300 es");
    setup = true;
	}
    ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplAndroid_NewFrame(glWidth, glHeight);
    ImGui::NewFrame();
	DrawMenu(); // Calling Menu |
    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    return orig_eglSwapBuffers != nullptr
               ? orig_eglSwapBuffers(dpy, surface)
               : EGL_FALSE;
    
    


}

void HandleTouchInput() {
    ImGuiIO &io = ImGui::GetIO();
    static bool clearMousePos = true;

    int touchCount = (((int (*)())(address + touuu))()); // public static int get_touchCount() { }
    if (touchCount > 0) {
        UnityEngine_Touch_Fields touch = ((UnityEngine_Touch_Fields(*)(int))(address + tto))(0); // public static Touch GetTouch(int index) { }
        float reverseY = io.DisplaySize.y - touch.m_Position.fields.y;
        switch (touch.m_Phase) {
            case TouchPhase::Began:
            case TouchPhase::Stationary:
                io.MousePos = ImVec2(touch.m_Position.fields.x, reverseY);
                io.MouseDown[0] = true;
                break;
            case TouchPhase::Ended:
            case TouchPhase::Canceled:
                io.MouseDown[0] = false;
                clearMousePos = true;
                break;
            case TouchPhase::Moved:
                io.MousePos = ImVec2(touch.m_Position.fields.x, reverseY);
                break;
            default:
                break;
        }
    }

    if (clearMousePos) {
        io.MousePos = ImVec2(-1, -1);
        clearMousePos = false;
    }
}
uintptr_t il2cppMap;
ProcMap anogsMap, il2cppMap2;

void *Init_Thread(void *) {
    LOGI("ZIP runtime initialized; game feature adapters are disabled until target offsets are configured.");
    return nullptr;
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *, void *) {
    LOGI("Initialize JNI");
    return JNI_VERSION_1_6;
}

void android_main(struct android_app *app) {
    app_dummy();
    while (app != nullptr && !app->destroyRequested) {
        int events = 0;
        struct android_poll_source *source = nullptr;
        while (ALooper_pollOnce(0, nullptr, &events,
                                reinterpret_cast<void **>(&source)) >= 0) {
            if (source != nullptr) {
                source->process(app, source);
            }
            if (app->destroyRequested) {
                return;
            }
        }
        usleep(16000);
    }
}

#if defined(__aarch64__)
#define SYSTEM_LIB_ROOT "/system/lib64/"
#else
#define SYSTEM_LIB_ROOT "/system/lib/"
#endif

void InstallSymbolHook(const char *library, const char *symbol,
                       void *replacement, void **original) {
    void *target = DobbySymbolResolver(library, symbol);
    if (target == nullptr) {
        LOGW("Could not resolve %s from %s", symbol, library);
        return;
    }
    Tools::Hook(target, replacement, original);
}

__attribute__((constructor))
void lib_main()
{
    InstallSymbolHook(SYSTEM_LIB_ROOT "libandroid.so",
                      OBFUSCATE("ANativeWindow_getWidth"),
                      reinterpret_cast<void *>(_ANativeWindow_getWidth),
                      reinterpret_cast<void **>(&orig_ANativeWindow_getWidth));
    InstallSymbolHook(SYSTEM_LIB_ROOT "libandroid.so",
                      OBFUSCATE("ANativeWindow_getHeight"),
                      reinterpret_cast<void *>(_ANativeWindow_getHeight),
                      reinterpret_cast<void **>(&orig_ANativeWindow_getHeight));
    InstallSymbolHook(SYSTEM_LIB_ROOT "libinput.so",
                      OBFUSCATE("_ZN7android13InputConsumer21initializeMotionEventEPNS_11MotionEventEPKNS_12InputMessageE"),
                      reinterpret_cast<void *>(myInput),
                      reinterpret_cast<void **>(&origInput));
    InstallSymbolHook(SYSTEM_LIB_ROOT "libEGL.so",
                      OBFUSCATE("eglSwapBuffers"),
                      reinterpret_cast<void *>(_eglSwapBuffers),
                      reinterpret_cast<void **>(&orig_eglSwapBuffers));
	pthread_t myThread;
	pthread_create(&myThread, NULL, Init_Thread, NULL);
}
