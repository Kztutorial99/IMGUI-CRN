---
name: Android EGL ImGui hook
description: Durable constraints for rendering Dear ImGui from a host Android EGL context.
---

The overlay render path must account for direct PLT imports, eglGetProcAddress lookups, and both eglSwapBuffersWithDamageKHR/EXT variants; an "installed" PLT hook alone does not prove that a frame callback is reached.

**Why:** Modern Android/Unity renderers may use damage-buffer swap functions or resolve them dynamically, so a hook that only patches eglSwapBuffers can silently render nothing.

**How to apply:** Treat the runtime log sequence as the acceptance check: library loaded, swap/proc import patched, hook called, EGL surface dimensions valid, ImGui initialized, and first frame rendered.

NativeActivity glue should be compiled directly into the shared library (or force-linked whole-archive) so ANativeActivity_onCreate is exported when the library is loaded by Android's NativeActivity.

**Why:** A normal static-library link can discard the glue object because no regular object references ANativeActivity_onCreate, producing an UnsatisfiedLinkError before rendering starts.

**How to apply:** Inspect the final libSdk.so exports/build wiring whenever NativeActivity is part of the packaging path.