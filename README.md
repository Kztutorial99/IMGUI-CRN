# ImGui Universal Android

Project ini sekarang memakai source ZIP `ImGui Universal` sebagai basis native
dan memasukkan menu project kita ke dalam `app/src/main/cpp/ProjectMenu.h`.

## Menu project

Menu yang aktif adalah:

- **Aim** — aim settings, sensitivity, scope, recoil feedback, dan crosshair
- **Player** — status player lokal, teammate marker, dan player debug
- **Visuals** — hit marker, fire/footstep hint, screen effect, dan outline preview

State menu sengaja dipisahkan dari adapter runtime. Jadi offset, method IL2CPP,
atau callback fitur game target dapat ditambahkan kemudian tanpa mengubah
renderer dan layout menu.

## Output library

`Android.mk` menghasilkan:

```text
libSdk.so
```

NativeActivity memuat library tersebut melalui metadata
`android.app.lib_name`. JNI loader tidak lagi meneruskan ke `librealmain.so`;
`JNI_OnLoad` langsung mengembalikan `JNI_VERSION_1_6`.

## Build

Buka project ini dengan Android Studio menggunakan:

- Android SDK platform 35
- NDK `27.0.12077973`
- Android Gradle Plugin `8.7.3`

Jalankan:

```bash
gradle :app:assembleDebug
```

ABI yang dibuild:

- `armeabi-v7a`
- `arm64-v8a`

APK debug berada di `app/build/outputs/apk/debug/`.

## Catatan runtime

Source ZIP masih memakai hook Dobby untuk `eglSwapBuffers` dan backend ImGui
Android/OpenGL ES 3 versi ZIP. Path library system sudah dibuat ABI-aware.
Log yang diperlukan untuk menyatakan overlay aktif adalah:

```text
library loaded
eglSwapBuffers hook installed
hook called
surface dimensions valid
ImGui initialized
first frame rendered
```

Offset dan fitur game-specific dari source ZIP belum dianggap valid untuk target
baru. Adapter runtime harus menggunakan dump, ABI, dan versi game target sendiri.