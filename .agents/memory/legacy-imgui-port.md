---
name: Legacy ImGui source port
description: How to use the uploaded legacy Android ImGui source without regressing the current renderer.
---

Use the uploaded legacy ImGui source selectively: keep the current host-context renderer and EGL hook, and port only UI or input adapter concepts that match the current ImGui/backend APIs.

**Why:** The legacy source uses Dear ImGui 1.89.3, a custom Android backend, proxy-library JNI loading, and a single `eglSwapBuffers` hook. Replacing the current path with it would drop newer swap paths and reintroduce ABI/API coupling.

**How to apply:** Keep game-specific settings behind the current runtime adapter. Never treat the legacy source's offsets, class names, prebuilt binaries, or memory-patching helpers as portable project code.