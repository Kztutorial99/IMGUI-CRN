#pragma once

#define HOOK(ret, func, ...) \
    ret (*orig##func)(__VA_ARGS__); \
    ret my##func(__VA_ARGS__)

HOOK(void, Input, void *thiz, void *ex_ab, void *ex_ac) { 
    origInput(thiz, ex_ab, ex_ac);
ImGui_ImplAndroid_HandleInputEvent((AInputEvent *)thiz);
    return;
}

int32_t (*orig_ANativeWindow_getWidth)(ANativeWindow* window);
int32_t _ANativeWindow_getWidth(ANativeWindow* window) {
	screenWidth = orig_ANativeWindow_getWidth(window);
	return orig_ANativeWindow_getWidth(window);
}

int32_t (*orig_ANativeWindow_getHeight)(ANativeWindow* window);
int32_t _ANativeWindow_getHeight(ANativeWindow* window) {
	screenHeight = orig_ANativeWindow_getHeight(window);
	return orig_ANativeWindow_getHeight(window);
}

/*

-> Change the name of the game's libmain.so to libGUI.so.
-> Add your Gane libmain.so To Game lib folder.

const-string v0, "GUI"
 
invoke-static {v0}, Ljava/lang/System;->loadLibrary(Ljava/lang/String;)V

*/

int (*Screen_get_height)();
int (*Screen_get_width)();

static bool EnableESP = false;
static bool ESPLine = false;
static bool ESPBox = false;

namespace Draw
{
    void DrawLine(ImVec2 start, ImVec2 end, ImVec4 color, float thickness = 1.0f) {
        auto background = ImGui::GetBackgroundDrawList();
        if (background) {
            background->AddLine(start, end, ImColor(color.x, color.y, color.z, color.w), thickness);
        }
    }  
    void DrawBox(Rect rect, ImVec4 color) {
        ImVec2 v1(rect.x, rect.y);
        ImVec2 v2(rect.x + rect.width, rect.y);
        ImVec2 v3(rect.x + rect.width, rect.y + rect.height);
        ImVec2 v4(rect.x, rect.y + rect.height);

        DrawLine(v1, v2, color);
        DrawLine(v2, v3, color);
        DrawLine(v3, v4, color);
        DrawLine(v4, v1, color);
    }
}

vector<void*> players;

void clearPlayers() {
    vector<void*> pls;
    for (int i = 0; i < players.size(); i++) {
        if (players[i] != NULL) {
            pls.push_back(players[i]);
        }
    }
    players = pls;
}

bool playerFind(void* pl) {
    if (pl != NULL) {
        for (int i = 0; i < players.size(); i++) {
            if (pl == players[i]) return true;
        }
    }
    return false;
}

Vector3 (*WorldToScreenPoint)(void* instance, Vector3);
Vector3 (*Transform_get_position)(void* instance);
void* (*get_main)();
void* (*get_transform)(void* instance);

Vector3 getPosition(void* transform) {
    return Transform_get_position(get_transform(transform));
}

void* myPlayer = NULL;
void (*old_PlayerUpdate)(void* player);
void PlayerUpdate(void* player) {
    if (player != NULL) {
        glWidth = Screen_get_width();
        glHeight = Screen_get_height();
        if (!playerFind(player)) players.push_back(player);
    }
    old_PlayerUpdate(player);
}


void (*old_Player_OnDestroy)(...);
void Player_OnDestroy(void* player) {
    if (player != NULL) {
        old_Player_OnDestroy(player);
        // Clear players only if the player is in the list
        players.erase(remove(players.begin(), players.end(), player), players.end());
    }
}

void DrawEsp() {
    for (int i = 0; i < players.size(); i++) {
        void* Player = players[i];

        // Ensure the player is still valid before proceeding
        if (Player == NULL) continue;

        Vector3 PlayerPos = getPosition(Player);
        Vector3 Head = Vector3(PlayerPos.x, PlayerPos.y + 1.6f, PlayerPos.z);
        Vector3 Feet = Vector3(PlayerPos.x, PlayerPos.y, PlayerPos.z);

        auto HeadPosition = WorldToScreenPoint(get_main(), Head);
        auto FeetPosition = WorldToScreenPoint(get_main(), Feet);

        if (HeadPosition.z < 1.f || FeetPosition.z < 1.f)
            continue;

        float boxHeight = abs(HeadPosition.y - FeetPosition.y);
        float boxWidth = boxHeight * 0.65f;
        Rect playerRect(HeadPosition.x - (boxWidth / 2), (glHeight - HeadPosition.y - 5.0f), boxWidth, boxHeight);

        if (EnableESP) {
            if (Player != NULL && get_main() != NULL) {
                // Draw Line
                if (ESPLine) {
                    Draw::DrawLine(ImVec2(glWidth * 0.5f, glHeight * 0.0f),
                    ImVec2(HeadPosition.x, glHeight - HeadPosition.y),
                    ImVec4(0, 255, 0, 1), 1.0f);
                }

                // Draw Box
                if (ESPBox) {
                    Draw::DrawBox(playerRect, ImVec4(0, 255, 0, 1));
                }
            }
        }
    } // Closing bracket for the for loop
} // Closing bracket for the function


void EspPointers() {
	
	Screen_get_width = (int (*)()) IL2Cpp::Il2CppGetMethodOffset(
        OBFUSCATE("UnityEngine.dll"),
        OBFUSCATE("UnityEngine"),
        OBFUSCATE("Screen"),
        OBFUSCATE("get_width"),
        0
    );

    Screen_get_height = (int (*)()) IL2Cpp::Il2CppGetMethodOffset(
        OBFUSCATE("UnityEngine.dll"),
        OBFUSCATE("UnityEngine"),
        OBFUSCATE("Screen"),
        OBFUSCATE("get_height"),
        0
    );

    get_transform = (void *(*)(void *)) IL2Cpp::Il2CppGetMethodOffset(
        OBFUSCATE("UnityEngine.dll"),
        OBFUSCATE("UnityEngine"),
        OBFUSCATE("Component"),
        OBFUSCATE("get_transform"),
        0
    );

    Transform_get_position = (Vector3 (*)(void *)) IL2Cpp::Il2CppGetMethodOffset(
        OBFUSCATE("UnityEngine.dll"),
        OBFUSCATE("UnityEngine"),
        OBFUSCATE("Transform"),
        OBFUSCATE("get_position"),
        0
    );

    get_main = (void *(*)()) IL2Cpp::Il2CppGetMethodOffset(
        OBFUSCATE("UnityEngine.dll"),
        OBFUSCATE("UnityEngine"),
        OBFUSCATE("Camera"),
        OBFUSCATE("get_main"),
        0
    );

    WorldToScreenPoint = (Vector3 (*)(void *, Vector3)) IL2Cpp::Il2CppGetMethodOffset(
        OBFUSCATE("UnityEngine.dll"),
        OBFUSCATE("UnityEngine"),
        OBFUSCATE("Camera"),
        OBFUSCATE("WorldToScreenPoint"),
        1
    );
	
	
	// Use Your Game Namespace , class name , Method name 
	Tools::Hook(
        (void *)(uintptr_t) IL2Cpp::Il2CppGetMethodOffset(
            OBFUSCATE("Assembly-CSharp.dll"),
            OBFUSCATE("DProject"),  // Namespace
            OBFUSCATE("ZombieBase"), // Class
            OBFUSCATE("LateUpdate"), // Method
            0
        ),
        (void *)PlayerUpdate,
        (void **)&old_PlayerUpdate
    );

    
    Tools::Hook(
        (void *)(uintptr_t) IL2Cpp::Il2CppGetMethodOffset(
            OBFUSCATE("Assembly-CSharp.dll"),
            OBFUSCATE("DProject"),  // Namespace
            OBFUSCATE("ZombieBase"), // Class
            OBFUSCATE("OnDestroy"), // Method
            0
        ),
        (void *)Player_OnDestroy,
        (void **)&old_Player_OnDestroy
    );
	
}




// ====== Normal Hooks ===== 
// Normal Hook Example 

// Namespace: DProject
// public class DSystem
// public int get_grenadesCount() { }

bool Isspeed = false;
/*
void (*old_speed)(void *instance);
void speed(void *instance) {
    if (instance != NULL) {
        if (Isspeed) {
            *(float *)((uint64_t)instance + 0x264) = 50;
   }
    }
    return old_speed(instance);
}
// Add more Hooks Here 
*/

// =============== \\


void HookPointers() {
	
	/* Tools::Hook(
        (void *)(uintptr_t) IL2Cpp::Il2CppGetMethodOffset(
            OBFUSCATE("Assembly-CSharp.dll"),
            OBFUSCATE(""),  // Namespace
            OBFUSCATE("AIController"), // Class
            OBFUSCATE("Update"), // Method
            0 // Parameter
            ),(void *)speed,(void **)&old_speed);
			*/
			
			
			
			// Add more pointers here 
	
	
}




// =========================
// Read Below codes for better understanding 🙏 
/*
 * IL2Cpp::Il2CppGetMethodOffset is used to find the memory address of a method inside Unity's IL2CPP framework.
 *
 * Parameters:
 * 1. "Assembly.dll"   - The DLL file where the method is located.
 *    - Example: "UnityEngine.dll" (for Unity's built-in functions)
 *    - Example: "Assembly-CSharp.dll" (for game-specific scripts)
 *
 * 2. "Namespace"      - The namespace where the class belongs.
 *    - Example: "UnityEngine" (for Unity's built-in classes)
 *    - Example: "DProject" (for game-specific classes)
 *    - If there is no namespace, use an empty string: ""
 *
 * 3. "ClassName"      - The class that contains the method.
 *    - Example: "Screen" (if accessing screen width or height)
 *    - Example: "Transform" (if accessing object position)
 *
 * 4. "MethodName"     - The function that you want to find.
 *    - Example: "get_width" (to get screen width)
 *    - Example: "get_position" (to get an object’s position)
 *
 * 5. ParameterCount   - Number of arguments the function takes.
 *    - Example: 0 (if the function takes no parameters, like "get_width")
 *    - Example: 1 (if the function takes one parameter, like "set_position(Vector3 pos)")
 *
 * Example Usage:

 * Example 1: Getting Screen Width (No Parameters)
 * This function does not take any parameters, so the last argument is 0.
 
Screen_get_width = (int (*)()) IL2Cpp::Il2CppGetMethodOffset(
    "UnityEngine.dll",  // 1. Assembly (DLL) name
    "UnityEngine",      // 2. Namespace
    "Screen",           // 3. Class
    "get_width",        // 4. Method Name
    0                   // 5. No parameters (0)
);


 * Example 2: Setting an Object's Position (One Parameter)
 * This function takes one parameter (Vector3 newPosition), so the last argument is 1.
 
Transform_set_position = (void (*)(void *, Vector3)) IL2Cpp::Il2CppGetMethodOffset(
    "UnityEngine.dll",  // 1. Assembly (DLL) name
    "UnityEngine",      // 2. Namespace
    "Transform",        // 3. Class
    "set_position",     // 4. Method Name
    1                   // 5. One parameter (Vector3)
);


 * Example 6: Hooking a Function with Two Parameters
 * The function "ApplyDamage" takes two parameters (int damage, bool critical), so the last argument is 2.
 
Tools::Hook(
    (void *)(uintptr_t) IL2Cpp::Il2CppGetMethodOffset(
        "Assembly-CSharp.dll", // 1. Game's assembly
        "EnemySystem",         // 2. Namespace
        "Enemy",               // 3. Class
        "ApplyDamage",         // 4. Method Name
        2                      // 5. Two parameters (int damage, bool critical)
    ),
    (void *)NewApplyDamage,    // New function to replace the original
    (void **)&oldApplyDamage   // Store original function
);

*/
