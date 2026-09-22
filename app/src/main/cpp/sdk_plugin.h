#pragma once

#include <android/native_window.h>

#ifdef __cplusplus
extern "C" {
#endif

// The host must call these on its OpenGL ES 3 render thread. Sdk does not
// create an EGL context or call eglSwapBuffers in plugin mode.
bool Sdk_InitializeOnCurrentContext(ANativeWindow* window);
void Sdk_RenderOnCurrentContext();
void Sdk_SetDisplaySize(int width, int height);
void Sdk_ShutdownOnCurrentContext();

#ifdef __cplusplus
}
#endif