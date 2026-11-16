#include <cstdio>
#include <string>
#include <cstring>
#include <iostream>
#include <sstream>
#include <chrono>
#include <atomic>
#include <jni.h>
#include <iomanip>
#include <map>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <EGL/egl.h>
#include <unordered_map>
#include "Includes/Utils.h"
#include "ImGui/Call_ImGui.h"
#include "BasicStructs/Call_BasicStructs.h"
#include "ANOGS/Call_IL2Cpp.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_additional.h"
#include "ImGui/imgui_impl_android.h"
#include "ImGui/imgui_impl_opengl3.h"
#include "ImGui/android_native_app_glue.h"
#include "tools/enc/LicenseTools.h"
#include "tools/MenuKey.h"
#include "Includes/DOSOBF.h"
#include "ImGui/hooks.h"
#include "ImGui/CLASS.h"
#include "ImGui/Icon.h"
#include "tools/things.h"
#include "ImGui/widgets.hpp"
#include "ImGui/Iconcpp.h"
#include "ImGui/minecraft_font.h"
#include "ImGui/stb_image.h"
#include "ImGui/LOGONIDOS.h"
#include "Hacks/Hacks.h"
#include "Hacks/Feature/WeaponCategoryConfig.h"


static CustomWidgets g_widgets;
#include "Hacks/Feature/SkinUnlock.h"


struct TextureInfo { 
    ImTextureID textureId; 
    int w, h; 
};

TextureInfo LogoNew;
ImFont* LOGIN = nullptr;
ImFont* F48 = nullptr;
ImFont* Crozn = nullptr;
static bool checked = false;
static bool isToggled = false; 

TextureInfo CreateTexture(const unsigned char* buf, int len) {
    TextureInfo image = {0,0,0};
    int channels;
    unsigned char* data = stbi_load_from_memory(buf, len, &image.w, &image.h, &channels, 4);
    if (!data) return image;
    
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.w, image.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
    
    image.textureId = (ImTextureID)(uintptr_t)tex;
    return image;
}

void InitTexture() { 
    LogoNew = CreateTexture(dos_data, sizeof(dos_data)); 
}

void DrawAndTouch(ImTextureID textureID, ImVec2& position, float radius, int opacity, bool& isToggled) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 min = ImVec2(position.x - radius, position.y - radius);
    ImVec2 max = ImVec2(position.x + radius, position.y + radius);
    bool isHovered = ImGui::IsMouseHoveringRect(min, max);

    if (isHovered && ImGui::IsMouseDown(0)) {
        ImVec2 mouseDelta = ImGui::GetIO().MouseDelta;
        position.x += mouseDelta.x; 
        position.y += mouseDelta.y; 
    }

    drawList->AddImage(textureID, min, max, ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, opacity));

    if (ImGui::IsMouseReleased(0) && isHovered)
        isToggled = !isToggled; 
}



EGLBoolean (*orig_eglSwapBuffers)(...);

EGLBoolean _eglSwapBuffers (EGLDisplay dpy, EGLSurface surface) {
eglQuerySurface(dpy, surface, EGL_WIDTH, &glWidth);
eglQuerySurface(dpy, surface, EGL_HEIGHT, &glHeight);
if (!g_Initialized) {
    ImGui::CreateContext();
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Your exact colors
    colors[ImGuiCol_WindowBg] = ImVec4(0.07f, 0.08f, 0.10f, 0.96f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.09f, 0.10f, 0.12f, 0.55f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.09f, 0.11f, 0.98f);
    colors[ImGuiCol_Border] = ImVec4(1.f, 1.f, 1.f, 0.08f);
    colors[ImGuiCol_FrameBg] = ImVec4(1.f, 1.f, 1.f, 0.05f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(1.f, 1.f, 1.f, 0.08f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.78f, 0.92f, 0.35f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.07f, 0.08f, 0.10f, 1.0f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.07f, 0.08f, 0.10f, 1.0f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(1.f, 1.f, 1.f, 0.06f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.55f, 0.62f, 0.68f, 0.55f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.35f, 0.82f, 0.95f, 0.55f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.35f, 0.82f, 0.95f, 0.85f);
    colors[ImGuiCol_Header] = ImVec4(0.25f, 0.78f, 0.92f, 0.18f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.78f, 0.92f, 0.28f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.25f, 0.78f, 0.92f, 0.40f);
    colors[ImGuiCol_Text] = ImVec4(0.93f, 0.95f, 0.97f, 1.0f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.55f, 0.58f, 0.62f, 1.0f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.10f, 0.12f, 0.14f, 1.0f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.35f, 0.82f, 0.95f, 1.0f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.55f, 0.90f, 1.0f, 1.0f);
    colors[ImGuiCol_Button] = ImVec4(1.f, 1.f, 1.f, 0.04f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(1.f, 1.f, 1.f, 0.08f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.25f, 0.78f, 0.92f, 0.28f);
    colors[ImGuiCol_Separator] = ImVec4(1.f, 1.f, 1.f, 0.08f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.35f, 0.82f, 0.95f, 0.45f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.35f, 0.82f, 0.95f, 0.80f);
    colors[ImGuiCol_Tab] = ImVec4(0.f, 0.f, 0.f, 0.f);
    colors[ImGuiCol_TabHovered] = ImVec4(1.f, 1.f, 1.f, 0.06f);
    colors[ImGuiCol_TabActive] = ImVec4(0.25f, 0.78f, 0.92f, 0.16f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.f, 0.f, 0.f, 0.f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(1.f, 1.f, 1.f, 0.04f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.35f, 0.82f, 0.95f, 0.45f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 0.78f, 0.92f, 0.28f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.45f);

    style.WindowRounding = 16.0f;
    style.ChildRounding = 12.0f;
    style.FrameRounding = 10.0f;
    style.PopupRounding = 12.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding = 8.0f;
    style.TabRounding = 10.0f;
    style.WindowPadding = ImVec2(16.0f, 14.0f);
    style.FramePadding = ImVec2(12.0f, 8.0f);
    style.ItemSpacing = ImVec2(10.0f, 8.0f);
    style.ItemInnerSpacing = ImVec2(8.0f, 6.0f);
    style.FrameBorderSize = 0.0f;
    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.ScrollbarSize = 22.0f;
    style.GrabMinSize = 36.0f;
    style.AntiAliasedLines = false;
    style.AntiAliasedLinesUseTex = false;

    ImGuiIO &io = ImGui::GetIO();
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    io.IniFilename = nullptr;

    ImGui_ImplAndroid_Init();
    ImGui_ImplOpenGL3_Init("#version 300 es");

    InitTexture();
ImFontConfig font_cfg;
font_cfg.FontDataOwnedByAtlas = false;
font_cfg.PixelSnapH = false;
font_cfg.OversampleH = 2;
font_cfg.OversampleV = 1;
font_cfg.SizePixels = 20.0f;
LOGIN = io.Fonts->AddFontFromMemoryTTF((void*)minecraft_font_data, (int)minecraft_font_size, 20.0f, &font_cfg);
font_cfg.SizePixels = 32.0f;
F48 = io.Fonts->AddFontFromMemoryTTF((void*)minecraft_font_data, (int)minecraft_font_size, 32.0f, &font_cfg);
font_cfg.SizePixels = 22.0f;
Crozn = io.Fonts->AddFontFromMemoryTTF((void*)minecraft_font_data, (int)minecraft_font_size, 22.0f, &font_cfg);
static const ImWchar icons_ranges[] = { 0xf000, 0xf8ff, 0 };
ImFontConfig icons_config;
icons_config.MergeMode = true;
icons_config.PixelSnapH = true;

io.Fonts->AddFontFromMemoryCompressedTTF(
    font_awesome_data,
    font_awesome_size,
    24.0f,        
    &icons_config,
    icons_ranges
);

io.Fonts->Build();

    g_Initialized = true;
}




ImGuiIO* io = &ImGui::GetIO();
ImGui_GetTouch(io, glHeight);
ImGui_ImplOpenGL3_NewFrame();
ImGui_ImplAndroid_NewFrame(glWidth, glHeight);
ImGui::NewFrame();
DrawESP(ImGui::GetBackgroundDrawList());// ESP AIM THREAD


if (checkboxStates["HAND::CAM"]) {
    isToggled = false;
}


if (!checkboxStates["HAND::CAM"]) 
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 22.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.06f, 0.07f, 0.09f, 0.88f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.f, 1.f, 1.f, 0.08f));

    if (ImGui::Begin("LogoOverlay", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_AlwaysAutoResize))
    {
        
        static float fps = 0.0f;
        static float fpsSmoothing = 0.9f;
        float frameFps = 1.0f / ImGui::GetIO().DeltaTime;
        fps = fps * fpsSmoothing + frameFps * (1.0f - fpsSmoothing);

        float size = 40.0f;
        float radius = size * 0.5f;
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        
        if (ImGui::InvisibleButton("logo_btn", ImVec2(size, size))) {
            isToggled = !isToggled;
        }

        draw_list->AddCircleFilled(ImVec2(pos.x + radius, pos.y + radius), radius + 2.0f, IM_COL32(56, 210, 242, 40), 24);
        draw_list->AddImageRounded(
            (void*)(intptr_t)LogoNew.textureId,
            pos,
            ImVec2(pos.x + size, pos.y + size),
            ImVec2(0, 0),
            ImVec2(1, 1),
            IM_COL32_WHITE,
            radius
        );

        float textX = pos.x + size + 10.0f;
        float textY = pos.y + 2.0f;
        ImGui::SetCursorScreenPos(ImVec2(textX, textY));
        ImGui::BeginGroup();
        ImGui::TextColored(ImVec4(0.93f, 0.95f, 0.97f, 1.f), "FPS %.0f", fps);
        ImGui::SetWindowFontScale(0.78f);
        ImGui::TextColored(ImVec4(0.35f, 0.82f, 0.95f, 1.f), DOS("Accel"));
        ImGui::SetWindowFontScale(1.0f);
        ImGui::EndGroup();
    }
    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}
///°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°MAIN MENU= OPTIMIZED 
if (isToggled)
{
   
const float sw = glWidth > 0 ? (float)glWidth : 1080.0f;
const float sh = glHeight > 0 ? (float)glHeight : 1920.0f;
const float menuW = sw * 0.94f;
const float menuH = sh * 0.88f;
float uiScale = sh / 1080.0f;
if (uiScale < 0.90f) uiScale = 0.90f;
if (uiScale > 1.55f) uiScale = 1.55f;
ImVec2 center2 = ImGui::GetMainViewport()->GetCenter();
ImGui::SetNextWindowPos(center2, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
ImGui::SetNextWindowSize(ImVec2(menuW, menuH), ImGuiCond_Always);
ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 18.0f);
ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 14.0f));
ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.055f, 0.062f, 0.078f, 0.96f));
ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.35f, 0.82f, 0.95f, 0.22f));
ImGui::PushFont(Crozn);
if (ImGui::Begin(" ", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize))
{
    ImGui::SetWindowFontScale(uiScale);
    if (DrawMacOSHeaderButtons()) isToggled = false;
    ImGui::Dummy(ImVec2(0, 6));
    
    ImGui::BeginChild("##ScrollablePage", ImVec2(-1, -1), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    
    static int tab = 1;
    
    ImGui::Columns(2);
    ImGui::SetColumnOffset(1, menuW * 0.30f); 
    
    {
        float buttonWidth = menuW * 0.26f;
        float buttonHeight = sh * 0.068f;
        if (buttonHeight < 48.0f) buttonHeight = 48.0f;
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6.0f));
        
        struct TabButton {
            const char* icon;
            const char* name;
            int id;
            float anim;
        };
        
        static TabButton buttons[] = {
            { ICON_FA_EYE,        "ESP",    1, 0.0f },
            { ICON_FA_CROSSHAIRS, "Aim",    2, 0.0f },
            { ICON_FA_MICROCHIP,  "Memory", 3, 0.0f },
            { ICON_FA_BULLSEYE,   "Guns",   7, 0.0f },
            { ICON_FA_TSHIRT,     "Skin",   10, 0.0f },
            { ICON_FA_COGS,       "Setup",  5, 0.0f },
            { ICON_FA_SHIELD,     "Shield", 8, 0.0f },
        };
        
        ImGuiIO& io = ImGui::GetIO();
        float speed = 8.0f * io.DeltaTime; 
        
        auto Lerp = [](float a, float b, float t) {
            return a + (b - a) * t;
        };
        
        for (auto& b : buttons) {
            if (!kCallMonitor && b.id == 8) continue;
            b.anim = Lerp(b.anim, (tab == b.id) ? 1.0f : 0.0f, speed);
            
            ImVec4 iconColor = ImVec4(
                0.62f + 0.10f * b.anim,
                0.66f + 0.16f * b.anim,
                0.70f + 0.25f * b.anim,
                1.0f
            );
            
            ImVec4 btnColor = ImVec4(1.f, 1.f, 1.f, 0.03f);
            ImVec4 btnHover = ImVec4(1.f, 1.f, 1.f, 0.07f);
            ImVec4 btnActive = ImVec4(0.25f, 0.78f, 0.92f, 0.22f);
            
            if (tab == b.id) {
                btnColor = ImVec4(0.25f, 0.78f, 0.92f, 0.16f);
                btnHover = ImVec4(0.25f, 0.78f, 0.92f, 0.24f);
                btnActive = ImVec4(0.25f, 0.78f, 0.92f, 0.32f);
            }
            
            ImGui::PushStyleColor(ImGuiCol_Button, btnColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btnHover);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, btnActive);
            ImGui::PushStyleColor(ImGuiCol_Text, iconColor);
            char tabLabel[64];
            snprintf(tabLabel, sizeof(tabLabel), "%s   %s##tab%d", b.icon, b.name, b.id);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
            if (ImGui::Button(tabLabel, ImVec2(buttonWidth, buttonHeight))) tab = b.id;
            ImGui::PopStyleVar();
            
            ImGui::PopStyleColor(4);
            ImGui::Spacing();
        }
        
        ImGui::PopStyleVar();
        
        ImGui::NextColumn();
        ImGui::BeginChild("right_content", ImVec2(0, 0), true);
        MenuKeyEnsure();
        if (!MenuKeyUnlocked()) {
            DrawMenuKeyBox();
            ImGui::TextUnformatted("Menu locked");
        } else {
        ImGui::Separator();
        
        if (tab == 1) {
            if (checkboxStates["ESP ENABLE"]) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.72f, 0.84f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.05f, 0.08f, 0.10f, 1.0f));
                if (ImGui::Button("ESP  ON", ImVec2(-1, 52))) {
                    checkboxStates["ESP ENABLE"] = false;
                }
                ImGui::PopStyleColor(2);
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.f, 1.f, 1.f, 0.06f));
                if (ImGui::Button("ESP  OFF", ImVec2(-1, 52))) {
                    checkboxStates["ESP ENABLE"] = true;
                }
                ImGui::PopStyleColor();
            }
            
            ImGui::Columns(2);
            g_widgets.checkbox("Line", &checkboxStates["ESP LINE"]);
            g_widgets.checkbox("2D Box", &checkboxStates["ESP BOX"]);
            g_widgets.checkbox("Name", &checkboxStates["ESP NAME"]);
            g_widgets.checkbox("Vehicle", &checkboxStates["ESP::VEHICLE"]);
            g_widgets.checkbox("Weapon", &checkboxStates["ESP::WEAPON"]);
            g_widgets.checkbox("3D Box", &checkboxStates["3D::BOX"]);
            g_widgets.checkbox("Armor", &checkboxStates["ESP ARMOR"]);
            
            ImGui::NextColumn();
            g_widgets.checkbox("Alert", &checkboxStates["ESP ALERT 360"]);
            g_widgets.checkbox("Health", &checkboxStates["ESP HEALTH"]);
            g_widgets.checkbox("Distance", &checkboxStates["ESP DISTANCE"]);
            g_widgets.checkbox("Circle", &checkboxStates["ESP CIRCLE"]);
            g_widgets.checkbox("Skeleton", &checkboxStates["SKELETON"]);
            g_widgets.checkbox("Enemy Counter", &checkboxStates["ENEMY::COUNTER"]);
            ImGui::Columns(1);
            
            ImGui::Spacing();
            ImGui::Columns(2, nullptr, false);
            
            ImGui::BeginGroupPanel("Line Position", ImVec2(-1, 0));
            static const char* linePositions[] = { "Bottom", "Up" };
            for (int i = 0; i < IM_ARRAYSIZE(linePositions); i++) {
                ImGui::RadioButton(linePositions[i], &dropdownStates["#LINE_POS"], i);
                if (i < IM_ARRAYSIZE(linePositions) - 1)
                    ImGui::SameLine();
            }
            ImGui::EndGroupPanel();
            
            ImGui::NextColumn();
            ImGui::BeginGroupPanel("2D Box Style", ImVec2(-1, 0));
            static const char* boxStyles[] = { "Outline", "Filled" };
            for (int i = 0; i < IM_ARRAYSIZE(boxStyles); i++) {
                ImGui::RadioButton(boxStyles[i], &dropdownStates["#BOX_STYLE"], i);
                if (i < IM_ARRAYSIZE(boxStyles) - 1)
                    ImGui::SameLine();
            }
            ImGui::EndGroupPanel();
        }
        
        if (tab == 2)
        {
            ImGui::Text(ICON_FA_EXCLAMATION_CIRCLE "  Aim");
            ImGui::Separator();
            
            ImGui::BeginGroupPanel("", ImVec2(-1, 0));
            g_widgets.checkbox("SILENT AIM", &checkboxStates["AIMBOT 360"]);
            g_widgets.checkbox("Ignore Wall", &checkboxStates["DISABLE::VIS"]);
            g_widgets.checkbox("Ignore Knocked Enemy", &checkboxStates["IGNORE KNOCKED"]);
            ImGui::EndGroupPanel();
            
            ImGui::Spacing();
            
            ImGui::BeginGroupPanel("TUNING", ImVec2(-1, 0));
            SLIDER("AIM SMOOTHNESS", &seekbarValues["AIM SMOOTHNESS"], 1.0f, 100.0f);
            SLIDER("AIM SPEED", &seekbarValues["AIM SPEED"], 1.0f, 100.0f);
            SLIDER("AIM FOV", &seekbarValues["FOV500"], 1.0f, 500.0f);
            SLIDER("[Change Recoil To Current Gun]", &seekbarValues["NO::RECOIL"], 0.0f, 100.0f);
            SLIDER("[change ADS To Current Gun]", &seekbarValues["FAST::SCOPE"], 0.0f, 100.0f);
            SLIDER("[change Reload To Current Gun]", &seekbarValues["NO::RELOAD"], 0.0f, 100.0f);
            SLIDER("[change Spread To Current Gun]", &seekbarValues["NO::SPREAD"], 0.0f, 100.0f);
            SLIDER("Aim Range", &seekbarValues["MAXDIST"], 0.0f, 200.0f);
            ImGui::EndGroupPanel();
            
            ImGui::Spacing();
            
            ImGui::BeginGroupPanel("Aiming Location", ImVec2(-1, 0));
            static const char* targets[] = { "BODY", "HEAD", "CHEST", "FEET" };
            g_widgets.dropdown("##target", &dropdownStates["#TARG"], targets, 4);
            ImGui::EndGroupPanel();
            
            ImGui::Spacing();
            
            ImGui::BeginGroupPanel("Aim Priority", ImVec2(-1, 0));
            static const char* distances[] = { "NEAREST", "FOV" };
            g_widgets.dropdown("##priority", &dropdownStates["#DIST"], distances, 2);
            ImGui::EndGroupPanel();
            
            ImGui::Spacing();
            
            ImGui::BeginGroupPanel("Trigger Condition", ImVec2(-1, 0));
            static const char* triggers[] = { "NONE", "SHOOTING", "SCOPING", "HYBRID" };
            g_widgets.dropdown("##trigger", &dropdownStates["#TRIG"], triggers, 4);
            ImGui::EndGroupPanel();
            
            ImGui::Spacing();
        }
        
        if (tab == 3)
        {
            ImGui::Text(ICON_FA_MICROCHIP "  Memory");
            ImGui::Separator();
            
            ImGui::BeginGroupPanel("", ImVec2(-1, 0));
            ImGui::Columns(2, nullptr, false);
            g_widgets.checkbox("Wallhack", &checkboxStates["WALLHACK"]);
            ImGui::NextColumn();
            g_widgets.checkbox("Wall Shot", &checkboxStates["WALL::SHOT"]);
            ImGui::NextColumn();
            g_widgets.checkbox("Maphack", &checkboxStates["ADVANCE::UAV"]);
            ImGui::NextColumn();
            g_widgets.checkbox("No Parachute", &checkboxStates["NO::PARACHUTE"]);
            ImGui::NextColumn();
            g_widgets.checkbox("Fast Swim", &checkboxStates["FAST::SWIM"]);
            ImGui::NextColumn();
            g_widgets.checkbox("Extend Hitbox", &checkboxStates["HITBOX"]);
            ImGui::NextColumn();
            g_widgets.checkbox("No Shake", &checkboxStates["NO::SHAKE"]);
            ImGui::NextColumn();
            g_widgets.checkbox("Disable Crouch", &checkboxStates["NO CROUCH"]);
            ImGui::NextColumn();
            g_widgets.checkbox("Misdirection Effect", &checkboxStates["AUTO::MISDIRECTION"]);
            ImGui::Columns(1);
            ImGui::EndGroupPanel();
            
            ImGui::Spacing();
            
            ImGui::BeginGroupPanel("Movement Adjustments", ImVec2(-1, 0));
            ImGui::Spacing();
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
            SLIDER("Aim Assist", &seekbarValues["AIM::ASSIST"], 0.0f, 100.0f);
            SLIDER("Fov BR", &seekbarValues["FOV"], 0.0f, 200.0f);
            SLIDER("Fov MP", &seekbarValues["FOV1"], 0.0f, 200.0f);
            SLIDER("Snowb Speed", &seekbarValues["SizeSnow"], 0.0f, 70.0f); 
            SLIDER("Loot Speed", &seekbarValues["FAST::LOOT"], 0.0f, 100.0f);
            SLIDER("Game Speed", &seekbarValues["SPEED_HACK"], 0.0f, 70.0f); 
            ImGui::PopStyleVar();
            ImGui::EndGroupPanel();
        }
        
        if (tab == 5)
        {
            ImGui::Spacing();
            
            ImGui::BeginChild("##SettingsContent", ImVec2(0, 0), true);
            {
                DrawMenuKeyBox();
                ImGui::Spacing();
                ImGui::BeginGroupPanel("Performance", ImVec2(-1, 0));
                ImGui::Columns(2, nullptr, false);
                g_widgets.checkbox("120 FPS", &checkboxStates["FRAME120"]);
                ImGui::NextColumn();
                g_widgets.checkbox("Super low graphics", &checkboxStates["LOWGFX"]);
                ImGui::NextColumn();
                g_widgets.checkbox("Hand Cam Mode", &checkboxStates["HAND::CAM"]);
                ImGui::NextColumn();
                g_widgets.checkbox("Disable VisCheck", &checkboxStates["DISABLE_VISCHECK"]);
                
                ImGui::Columns(1);
                ImGui::Spacing();
                SLIDER("Low level", &seekbarValues["LOW::GFX"], 0.0f, 500.0f);
                ImGui::EndGroupPanel();

                ImGui::Spacing();
                ImGui::BeginGroupPanel("Graphics level", ImVec2(-1, 0));
                ImGui::TextUnformatted("Very Low  1    Low  2    Mid  3");
                ImGui::TextUnformatted("High  4    Very High  5    Extreme  6");
                ImGui::Spacing();
                g_widgets.checkbox("Force Very Low", &checkboxStates["GFX::LEVEL"]);
                ImGui::EndGroupPanel();

                ImGui::Spacing();
                
                ImGui::BeginGroupPanel("Esp Colors", ImVec2(-1, 0));
                static ImVec4 enemyVisibleColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); 
                static ImVec4 botVisibleColor   = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);  
                static ImVec4 notVisibleColor   = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);  
                static ImVec4 knockedColor      = ImVec4(1.0f, 0.65f, 0.0f, 1.0f);
                static ImVec4 defaultEspColor   = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

                ImGui::ColorEdit4("Enemy",     (float*)&enemyVisibleColor, ImGuiColorEditFlags_NoInputs);
                ImGui::SameLine(); 
                ImGui::ColorEdit4("Bot",       (float*)&botVisibleColor,   ImGuiColorEditFlags_NoInputs);
                ImGui::SameLine();
                ImGui::ColorEdit4("Hidden",    (float*)&notVisibleColor,   ImGuiColorEditFlags_NoInputs);
                ImGui::SameLine();
                ImGui::ColorEdit4("Knocked",   (float*)&knockedColor,      ImGuiColorEditFlags_NoInputs);
                ImGui::ColorEdit4("Default",   (float*)&defaultEspColor,   ImGuiColorEditFlags_NoInputs);

                for (auto& color : {&enemyVisibleColor, &botVisibleColor, &notVisibleColor, &knockedColor, &defaultEspColor})
                    color->w = 1.0f;

                colorStates = {
                    {"ENEMY", enemyVisibleColor}, {"BOT", botVisibleColor}, {"HIDDEN", notVisibleColor},
                    {"KNOCKED", knockedColor}, {"DEFAULT", defaultEspColor}
                };
                ImGui::EndGroupPanel();
            }
            ImGui::EndChild();
        }
        if (tab == 7)
        {
            ImGui::Spacing();
            WeaponCfg::DrawTab();
        }
        if (tab == 10)
        {
            ImGui::Spacing();
            DrawSkinTab();
        }
        if (tab == 8)
        {
            ImGui::Spacing();
            DrawAnogsTab();
        }
        }
        ImGui::EndChild();
        ImGui::Columns(1);
    }
    
    {
        ImGuiIO& tio = ImGui::GetIO();
        static float scrollVel = 0.f;
        if (ImGui::IsWindowHovered() && ImGui::IsMouseDragging(0, 3.0f) && !ImGui::IsAnyItemActive()) {
            scrollVel = -tio.MouseDelta.y * 1.2f;
            ImGui::SetScrollY(ImGui::GetScrollY() + scrollVel);
        } else if (!tio.MouseDown[0] && fabsf(scrollVel) >= 0.5f) {
            ImGui::SetScrollY(ImGui::GetScrollY() + scrollVel);
            scrollVel *= 0.86f;
        } else if (!tio.MouseDown[0]) {
            scrollVel = 0.f;
        }
    }
    ImGui::EndChild();
}
ImGui::PopFont();
ImGui::End();
ImGui::PopStyleColor(2);
ImGui::PopStyleVar(3);
}

if (g_Initialized) {
    ApplyFpsBoost();
    ApplyGraphicsLevel();
    ApplyLowGfx();
    SkinTick();
}
ImGui::Render();
ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
if (clearMousePos && !io->MouseDown[0]) {
    io->MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
    clearMousePos = false;
}
return orig_eglSwapBuffers(dpy, surface);
}



void Init_Thread(){
    while (!m_Il2Cpp) {
        m_Il2Cpp = getAbsoluteAddress("libunity.so", 0);  
        sleep(1);      
    }
    
    INIT_OFFSETS();
    hooking();
    SkinInstallHooks();
    
    DOSHOOK((void*)dlsym(RTLD_NEXT, DOS("eglSwapBuffers")), (void*)_eglSwapBuffers, (void**)&orig_eglSwapBuffers);
    started = true;
}

__attribute__((constructor))
void native_Init() {
    std::thread(Init_Thread).detach();
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    MenuKeyBindVm(vm);
    JNIEnv *env;
    vm->GetEnv((void **) &env, JNI_VERSION_1_6);
    return JNI_VERSION_1_6;
}
