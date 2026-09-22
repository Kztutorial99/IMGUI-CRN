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

## Fokus gameplay: Aim, Player, dan efek in-game

Pencarian lanjutan menemukan sistem gameplay yang lebih dekat dengan kebutuhan
menu in-game:

### Aim dan crosshair

Indikator yang ditemukan:

- `ZLH.CameraManager.TickAimAssist()` dan `TestAimAssist()` sekitar baris
  21252–21258.
- Parameter `sight_aim_assist_snap_range`,
  `sight_aim_assist_dist_falloff`, `sight_aim_assist_distpriority`, dan
  `sight_aim_assist` sekitar baris 21564–21573.
- `ZLH.UISensiSettingData` memiliki `HipAimAssist`, `SightAimAssist`, dan
  `AimAssistVertical` sekitar baris 22784–22786.
- Ada `AimAssistAutoLock`, `AimAssistOnSighting`, dan `CrossHairConfig`
  sekitar baris 23856–23857 serta 21610.
- `ZLH.Player.get_AimDir()` terlihat sekitar baris 22657.

**UI yang aman untuk dibuat:** menu Aim yang menampilkan status setting aim
resmi, sensitivity hip/scope, pilihan crosshair, dan indikator visual ketika
assist resmi aktif. Nilai konfigurasi hanya diubah melalui API settings resmi.

**Yang tidak dibuat:** aimbot, auto-lock target dengan mengubah rotasi player,
target selection tersembunyi, atau pengiriman action/network paksa.

### Player, enemy, dan teammate

Indikator yang ditemukan:

- `ZLH.LocalPlayer` dan `ZLH.NetworkPlayer` sekitar baris 22595–22639.
- `ZLH.Player.GetAllEnemys()` dan `GetAllTeammates()` sekitar baris
  22450–22451.
- `ZLH.Player.get_PlayerPos()` sekitar baris 22566.
- `ZLH.Player.SetEnemyEffect(Boolean)` sekitar baris 22581.
- `COW.HUD.UIHudNameEnemyController`,
  `UIHudNameLocalController`, dan `UIHudTeammateBriefController` ada di
  area controller HUD sekitar baris 302658–302853.

**UI yang aman untuk dibuat:** panel Player yang menampilkan player lokal,
teammate, nama, status, dan marker yang mengikuti aturan visibility game.
Marker hanya boleh muncul jika entity memang terdeteksi/terlihat oleh sistem
HUD normal.

**Yang tidak dibuat:** ESP, wallhack, skeleton melalui dinding, membaca posisi
enemy yang tidak terlihat, atau mengubah `SetEnemyEffect` menjadi highlight
global.

### Combat feedback dan efek visual

Indikator yang ditemukan:

- `EnemyFootStepSetting` dan `EnemyFireSetting` sekitar baris 5872–5873.
- `EnemyFireHint` pada `UIOperationSettingController` sekitar baris 6136.
- `ZLH.Player` menyimpan `PlayerPos`, `AimDir`, dan data target/rotasi di
  area sekitar baris 22566–22657.
- Ada konfigurasi recoil di `ZLH.CameraManager` dan `V.RecoilConfig` sekitar
  baris 21268–21270.

**UI yang aman untuk dibuat:** menu Visual/Combat berisi crosshair preview,
hit marker yang berasal dari event game normal, indikator enemy fire/footstep
yang memang disediakan game, dan preview recoil/camera feedback.

## Rekomendasi menu gameplay

Jika tiga menu yang diinginkan benar-benar berorientasi in-game, struktur yang
paling sesuai adalah:

1. **Aim** — status aim assist resmi, sensitivity, scope, crosshair.
2. **Player** — status player lokal, teammate, marker visibility normal.
3. **Visuals** — hit marker, enemy fire/footstep hint, recoil dan screen effect.

Struktur ini memakai sistem yang benar-benar ditemukan di dump, tetapi tetap
memisahkan UI diagnostik/konfigurasi dari fitur yang memberi keuntungan tidak
adil pada multiplayer.