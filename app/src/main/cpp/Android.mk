LOCAL_PATH := $(call my-dir)
MAIN_LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE            := libdobby
LOCAL_SRC_FILES         := TuanMeta/Tools/Dobby/libraries/$(TARGET_ARCH_ABI)/libdobby.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/TuanMeta/Tools/Dobby/
include $(PREBUILT_STATIC_LIBRARY)
# ============================================================================#

include $(CLEAR_VARS)

LOCAL_MODULE           := Sdk

LOCAL_CFLAGS           := -Wno-error=format-security -fvisibility=hidden -ffunction-sections -fdata-sections -w
LOCAL_CFLAGS           += -fno-rtti -fno-exceptions -fpermissive
LOCAL_CPPFLAGS         := -Wno-error=format-security -fvisibility=hidden -ffunction-sections -fdata-sections -w -Werror -s -std=c++17
LOCAL_CPPFLAGS         += -Wno-error=c++11-narrowing -fms-extensions -fno-rtti -fno-exceptions -fpermissive
LOCAL_LDFLAGS          += -Wl,--gc-sections,--strip-all, -llog
LOCAL_ARM_MODE         := arm
LOCAL_LDLIBS           := -llog -landroid -lEGL -lGLESv3 -lGLESv2 -lGLESv1_CM -lz

LOCAL_C_INCLUDES       += $(LOCAL_PATH)
LOCAL_C_INCLUDES       += $(LOCAL_PATH)/ImGui
LOCAL_C_INCLUDES       += $(LOCAL_PATH)/ImGui/backends
LOCAL_C_INCLUDES       += $(LOCAL_PATH)/TuanMeta
LOCAL_C_INCLUDES       += $(LOCAL_PATH)/TuanMeta/Tools/curl/openssl-android-$(TARGET_ARCH_ABI)/include
LOCAL_C_INCLUDES       += $(NDK_ROOT)/sources/android/native_app_glue

LOCAL_SRC_FILES := Main.cpp \
    $(NDK_ROOT)/sources/android/native_app_glue/android_native_app_glue.c \
    ImGui/imgui.cpp \
    ImGui/imgui_draw.cpp \
    ImGui/imgui_tables.cpp \
    ImGui/imgui_widgets.cpp \
    ImGui/imgui_stdlib.cpp \
    ImGui/backends/imgui_impl_opengl3.cpp \
    ImGui/backends/imgui_impl_android.cpp \
	Substrate/hde64.c \
	Substrate/SubstrateDebug.cpp \
	Substrate/SubstrateHook.cpp \
	Substrate/SubstratePosixMemory.cpp \
	Substrate/SymbolFinder.cpp \
    KittyMemory/KittyMemory.cpp \
    KittyMemory/MemoryPatch.cpp \
    KittyMemory/MemoryBackup.cpp \
    KittyMemory/KittyUtils.cpp \
    KittyMemory/KittyScanner.cpp \
    KittyMemory/KittyArm64.cpp \
    And64InlineHook/And64InlineHook.cpp \
    TuanMeta/IL2CppSDKGenerator/Il2Cpp.cpp \
    TuanMeta/Tools/MonoString.cpp \
    TuanMeta/Tools/Tools.cpp \
    TuanMeta/Dump/Il2Cpp/il2cpp_dump.cpp \
     
LOCAL_STATIC_LIBRARIES := libdobby
# The ZIP source is built as the library loaded by NativeActivity.

include $(BUILD_SHARED_LIBRARY)
# ============================================================================

