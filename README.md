# Modern ImGui Android SDK

Base Android NativeActivity untuk Dear ImGui + OpenGL ES 3 dengan desain modern
dan tiga menu utama:

- **Overview** — status, statistik, dan live telemetry
- **Controls** — slider, toggle, apply, dan reset
- **Settings** — preferensi dan informasi build

## Output library

Nama target CMake adalah `Sdk`, sehingga Android menghasilkan:

```text
libSdk.so
```

Library dimuat oleh `android.app.NativeActivity` melalui metadata
`android.app.lib_name`.

## Build

Buka folder ini di Android Studio dengan Android SDK, NDK `27.0.12077973`,
dan CMake `3.22.1` terpasang, lakukan Gradle Sync, lalu jalankan task
`app > Tasks > build > assembleDebug`. Jika Gradle tersedia di PATH, perintah
yang setara adalah:

```bash
gradle :app:assembleDebug
```

APK debug akan berada di `app/build/outputs/apk/debug/`.

## Base upstream

Project mengambil sumber resmi Dear ImGui dari:

```text
https://github.com/ocornut/imgui
```

Versi dipin ke `v1.92.9b` di `app/src/main/cpp/CMakeLists.txt`, memakai
`examples/example_android_opengl3` sebagai pola NativeActivity/OpenGL ES 3.
Untuk upgrade, ubah `IMGUI_VERSION` ke tag resmi yang ingin digunakan.