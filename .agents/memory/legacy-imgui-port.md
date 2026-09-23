---
name: Legacy ImGui source port
description: How the uploaded Android ImGui source is used as the active native base.
---

Use the uploaded legacy ImGui source as the active native base when that is the requested target. Keep project-specific UI in a separate menu adapter included by its entry point, and keep game-specific offsets behind an adapter boundary.

**Why:** The user explicitly chose the ZIP project as the base. Its old backend and hook assumptions must be fixed in place rather than silently replacing the source with the previous CMake implementation.

**How to apply:** Preserve the ZIP's source tree and build model, but replace its menu and proxy-library entry behavior with project code. Do not assume its offsets, class names, or prebuilt binaries match a new target.