# Analisis `dump.cs`

## Sumber dan hasil ekstraksi

- Arsip: `com.ez.ths_1.0.159_1790097535269.zip`
- Isi arsip: `com.ez.ths_1.0.159.cs`
- Hasil yang disimpan: `dump_file/dump.cs`
- Ukuran dump: sekitar 35,6 MB
- Header dump menunjukkan `Assembly-CSharp.dll`, sehingga ini adalah metadata
  assembly Unity/IL2CPP, bukan source C# asli.

Nama class dan field di beberapa bagian sudah di-obfuscate. Karena itu, nama
field seperti `PIGAJKODKBC` tidak boleh dianggap stabil; integrasi produksi
harus memakai discovery/runtime adapter dan validasi versi.

## Tiga fitur yang paling realistis untuk ImGui

### 1. Skill Loadout & Cooldown Dashboard

**Dasar dari dump:**

- `THS.SkillSlot` mendefinisikan `Skill_1`, `Skill_2`, dan `Skill_3`
  sekitar baris 101439–101445.
- `THS.Skill` menyediakan `GetSkills()`, `GetSkill(...)`, `GetSkillConfig(...)`,
  dan `SkillRuntime(...)` sekitar baris 101560–101621.
- Daftar ID skill yang terlihat mencakup ice sentry, ice landmine, healing,
  smoke, sonar, dash, dan jump jet.
- `THS.SkillBastion` memiliki status `InUsing`, `BulletNum`, serta API UI
  `SetSkillUICountDown()` dan `SetCountDownProgressBar()` sekitar baris
  102119–102154.
- `VUIHud` memiliki data cooldown seperti `ImgCooldown`, `TxtCooldown`,
  `isInCooldown`, `cooldownProgress`, dan `cooldownLeftTime` sekitar baris
  110901–110917.

**Bentuk UI ImGui:**

- Tiga kartu skill untuk slot 1–3.
- Nama/ID skill dan status `Ready`, `Active`, atau `Cooldown`.
- Progress bar cooldown dan sisa waktu.
- Jumlah charge/bullet bila data tersedia.
- Filter hanya untuk skill pemain lokal.

**Batasan:**

Fitur ini sebaiknya read-only untuk HUD/debug. Jangan memanggil method
network/action seperti `ReqSkillAction()` hanya dari tombol ImGui, karena
dump memperlihatkan adanya sinkronisasi pesan dan state server.

### 2. Player Status & Session HUD

**Dasar dari dump:**

- `COW.GamePlay.Player` diturunkan dari `AttackableEntity` sekitar baris
  331991.
- Player memiliki `isInZeppelin`, `IsAimAssitDisabled`, `MainCameraTransform`,
  `TeamModeID`, `IsClientBot`, dan `CharacterController` sekitar baris
  332008–332053.
- Tersedia getter untuk `UserID`, `PlayerID`, `TeamIndex`, `GroupName`,
  `GetAttackableCenterWS()`, dan `GetAttackableRadius()` sekitar baris
  332702–332723.
- Dump juga berisi banyak controller HUD untuk nama player, teammate,
  minimap, vehicle, reload, dan match information.

**Bentuk UI ImGui:**

- Panel status pemain lokal.
- Team index/mode, status kendaraan atau zeppelin, dan status bot.
- ID/session label yang disamarkan atau dipendekkan.
- Position/camera diagnostic hanya jika memang diperlukan untuk debugging.
- FPS, frame time, dan status koneksi dari sisi aplikasi ImGui.

**Batasan:**

Jangan menampilkan atau memproses data pemain lain sebagai wallhack/ESP.
Versi pertama sebaiknya hanya menampilkan status pemain lokal dan data HUD
yang memang sudah terlihat oleh pemain.

### 3. Settings Hub: Graphics, Controls, Sensitivity, Sound

**Dasar dari dump:**

- Ada `GraphicSettingInfo` dan `UIGraphicSettingController` sekitar baris
  5865–5911.
- Ada `OperationSettingInfo` dan `UIOperationSettingController` sekitar baris
  6079–6176.
- `UISensiSettingController` mendefinisikan sensitivity umum, x1/x2/x4/x8
  scope, auxiliary aim, dan fire sensitivity sekitar baris 6312–6355.
- `UISoundSettingController` mendefinisikan music, sound effect, kill sound,
  game voice, fire sound, achievement sound, dan special character voice
  sekitar baris 6388–6429.

**Bentuk UI ImGui:**

- Tab `Graphics`: preset kualitas, FPS target, dan toggle efek.
- Tab `Controls`: tombol, input method, dan layout.
- Tab `Sensitivity`: slider sensitivity umum dan per-scope.
- Tab `Sound`: slider music, effect, voice, dan kill sound.
- Tombol `Apply`, `Reset`, dan indikator perubahan yang belum diterapkan.

**Batasan:**

Gunakan jalur pengaturan resmi yang tersedia di aplikasi. Jangan mengubah
memory secara langsung berdasarkan offset dump, karena offset dapat berubah
setiap update dan berisiko crash.

## Rekomendasi urutan implementasi

1. Mulai dari **Settings Hub** karena paling rendah risiko dan paling mudah
   diuji secara visual.
2. Lanjutkan ke **Skill Dashboard** dalam mode read-only.
3. Tambahkan **Player Status HUD** hanya untuk pemain lokal.

## Catatan kompatibilitas

Dump ini berasal dari versi `1.0.159`. Address seperti `0x46a873c` adalah
address hasil dump untuk versi tersebut, bukan API publik. Update game dapat
mengubah address, class, field, dan alur data. Source ImGui sebaiknya memiliki
adapter terpisah dari layer UI agar perubahan runtime tidak merusak desain.