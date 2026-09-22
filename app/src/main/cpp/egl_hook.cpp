#include "sdk_plugin.h"

#include <EGL/egl.h>
#include <android/log.h>
#include <dlfcn.h>
#include <elf.h>
#include <link.h>
#include <sys/mman.h>
#include <unistd.h>

#include <atomic>
#include <cstring>
#include <mutex>
#include <thread>

namespace {

constexpr char kLogTag[] = "ModernImGuiSdk";
using EglSwapBuffersFn = EGLBoolean (*)(EGLDisplay, EGLSurface);
using EglSwapBuffersWithDamageFn =
    EGLBoolean (*)(EGLDisplay, EGLSurface, const EGLint*, EGLint);
using EglProcAddress = __eglMustCastToProperFunctionPointerType;
using EglGetProcAddressFn = EglProcAddress (*)(const char*);

EglSwapBuffersFn gOriginalSwapBuffers = nullptr;
EglSwapBuffersWithDamageFn gOriginalSwapBuffersWithDamageKHR = nullptr;
EglSwapBuffersWithDamageFn gOriginalSwapBuffersWithDamageEXT = nullptr;
EglGetProcAddressFn gOriginalGetProcAddress = nullptr;
std::mutex gPatchMutex;
std::atomic_bool gHookInstalled = false;
std::atomic_uint gSwapHookCalls = 0;
std::atomic_bool gFirstFrameLogged = false;
thread_local bool gInsideHook = false;

EGLBoolean HookedEglSwapBuffers(EGLDisplay display, EGLSurface surface);
EGLBoolean HookedEglSwapBuffersWithDamageKHR(
    EGLDisplay display, EGLSurface surface, const EGLint* rects, EGLint nRects);
EGLBoolean HookedEglSwapBuffersWithDamageEXT(
    EGLDisplay display, EGLSurface surface, const EGLint* rects, EGLint nRects);
EglProcAddress HookedEglGetProcAddress(const char* name);

uintptr_t DynamicAddress(ElfW(Addr) base, ElfW(Addr) address) {
    return static_cast<uintptr_t>(base + address);
}

size_t RelocationSymbolIndex(ElfW(Xword) info) {
#if defined(__LP64__)
    return static_cast<size_t>(ELF64_R_SYM(info));
#else
    return static_cast<size_t>(ELF32_R_SYM(info));
#endif
}

bool PatchSlot(void** slot, void* replacement, void** original,
               const char* symbolName) {
    if (slot == nullptr) {
        return false;
    }

    std::lock_guard<std::mutex> lock(gPatchMutex);
    if (*slot == replacement) {
        return false;
    }

    if (original == nullptr) {
        return false;
    }
    if (*original == nullptr) {
        *original = *slot;
    }
    if (*original == nullptr) {
        *original = dlsym(RTLD_NEXT, symbolName);
    }
    if (*original == nullptr) {
        __android_log_print(ANDROID_LOG_WARN, kLogTag,
                            "Skipping %s: original symbol is unresolved",
                            symbolName);
        return false;
    }

    const long pageSize = sysconf(_SC_PAGESIZE);
    if (pageSize <= 0) {
        return false;
    }
    const uintptr_t slotAddress = reinterpret_cast<uintptr_t>(slot);
    const uintptr_t pageStart =
        slotAddress & ~static_cast<uintptr_t>(pageSize - 1);
    if (mprotect(reinterpret_cast<void*>(pageStart),
                 static_cast<size_t>(pageSize), PROT_READ | PROT_WRITE) != 0) {
        __android_log_print(ANDROID_LOG_WARN, kLogTag,
                            "mprotect failed while patching %s", symbolName);
        return false;
    }
    *slot = replacement;
    if (mprotect(reinterpret_cast<void*>(pageStart),
                 static_cast<size_t>(pageSize), PROT_READ) != 0) {
        __android_log_print(ANDROID_LOG_WARN, kLogTag,
                            "Unable to restore GOT protection after patching %s",
                            symbolName);
    }
    return true;
}

bool IsSwapSymbol(const char* symbolName) {
    return std::strcmp(symbolName, "eglSwapBuffers") == 0 ||
           std::strcmp(symbolName, "eglSwapBuffersWithDamageKHR") == 0 ||
           std::strcmp(symbolName, "eglSwapBuffersWithDamageEXT") == 0;
}

bool IsHookSymbol(const char* symbolName) {
    return IsSwapSymbol(symbolName) ||
           std::strcmp(symbolName, "eglGetProcAddress") == 0;
}

void* ReplacementForSymbol(const char* symbolName, void** original) {
    if (std::strcmp(symbolName, "eglSwapBuffers") == 0) {
        return reinterpret_cast<void*>(&HookedEglSwapBuffers);
    }
    if (std::strcmp(symbolName, "eglSwapBuffersWithDamageKHR") == 0) {
        return reinterpret_cast<void*>(&HookedEglSwapBuffersWithDamageKHR);
    }
    if (std::strcmp(symbolName, "eglSwapBuffersWithDamageEXT") == 0) {
        return reinterpret_cast<void*>(&HookedEglSwapBuffersWithDamageEXT);
    }
    if (std::strcmp(symbolName, "eglGetProcAddress") == 0) {
        return reinterpret_cast<void*>(&HookedEglGetProcAddress);
    }
    *original = nullptr;
    return nullptr;
}

void** OriginalForSymbol(const char* symbolName) {
    if (std::strcmp(symbolName, "eglSwapBuffers") == 0) {
        return reinterpret_cast<void**>(&gOriginalSwapBuffers);
    }
    if (std::strcmp(symbolName, "eglSwapBuffersWithDamageKHR") == 0) {
        return reinterpret_cast<void**>(&gOriginalSwapBuffersWithDamageKHR);
    }
    if (std::strcmp(symbolName, "eglSwapBuffersWithDamageEXT") == 0) {
        return reinterpret_cast<void**>(&gOriginalSwapBuffersWithDamageEXT);
    }
    if (std::strcmp(symbolName, "eglGetProcAddress") == 0) {
        return reinterpret_cast<void**>(&gOriginalGetProcAddress);
    }
    return nullptr;
}

bool PatchRelocations(ElfW(Addr) base,
                      const ElfW(Sym)* symtab,
                      const char* strtab,
                      uintptr_t relocations,
                      size_t relocationCount,
                      bool rela) {
    bool patched = false;
    for (size_t i = 0; i < relocationCount; ++i) {
        ElfW(Addr) offset = 0;
        ElfW(Xword) info = 0;
        if (rela) {
            const auto* entries = reinterpret_cast<const ElfW(Rela)*>(relocations);
            offset = entries[i].r_offset;
            info = entries[i].r_info;
        } else {
            const auto* entries = reinterpret_cast<const ElfW(Rel)*>(relocations);
            offset = entries[i].r_offset;
            info = entries[i].r_info;
        }

        const size_t symbolIndex = RelocationSymbolIndex(info);
        const char* symbolName = strtab + symtab[symbolIndex].st_name;
        if (!IsHookSymbol(symbolName)) {
            continue;
        }

        void** original = OriginalForSymbol(symbolName);
        void* replacement = ReplacementForSymbol(symbolName, original);
        auto** slot = reinterpret_cast<void**>(DynamicAddress(base, offset));
        if (replacement != nullptr && original != nullptr &&
            PatchSlot(slot, replacement, original, symbolName)) {
            patched = true;
        }
    }
    return patched;
}

struct PatchState {
    int patchedObjects = 0;
};

int PatchLoadedObject(struct dl_phdr_info* info, size_t, void* userData) {
    const char* name = info->dlpi_name == nullptr ? "" : info->dlpi_name;
    if (std::strstr(name, "libSdk.so") != nullptr ||
        std::strstr(name, "libEGL.so") != nullptr) {
        return 0;
    }

    const ElfW(Dyn)* dynamic = nullptr;
    for (int i = 0; i < info->dlpi_phnum; ++i) {
        if (info->dlpi_phdr[i].p_type == PT_DYNAMIC) {
            dynamic = reinterpret_cast<const ElfW(Dyn)*>(
                DynamicAddress(info->dlpi_addr, info->dlpi_phdr[i].p_vaddr));
            break;
        }
    }
    if (dynamic == nullptr) {
        return 0;
    }

    const ElfW(Sym)* symtab = nullptr;
    const char* strtab = nullptr;
    uintptr_t jumpRelocations = 0;
    size_t jumpRelocationBytes = 0;
    ElfW(Sword) relocationType = DT_REL;
    for (const ElfW(Dyn)* entry = dynamic; entry->d_tag != DT_NULL; ++entry) {
        switch (entry->d_tag) {
            case DT_SYMTAB:
                symtab = reinterpret_cast<const ElfW(Sym)*>(
                    DynamicAddress(info->dlpi_addr, entry->d_un.d_ptr));
                break;
            case DT_STRTAB:
                strtab = reinterpret_cast<const char*>(
                    DynamicAddress(info->dlpi_addr, entry->d_un.d_ptr));
                break;
            case DT_JMPREL:
                jumpRelocations = DynamicAddress(info->dlpi_addr,
                                                 entry->d_un.d_ptr);
                break;
            case DT_PLTRELSZ:
                jumpRelocationBytes = static_cast<size_t>(entry->d_un.d_val);
                break;
            case DT_PLTREL:
                relocationType = entry->d_un.d_val;
                break;
            default:
                break;
        }
    }

    if (symtab == nullptr || strtab == nullptr || jumpRelocations == 0 ||
        jumpRelocationBytes == 0) {
        return 0;
    }

    const bool rela = relocationType == DT_RELA;
    const size_t entrySize = rela ? sizeof(ElfW(Rela)) : sizeof(ElfW(Rel));
    if (entrySize == 0) {
        return 0;
    }
    const size_t entryCount = jumpRelocationBytes / entrySize;
    if (PatchRelocations(info->dlpi_addr, symtab, strtab, jumpRelocations,
                         entryCount, rela)) {
        auto* state = static_cast<PatchState*>(userData);
        if (state != nullptr) {
            ++state->patchedObjects;
        }
        __android_log_print(
            ANDROID_LOG_INFO, kLogTag,
            "Patched EGL swap import in object: %s",
            name[0] == '\0' ? "<main executable>" : name);
    }
    // Returning non-zero stops dl_iterate_phdr immediately. Keep walking so
    // Unity and its bootstrap libraries are all patched, not just the first
    // system/library object that imports eglSwapBuffers.
    return 0;
}

void RenderOnSwap(EGLDisplay display, EGLSurface surface, const char* symbolName,
                  unsigned int callNumber) {
    if (eglGetCurrentContext() == EGL_NO_CONTEXT) {
        if (callNumber == 0) {
            __android_log_print(ANDROID_LOG_ERROR, kLogTag,
                                "%s has no current EGL context", symbolName);
        }
        return;
    }

    EGLint width = 0;
    EGLint height = 0;
    if (eglQuerySurface(display, surface, EGL_WIDTH, &width) != EGL_TRUE ||
        eglQuerySurface(display, surface, EGL_HEIGHT, &height) != EGL_TRUE ||
        width <= 0 || height <= 0) {
        if (callNumber == 0) {
            __android_log_print(ANDROID_LOG_ERROR, kLogTag,
                                "%s could not query surface dimensions",
                                symbolName);
        }
        return;
    }

    if (!Sdk_InitializeOnCurrentContext(nullptr)) {
        if (callNumber == 0) {
            __android_log_print(ANDROID_LOG_ERROR, kLogTag,
                                "ImGui initialization failed on %s (%dx%d)",
                                symbolName, width, height);
        }
        return;
    }

    Sdk_SetDisplaySize(width, height);
    Sdk_RenderOnCurrentContext();
    if (!gFirstFrameLogged.exchange(true)) {
        __android_log_print(ANDROID_LOG_INFO, kLogTag,
                            "ImGui rendered on %s (%dx%d)", symbolName, width,
                            height);
    }
}

EGLBoolean HookedEglSwapBuffers(EGLDisplay display, EGLSurface surface) {
    if (gInsideHook || gOriginalSwapBuffers == nullptr) {
        return gOriginalSwapBuffers == nullptr
                   ? EGL_FALSE
                   : gOriginalSwapBuffers(display, surface);
    }

    gInsideHook = true;
    const unsigned int callNumber = gSwapHookCalls.fetch_add(1);
    if (callNumber == 0) {
        __android_log_print(ANDROID_LOG_INFO, kLogTag,
                            "eglSwapBuffers hook called for the first time");
    }
    RenderOnSwap(display, surface, "eglSwapBuffers", callNumber);

    EGLBoolean result = gOriginalSwapBuffers(display, surface);
    gInsideHook = false;
    return result;
}

EGLBoolean HookedEglSwapBuffersWithDamageKHR(
    EGLDisplay display, EGLSurface surface, const EGLint* rects, EGLint nRects) {
    if (gInsideHook || gOriginalSwapBuffersWithDamageKHR == nullptr) {
        return gOriginalSwapBuffersWithDamageKHR == nullptr
                   ? EGL_FALSE
                   : gOriginalSwapBuffersWithDamageKHR(display, surface, rects,
                                                       nRects);
    }
    gInsideHook = true;
    const unsigned int callNumber = gSwapHookCalls.fetch_add(1);
    if (callNumber == 0) {
        __android_log_print(ANDROID_LOG_INFO, kLogTag,
                            "eglSwapBuffersWithDamageKHR hook called");
    }
    RenderOnSwap(display, surface, "eglSwapBuffersWithDamageKHR", callNumber);
    EGLBoolean result =
        gOriginalSwapBuffersWithDamageKHR(display, surface, rects, nRects);
    gInsideHook = false;
    return result;
}

EGLBoolean HookedEglSwapBuffersWithDamageEXT(
    EGLDisplay display, EGLSurface surface, const EGLint* rects, EGLint nRects) {
    if (gInsideHook || gOriginalSwapBuffersWithDamageEXT == nullptr) {
        return gOriginalSwapBuffersWithDamageEXT == nullptr
                   ? EGL_FALSE
                   : gOriginalSwapBuffersWithDamageEXT(display, surface, rects,
                                                       nRects);
    }
    gInsideHook = true;
    const unsigned int callNumber = gSwapHookCalls.fetch_add(1);
    if (callNumber == 0) {
        __android_log_print(ANDROID_LOG_INFO, kLogTag,
                            "eglSwapBuffersWithDamageEXT hook called");
    }
    RenderOnSwap(display, surface, "eglSwapBuffersWithDamageEXT", callNumber);
    EGLBoolean result =
        gOriginalSwapBuffersWithDamageEXT(display, surface, rects, nRects);
    gInsideHook = false;
    return result;
}

EglProcAddress HookedEglGetProcAddress(const char* name) {
    if (gOriginalGetProcAddress == nullptr) {
        return nullptr;
    }

    EglProcAddress resolved = gOriginalGetProcAddress(name);
    if (name == nullptr || resolved == nullptr) {
        return resolved;
    }

    if (std::strcmp(name, "eglSwapBuffers") == 0) {
        if (gOriginalSwapBuffers == nullptr) {
            gOriginalSwapBuffers = reinterpret_cast<EglSwapBuffersFn>(resolved);
        }
        return reinterpret_cast<EglProcAddress>(&HookedEglSwapBuffers);
    }
    if (std::strcmp(name, "eglSwapBuffersWithDamageKHR") == 0) {
        if (gOriginalSwapBuffersWithDamageKHR == nullptr) {
            gOriginalSwapBuffersWithDamageKHR =
                reinterpret_cast<EglSwapBuffersWithDamageFn>(resolved);
        }
        return reinterpret_cast<EglProcAddress>(
            &HookedEglSwapBuffersWithDamageKHR);
    }
    if (std::strcmp(name, "eglSwapBuffersWithDamageEXT") == 0) {
        if (gOriginalSwapBuffersWithDamageEXT == nullptr) {
            gOriginalSwapBuffersWithDamageEXT =
                reinterpret_cast<EglSwapBuffersWithDamageFn>(resolved);
        }
        return reinterpret_cast<EglProcAddress>(
            &HookedEglSwapBuffersWithDamageEXT);
    }
    return resolved;
}

void HookThread() {
    bool foundAnyObject = false;
    // Unity may load libunity.so after JNI_OnLoad returns. Keep scanning after
    // the first match so a bootstrap library does not prevent the actual
    // renderer library from being patched later.
    for (int attempt = 0; attempt < 300; ++attempt) {
        PatchState state;
        dl_iterate_phdr(PatchLoadedObject, &state);
        if (state.patchedObjects > 0) {
            foundAnyObject = true;
            const bool firstInstall = !gHookInstalled.exchange(true);
            __android_log_print(
                ANDROID_LOG_INFO, kLogTag,
                firstInstall
                    ? "eglSwapBuffers hook installed in %d object(s)"
                    : "eglSwapBuffers hook patched %d additional object(s)",
                state.patchedObjects);
        }
        usleep(100000);
    }
    if (!foundAnyObject) {
        __android_log_print(ANDROID_LOG_ERROR, kLogTag,
                            "Unable to find EGL swap/proc relocations");
    }
}

}  // namespace

extern "C" void Sdk_StartEglHook() {
    static std::once_flag startFlag;
    std::call_once(startFlag, []() {
        std::thread(HookThread).detach();
    });
}