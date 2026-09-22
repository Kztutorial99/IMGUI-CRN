cmake_minimum_required(VERSION 3.22.1)
project("modern_imgui_sdk")

include(FetchContent)

# Official Dear ImGui release verified from ocornut/imgui.
# The Android and OpenGL ES 3 backends are kept in the upstream tree.
set(IMGUI_VERSION v1.92.9b CACHE STRING "Dear ImGui release")
FetchContent_Declare(
    imgui
    URL https://github.com/ocornut/imgui/archive/refs/tags/${IMGUI_VERSION}.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
# Dear ImGui is intentionally source-only and does not ship a root
# CMakeLists.txt. Populate it without trying to add it as a subdirectory.
FetchContent_Populate(imgui)

set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_android.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_compile_definitions(imgui PRIVATE IMGUI_IMPL_OPENGL_ES3)

set(NATIVE_APP_GLUE
    ${CMAKE_ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
)
add_library(native_app_glue STATIC ${NATIVE_APP_GLUE})
target_include_directories(native_app_glue PUBLIC
    ${CMAKE_ANDROID_NDK}/sources/android/native_app_glue
)

add_library(Sdk SHARED main.cpp)
set_target_properties(Sdk PROPERTIES OUTPUT_NAME "Sdk")

target_include_directories(Sdk PRIVATE
    ${CMAKE_ANDROID_NDK}/sources/android/native_app_glue
)

target_link_libraries(Sdk
    imgui
    native_app_glue
    android
    EGL
    GLESv3
    log
)

target_compile_options(Sdk PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
)