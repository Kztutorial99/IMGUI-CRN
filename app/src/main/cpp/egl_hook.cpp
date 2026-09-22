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

EglSwapBuffersFn gOriginalSwapBuffers = nullptr;
std::mutex gPatchMutex;
std::atomic_bool gHookInstalled = false;
thread_local bool gInsideHook = false;

EGLBoolean HookedEglSwapBuffers(EGLDisplay display, EGLSurface surface);

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

bool PatchSlot(void** slot) {
    if (slot == nullptr) {
        return false;
    }

    std::lock_guard<std::mutex> lock(gPatchMutex);
    if (*slot == reinterpret_cast<void*>(&HookedEglSwapBuffers)) {
        return false;
    }

    if (gOriginalSwapBuffers == nullptr) {
        gOriginalSwapBuffers = reinterpret_cast<EglSwapBuffersFn>(*slot);
    }
    if (gOriginalSwapBuffers == nullptr) {
        gOriginalSwapBuffers =
            reinterpret_cast<EglSwapBuffersFn>(dlsym(RTLD_NEXT, "eglSwapBuffers"));
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
        return false;
    }
    *slot = reinterpret_cast<void*>(&HookedEglSwapBuffers);
    mprotect(reinterpret_cast<void*>(pageStart),
             static_cast<size_t>(pageSize), PROT_READ);
    return true;
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
        if (std::strcmp(symbolName, "eglSwapBuffers") != 0) {
            continue;
        }

        auto** slot = reinterpret_cast<void**>(DynamicAddress(base, offset));
        patched = PatchSlot(slot) || patched;
    }
    return patched;
}

int PatchLoadedObject(struct dl_phdr_info* info, size_t, void*) {
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
    return PatchRelocations(info->dlpi_addr, symtab, strtab, jumpRelocations,
                            entryCount, rela)
               ? 1
               : 0;
}

EGLBoolean HookedEglSwapBuffers(EGLDisplay display, EGLSurface surface) {
    if (gInsideHook || gOriginalSwapBuffers == nullptr) {
        return gOriginalSwapBuffers == nullptr
                   ? EGL_FALSE
                   : gOriginalSwapBuffers(display, surface);
    }

    gInsideHook = true;
    EGLContext context = eglGetCurrentContext();
    if (context != EGL_NO_CONTEXT) {
        EGLint width = 0;
        EGLint height = 0;
        if (eglQuerySurface(display, surface, EGL_WIDTH, &width) == EGL_TRUE &&
            eglQuerySurface(display, surface, EGL_HEIGHT, &height) == EGL_TRUE &&
            width > 0 && height > 0 &&
            Sdk_InitializeOnCurrentContext(nullptr)) {
            Sdk_SetDisplaySize(width, height);
            Sdk_RenderOnCurrentContext();
        }
    }

    EGLBoolean result = gOriginalSwapBuffers(display, surface);
    gInsideHook = false;
    return result;
}

void HookThread() {
    for (int attempt = 0; attempt < 120 && !gHookInstalled; ++attempt) {
        const int patched = dl_iterate_phdr(PatchLoadedObject, nullptr);
        if (patched > 0) {
            gHookInstalled = true;
            __android_log_print(ANDROID_LOG_INFO, kLogTag,
                                "eglSwapBuffers hook installed");
            return;
        }
        usleep(100000);
    }
    __android_log_print(ANDROID_LOG_ERROR, kLogTag,
                        "Unable to find an eglSwapBuffers relocation");
}

}  // namespace

extern "C" void Sdk_StartEglHook() {
    static std::once_flag startFlag;
    std::call_once(startFlag, []() {
        std::thread(HookThread).detach();
    });
}