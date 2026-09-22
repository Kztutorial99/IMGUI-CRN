cmake_minimum_required(VERSION 3.22.1)
project("modern_imgui_sdk")

# Official Dear ImGui release verified from ocornut/imgui.
# The Android and OpenGL ES 3 backends are kept in the upstream tree.
set(IMGUI_VERSION v1.92.9b CACHE STRING "Dear ImGui release")
string(REGEX REPLACE "^v" "" IMGUI_FOLDER "${IMGUI_VERSION}")
set(IMGUI_DEPS_DIR "${CMAKE_BINARY_DIR}/third_party")
set(imgui_SOURCE_DIR "${IMGUI_DEPS_DIR}/imgui-${IMGUI_FOLDER}")
set(IMGUI_ARCHIVE "${IMGUI_DEPS_DIR}/imgui-${IMGUI_FOLDER}.tar.gz")
set(IMGUI_URL "https://github.com/ocornut/imgui/archive/refs/tags/${IMGUI_VERSION}.tar.gz")

# Dear ImGui is source-only and does not ship a root CMakeLists.txt. Download
# and extract it explicitly instead of using FetchContent's sub-build, which
# is not reliable with the Android CMake 3.22 generator.
if(NOT EXISTS "${imgui_SOURCE_DIR}/imgui.cpp")
    file(MAKE_DIRECTORY "${IMGUI_DEPS_DIR}")
    file(DOWNLOAD "${IMGUI_URL}" "${IMGUI_ARCHIVE}"
        SHOW_PROGRESS
        TLS_VERIFY ON
        STATUS IMGUI_DOWNLOAD_STATUS
    )
    list(GET IMGUI_DOWNLOAD_STATUS 0 IMGUI_DOWNLOAD_CODE)
    list(GET IMGUI_DOWNLOAD_STATUS 1 IMGUI_DOWNLOAD_MESSAGE)
    if(NOT IMGUI_DOWNLOAD_CODE EQUAL 0)
        message(FATAL_ERROR "Failed to download Dear ImGui: ${IMGUI_DOWNLOAD_MESSAGE}")
    endif()

    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E tar xzf "${IMGUI_ARCHIVE}"
        WORKING_DIRECTORY "${IMGUI_DEPS_DIR}"
        RESULT_VARIABLE IMGUI_EXTRACT_RESULT
    )
    if(NOT IMGUI_EXTRACT_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to extract Dear ImGui archive")
    endif()
endif()

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

add_library(Sdk SHARED
    main.cpp
    egl_hook.cpp
    ${NATIVE_APP_GLUE}
)
set_target_properties(Sdk PROPERTIES OUTPUT_NAME "Sdk")

target_include_directories(Sdk PRIVATE
    ${CMAKE_ANDROID_NDK}/sources/android/native_app_glue
)

target_link_libraries(Sdk
    imgui
    android
    EGL
    GLESv3
    dl
    log
)

target_compile_options(Sdk PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
)