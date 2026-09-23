#pragma once
#include "imgui.h"

namespace ImHotKey
{
    struct HotKey
    {
        const char *functionName;
        const char *functionLib;
        unsigned int functionKeys;
    };

    static std::vector<HotKey> hotkeys = {
        {"Open Menu", "Show the main menu", 0xFFFF261D},
        {"Checkbox ESP", "Enable/Disable ESP", 0xFFFF1F1D},
        {"Aim Assist", "Enable Aim Assist", 0xFFFF181D},
        {"Speed Hack", "Increase movement speed", 0xFFFFFF3F},
        {"God Mode", "Invincibility", 0xFFFFFF1F},
    };

    static void EditHotkeys()
    {
        if (ImGui::BeginPopupModal("HotKeys Editor", NULL, ImGuiWindowFlags_NoResize))
        {
            for (size_t i = 0; i < hotkeys.size(); i++)
            {
                ImGui::Text("%s - %s", hotkeys[i].functionName, hotkeys[i].functionLib);
                ImGui::SameLine();
                if (ImGui::Button("Edit"))
                {
                    // Implement key binding logic here
                }
            }
            if (ImGui::Button("Close"))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
    }

    static void ShowHotkeyMenu()
    {
        if (ImGui::Button("Edit Hotkeys"))
        {
            ImGui::OpenPopup("HotKeys Editor");
        }
        EditHotkeys();
    }

    static int GetHotKey()
    {
        for (size_t i = 0; i < hotkeys.size(); i++)
        {
            if (ImGui::IsKeyPressed(hotkeys[i].functionKeys))
            {
                return i;
            }
        }
        return -1;
    }
}
