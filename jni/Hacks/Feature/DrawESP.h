#include <atomic>
#include <unordered_map> 
#include <cmath>       
#include "ImGui/skeleton.h"
#include <array>
#include <cstdio>
#include <string>
#pragma once
std::unordered_map<std::string, int> dropdownStates;
std::atomic<bool> gMatchLive{false};

void AddTextWithShadow(ImDrawList* draw, ImFont* font, float fontSize, ImVec2 pos, ImU32 textColor, const char* text);
static void DrawTag(ImDrawList* draw, float centerX, float top, const char* text, ImU32 textCol, ImU32 edge);

static std::string ReadGameString(String* text) {
    if (!Tools::IsPtrValid(text) || text->length <= 0 || text->length > 48) return {};
    const char* utf = text->CString();
    if (!utf || !utf[0]) return {};
    return utf;
}

static std::string WeaponItemName(uintptr_t weaponConf, int itemId) {
    static std::unordered_map<int, std::string> names;
    auto found = names.find(itemId);
    if (found != names.end() && found->second != "Weapon") return found->second;
    std::string name;
    if (Tools::IsPtrValid((void*)weaponConf) && Class_WeaponConf_get_Name) {
        auto fn = (String* (*)(uintptr_t))Class_WeaponConf_get_Name;
        name = ReadGameString(fn(weaponConf));
    }
    if (name.empty() && itemId > 0 && Class_ItemConfig_GetItemName) {
        auto fn = (String* (*)(int, int))Class_ItemConfig_GetItemName;
        name = ReadGameString(fn(itemId, 0));
    }
    if (name.empty()) name = "Weapon";
    if (names.size() > 256) names.clear();
    names[itemId] = name;
    return name;
}

static void DrawWeaponMark(ImDrawList* draw, Camera* camera, const Vector3& myPos, uintptr_t entity, int& drawn) {
    if (drawn >= 40 || !Tools::IsPtrValid((void*)entity)) return;
    if (*(unsigned char*)(entity + 0x6C) == 0) return;
    uintptr_t weaponConf = *(uintptr_t*)(entity + 0xE0);
    int configType = *(int*)(entity + 0xA0);
    int pickType = *(int*)(entity + 0x58);
    if (!Tools::IsPtrValid((void*)weaponConf) && configType != 1 && pickType != 1) return;
    Vector3 root = *(Vector3*)(entity + 0x60);
    Transform* transform = ((Component*)entity)->get_transform();
    if (Tools::IsPtrValid(transform)) root = transform->get_position();
    if (root.x == 0.0f && root.y == 0.0f && root.z == 0.0f) return;
    float distance = Vector3::Distance(myPos, root);
    if (distance > 400.0f) return;
    Vector3 screen = camera->WorldToScreenPoint(root);
    if (screen.z <= 0.0f) return;
    float x = screen.x;
    float y = glHeight - screen.y;
    float startY = dropdownStates["#LINE_POS"] == 0 ? (float)glHeight - 3.0f : 20.0f;
    ImVec2 from(glWidth * 0.5f, startY);
    ImVec2 to(x, y);
    draw->AddLine(from, to, IM_COL32(0, 0, 0, 140), 2.4f);
    draw->AddLine(from, to, IM_COL32(232, 186, 78, 210), 1.15f);
    draw->AddCircleFilled(to, 3.2f, IM_COL32(232, 186, 78, 240), 10);
    char line[80];
    snprintf(line, sizeof(line), "%s  %dm", WeaponItemName(weaponConf, *(int*)(entity + 0x5C)).c_str(), (int)distance);
    DrawTag(draw, x, y - 22.0f, line, IM_COL32(255, 226, 160, 245), IM_COL32(232, 186, 78, 160));
    drawn++;
}

static void DrawDroppedWeapons(ImDrawList* draw, Camera* camera, const Vector3& myPos) {
    if (!checkboxStates["ESP ENABLE"] || !checkboxStates["ESP::WEAPON"]) return;
    MatchGame* match = GamePlay::get_MatchGame();
    if (!Tools::IsPtrValid(match)) return;
    uintptr_t manager = *(uintptr_t*)((uintptr_t)match + 0x320);
    if (!Tools::IsPtrValid((void*)manager) && Class_DropMgr_get_Instance) {
        auto instance = (uintptr_t (*)())Class_DropMgr_get_Instance;
        manager = instance();
    }
    if (!Tools::IsPtrValid((void*)manager)) return;
    uintptr_t entities = *(uintptr_t*)(manager + 0x118);
    if (!Tools::IsPtrValid((void*)entities)) return;
    uintptr_t values = *(uintptr_t*)(entities + 0x28);
    int touched = *(int*)(entities + 0x30);
    if (!Tools::IsPtrValid((void*)values) || touched <= 0) return;
    int length = *(int*)(values + 0x18);
    if (length <= 0) return;
    if (touched > length) touched = length;
    if (touched > 2000) touched = 2000;
    uintptr_t slots = values + 0x20;
    int drawn = 0;
    for (int i = 0; i < touched && drawn < 40; ++i) {
        uintptr_t entity = *(uintptr_t*)(slots + (uintptr_t)i * 8);
        DrawWeaponMark(draw, camera, myPos, entity, drawn);
    }
}

void AddTextWithShadow(ImDrawList* draw, ImFont* font, float fontSize, ImVec2 pos, ImU32 textColor, const char* text) {
    draw->AddText(font, fontSize, ImVec2(pos.x + 1.0f, pos.y + 1.0f), IM_COL32(0, 0, 0, 220), text);
    draw->AddText(font, fontSize, pos, textColor, text);
}

static ImU32 EspPawnColor(Pawn* pawn) {
    if (pawn && pawn->IsKnocked()) return IM_COL32(255, 156, 64, 235);
    if (pawn && pawn->m_IsBot()) return IM_COL32(168, 214, 224, 225);
    return IM_COL32(86, 214, 242, 240);
}

static void DrawCornerBox(ImDrawList* draw, ImVec2 a, ImVec2 b, ImU32 col) {
    float w = b.x - a.x;
    float h = b.y - a.y;
    float len = w * 0.28f;
    if (len > h * 0.22f) len = h * 0.22f;
    if (len < 5.f) len = 5.f;
    auto arm = [&](ImVec2 o, float sx, float sy) {
        ImVec2 hx(o.x + len * sx, o.y);
        ImVec2 hy(o.x, o.y + len * sy);
        draw->AddLine(o, hx, IM_COL32(0, 0, 0, 200), 3.0f);
        draw->AddLine(o, hy, IM_COL32(0, 0, 0, 200), 3.0f);
        draw->AddLine(o, hx, col, 1.6f);
        draw->AddLine(o, hy, col, 1.6f);
    };
    arm(a, 1.f, 1.f);
    arm(ImVec2(b.x, a.y), -1.f, 1.f);
    arm(ImVec2(a.x, b.y), 1.f, -1.f);
    arm(b, -1.f, -1.f);
}

static float SnapPx(float v) { return floorf(v + 0.5f); }

struct EspSmooth {
    float hx, hy, rx, ry;
    bool on;
};
static std::unordered_map<uintptr_t, EspSmooth> gEspSmooth;

struct EspLabel {
    char text[72];
    int age;
};
static std::unordered_map<uintptr_t, EspLabel> gEspNames;

static void SmoothScreen(uintptr_t id, float& hx, float& hy, float& rx, float& ry) {
    if (gEspSmooth.size() > 96) gEspSmooth.clear();
    float dt = ImGui::GetIO().DeltaTime;
    if (dt < 0.001f) dt = 0.001f;
    if (dt > 0.05f) dt = 0.05f;
    float k = 1.f - expf(-24.f * dt);
    EspSmooth& s = gEspSmooth[id];
    if (!s.on) {
        s.hx = hx; s.hy = hy; s.rx = rx; s.ry = ry; s.on = true;
    } else {
        s.hx += (hx - s.hx) * k;
        s.hy += (hy - s.hy) * k;
        s.rx += (rx - s.rx) * k;
        s.ry += (ry - s.ry) * k;
    }
    hx = SnapPx(s.hx);
    hy = SnapPx(s.hy);
    rx = SnapPx(s.rx);
    ry = SnapPx(s.ry);
}

static const char* EspNameText(Pawn* pawn) {
    if (gEspNames.size() > 96) gEspNames.clear();
    EspLabel& slot = gEspNames[(uintptr_t)pawn];
    if (slot.text[0] && slot.age < 45) {
        slot.age++;
        return slot.text;
    }
    slot.age = 0;
    char tmp[64];
    tmp[0] = 0;
    String* obj = pawn->get_PlayerName();
    if (Tools::IsPtrValid(obj) && obj->length > 0 && obj->length < 48) {
        const char* utf = obj->CString();
        if (utf && utf[0]) snprintf(tmp, sizeof(tmp), "%s", utf);
    }
    snprintf(slot.text, sizeof(slot.text), "%s%s", pawn->m_IsBot() ? "BOT " : "", tmp[0] ? tmp : "Player");
    return slot.text;
}

static void DrawTag(ImDrawList* draw, float centerX, float top, const char* text, ImU32 textCol, ImU32 edge) {
    ImVec2 size = ImGui::CalcTextSize(text);
    float x = SnapPx(centerX - size.x * 0.5f);
    float y = SnapPx(top);
    ImVec2 min(x - 6.f, y - 2.f);
    ImVec2 max(x + size.x + 6.f, y + size.y + 2.f);
    draw->AddRectFilled(min, max, IM_COL32(6, 8, 12, 210), 3.f);
    draw->AddRect(min, max, edge, 3.f, 0, 1.f);
    draw->AddText(ImVec2(x, y), textCol, text);
}


// ---------------- GetClosestTarget ----------------
uintptr_t GetClosestTarget() {
    uintptr_t result = 0;
    float MAX_AIM_DISTANCE = seekbarValues["MAXDIST"];  
    float MinDist = MAX_AIM_DISTANCE;

    auto MatchGame = GamePlay::get_MatchGame();
    if (!MatchGame) return 0;

    Pawn* localPawn = GamePlay::get_LocalPawn();
    if (!localPawn) return 0;

    Vector3 MyPos = localPawn->get_LastPawnPos();

    List<Pawn*>* EnemyPawns = MatchGame->EnemyPawns();
    if (!EnemyPawns) return 0;

    Pawn** Items = EnemyPawns->getItems();
    if (!Items) return 0;

    for (int i = 0; i < EnemyPawns->getSize(); i++) {
        Pawn* pPawn = Items[i];
        if (!pPawn) continue;
        if (!pPawn->m_IsAlive()) continue;

        Transform* mesh = pPawn->m_Mesh();
        if (!mesh) continue;

        Vector3 RootPos = mesh->get_position();
        float Distance = Vector3::Distance(MyPos, RootPos);

        if (Distance < MinDist && Distance <= MAX_AIM_DISTANCE) {
            result = (uintptr_t)pPawn;
            MinDist = Distance;
        }
    }

    return result;
}






Vector3 GetTargetWorldPosition(Pawn* targetPawn, int targetMode, const Vector3& camPos = Vector3{0,0,0}, const Vector3& aimDir = Vector3{0,0,1}) {
    if (!targetPawn) return Vector3{0, 0, 0};

    Transform* headBone = targetPawn->m_HeadBone();
    if (!headBone) return Vector3{0, 0, 0};

    Vector3 position = headBone->get_position();
    
    if (targetMode == 0) {
        auto normalizeDir = [&](Vector3 pos) -> Vector3 {
            Vector3 d = pos - camPos;
            float mag = sqrtf(d.x * d.x + d.y * d.y + d.z * d.z);
            if (mag > 0.0001f) {
                d.x /= mag;
                d.y /= mag;
                d.z /= mag;
            }
            return d;
        };

        Vector3 dirHead = normalizeDir(position);
        float bestDot = dirHead.x * aimDir.x + dirHead.y * aimDir.y + dirHead.z * aimDir.z;
        Vector3 bestPos = position;

        const float offsets[] = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f, 1.1f};
        for (float off : offsets) {
            Vector3 pos = position;
            pos.y -= off;
            Vector3 dir = normalizeDir(pos);
            float dot = dir.x * aimDir.x + dir.y * aimDir.y + dir.z * aimDir.z;
            if (dot > bestDot) {
                bestDot = dot;
                bestPos = pos;
            }
        }
        return bestPos;
    } else {
        static constexpr std::array<float, 5> HEIGHT_OFFSETS = {0.0f, 0.0f, 0.2f, 0.4f, 0.9f};
        if (targetMode >= 0 && targetMode < static_cast<int>(HEIGHT_OFFSETS.size())) {
            position.y -= HEIGHT_OFFSETS[targetMode];
        }
        return position;
    }
}


uintptr_t GetInsideFOVTarget() {
    uintptr_t result = 0;
    float maxPixelDist = FLT_MAX;
    
    float fovRadius = seekbarValues["FOV500"];
    if (fovRadius <= 0) return 0;
    
    float maxWorldDist = seekbarValues["MAXDIST"];
    if (maxWorldDist <= 0) maxWorldDist = FLT_MAX;

    auto matchGame = MatchGame::get_MatchGame();
    if (!matchGame) return 0;

    Pawn* localPawn = GamePlay::get_LocalPawn();
    if (!localPawn) return 0;

    List<Pawn*>* enemyPawns = matchGame->EnemyPawns();
    if (!enemyPawns || enemyPawns->getSize() == 0) return 0;

    Pawn** items = enemyPawns->getItems();
    if (!items) return 0;

    Camera* mainCam = Camera::get_main();
    if (!mainCam) return 0;
    
    Transform* camTransform = mainCam->get_transform();
    if (!camTransform) return 0;
    
    Vector3 camPos = camTransform->get_position();
    Vector3 aimDir = camTransform->get_forward();
    Vector3 myPos = localPawn->get_LastPawnPos();
    
    Vector2 screenCenter((float)get_width() / 2, (float)get_height() / 2);
    int targetMode = dropdownStates["#TARG"];
    
    float maxWorldDistSquared = maxWorldDist * maxWorldDist;
    float fovRadiusSquared = fovRadius * fovRadius;
    bool skipVisibility = checkboxStates["DISABLE::VIS"];

    for (int i = 0; i < enemyPawns->getSize(); i++) {
        Pawn* enemyPawn = items[i];
        if (!enemyPawn) continue;
        if (!enemyPawn->m_IsAlive()) continue;
        
        Vector3 targetWorldPos = GetTargetWorldPosition(enemyPawn, targetMode, camPos, aimDir);
        if (targetWorldPos.x == 0 && targetWorldPos.y == 0 && targetWorldPos.z == 0) continue;
        
        Vector3 diff = targetWorldPos - myPos;
        float distSquared = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
        if (distSquared > maxWorldDistSquared) continue;
        
        if (!skipVisibility && !enemyPawn->isVisible()) continue;
        
        Vector3 targetScreen = mainCam->WorldToScreenPoint(targetWorldPos);
        if (targetScreen.z < 0.1f) continue;
        
        float dx = targetScreen.x - screenCenter.x;
        float dy = targetScreen.y - screenCenter.y;
        float pixelDistSquared = dx * dx + dy * dy;
        
        if (pixelDistSquared <= fovRadiusSquared && pixelDistSquared < maxPixelDist * maxPixelDist) {
            result = (uintptr_t)enemyPawn;
            maxPixelDist = sqrtf(pixelDistSquared);
        }
    }

    return result;
}

void drawLine(long color, float thickness, Vector3 start, Vector3 end) {
    ImGui::GetBackgroundDrawList()->PathLineTo(ImVec2(start.x, start.y));
    ImGui::GetBackgroundDrawList()->PathLineTo(ImVec2(end.x, end.y));
    ImGui::GetBackgroundDrawList()->PathStroke(color, 0, thickness);
}

struct CircleEffect {
    float timer = 0;
};

std::unordered_map<void*, CircleEffect> circleMap;

void Draw3DCircle(ImDrawList* draw, void* player_ptr, Vector3 pos, int screenHeight, float deltaTime, Camera* mainCamera = nullptr) {
    if (circleMap.size() > 128) circleMap.clear();
    auto& c = circleMap[player_ptr];
    
    c.timer += deltaTime;
    float t = fmodf(c.timer, 1.2f) / 1.2f;
    
    float alpha = (1.0f - fabsf(2.0f * t - 1.0f)) * 255.0f;
    float radius = 0.3f + t * 0.6f;

    if (!mainCamera) mainCamera = Camera::get_main();
    if (!mainCamera) return;
    
    Vector3 base = pos;
    base.y -= 0.05f;
    
    float step = 2 * IM_PI / 8;
    for (float a = 0; a < 2 * IM_PI; a += step) {
        Vector3 p1 = {
            base.x + cosf(a) * radius,
            base.y,
            base.z + sinf(a) * radius
        };
        Vector3 p2 = {
            base.x + cosf(a + step) * radius,
            base.y,
            base.z + sinf(a + step) * radius
        };
        
        Vector3 sp1 = mainCamera->WorldToScreenPoint(p1);
        Vector3 sp2 = mainCamera->WorldToScreenPoint(p2);
        
        if (sp1.z > 0 && sp2.z > 0) {
            draw->AddLine(
                {sp1.x, (float)screenHeight - sp1.y},
                {sp2.x, (float)screenHeight - sp2.y},
                IM_COL32(255, 255, 255, (int)alpha),
                1.0f
            );
        }
    }
}


void Draw3dBox(ImDrawList* draw, uintptr_t player_ptr, Vector3 rootPos, int glHeight, int glWidth, Camera* mainCamera = nullptr) {
    Pawn* pawn = reinterpret_cast<Pawn*>(player_ptr);
    if (!pawn || !pawn->m_IsAlive()) return;

    if (!mainCamera) mainCamera = Camera::get_main();
    if (!mainCamera) return;

    float hw = 0.60f * 0.40f;
    float h = 1.65f;
    
    Vector3 localCorners[8] = {
        {-hw, 0, -hw}, {hw, 0, -hw}, {hw, h, -hw}, {-hw, h, -hw},
        {-hw, 0,  hw}, {hw, 0,  hw}, {hw, h,  hw}, {-hw, h,  hw}
    };

    Vector3 sc[8];
    for (int i = 0; i < 8; i++) {
        sc[i] = mainCamera->WorldToScreenPoint(rootPos + localCorners[i]);
        if (sc[i].z <= 0.0f) return;
        sc[i].y = glHeight - sc[i].y;
    }

    ImU32 color = EspPawnColor(pawn);
    auto edge = [&](int a, int b) {
        draw->AddLine({sc[a].x, sc[a].y}, {sc[b].x, sc[b].y}, IM_COL32(0, 0, 0, 170), 2.4f);
        draw->AddLine({sc[a].x, sc[a].y}, {sc[b].x, sc[b].y}, color, 1.2f);
    };

    edge(0, 1);
    edge(1, 2);
    edge(2, 3);
    edge(3, 0);
    edge(4, 5);
    edge(5, 6);
    edge(6, 7);
    edge(7, 4);
    edge(0, 4);
    edge(1, 5);
    edge(2, 6);
    edge(3, 7);
}

void drawESP__Player(ImDrawList *draw) {
    if (!started) return;
    
    float deltaTime = ImGui::GetIO().DeltaTime;
    Camera* mainCamera = Camera::get_main();
    if (!mainCamera) return;

        if (dropdownStates["#DIST"] == EAim::Crosshair && !checkboxStates["HAND::CAM"]) {
        float fovRadius = seekbarValues["FOV500"] * 1.0f;
        ImVec2 mid(glWidth * 0.5f, glHeight * 0.5f);
        draw->AddCircle(mid, fovRadius, IM_COL32(0, 0, 0, 120), 64, 2.4f);
        draw->AddCircle(mid, fovRadius, IM_COL32(86, 214, 242, 150), 64, 1.2f);
    }

    BaseGame *get_MatchGame = GamePlay::get_MatchGame();
    if (!Tools::IsPtrValid(get_MatchGame)) return;
    
    gMatchLive = true;
    
    Pawn *get_LocalPawn = GamePlay::get_LocalPawn();
    if (!Tools::IsPtrValid(get_LocalPawn)) return;
    
    Vector3 pLocalPawn_rootPos = get_LocalPawn->get_LastPawnPos();
    ApplyWallShot();
    DrawDroppedWeapons(draw, mainCamera, pLocalPawn_rootPos);
           
    List<Pawn *> *EnemyPawns = get_MatchGame->EnemyPawns();
    if (!Tools::IsPtrValid(EnemyPawns) || EnemyPawns->getSize() == 0) return;
    
    int pawnCount = EnemyPawns->getSize();
    if (pawnCount <= 0 || pawnCount > 100) return;
    
    Pawn **pawns = (Pawn **)EnemyPawns->getItems();
    if (!Tools::IsPtrValid(pawns)) return;
    
    int totalEnemies = 0, totalBots = 0;
    const bool espOn = checkboxStates["ESP ENABLE"];
    const bool espCircle = checkboxStates["ESP CIRCLE"];
    const bool esp3d = checkboxStates["3D::BOX"];
    const bool espAlert = checkboxStates["ESP ALERT 360"];
    const bool espLine = checkboxStates["ESP LINE"];
    const bool espSkel = checkboxStates["SKELETON"];
    const bool espBox = checkboxStates["ESP BOX"];
    const bool espName = checkboxStates["ESP NAME"];
    const bool espId = checkboxStates["ESP PLAYER ID"];
    const bool espHp = checkboxStates["ESP HEALTH"];
    const bool espArmor = checkboxStates["ESP ARMOR"];
    const bool espDist = checkboxStates["ESP DISTANCE"];
    const int linePos = dropdownStates["#LINE_POS"];
    const float fontH = ImGui::GetFontSize();
    
    for (int i = 0; i < pawnCount; i++) {
        Pawn *pawn = pawns[i];
        if (!Tools::IsPtrValid(pawn)) continue;
        if (!pawn->m_IsAlive()) continue;

        Transform* headBone = pawn->m_HeadBone();
        Transform* mesh = pawn->m_Mesh();
        if (!headBone || !mesh) continue;

        Vector3 pEnemyPawn_headPos = pawn->get_HeadPosition();
        Vector3 pEnemyPawn_rootPos = pawn->get_LastPawnPos();
        
        Vector3 headPos_WS = mainCamera->WorldToScreenPoint(pEnemyPawn_headPos);
        Vector3 rootPos_WS = mainCamera->WorldToScreenPoint(pEnemyPawn_rootPos);
        
        if (headPos_WS.z <= 0.1f || rootPos_WS.z <= 0.1f) continue;

        float distanceToMe = Vector3::Distance(pLocalPawn_rootPos, pEnemyPawn_rootPos);
        
        if (pawn->m_IsBot()) totalBots++;
        else totalEnemies++;

        if (!espOn) continue;

        float headX = headPos_WS.x;
        float headY = (float)glHeight - headPos_WS.y;
        float rootX = rootPos_WS.x;
        float rootY = (float)glHeight - rootPos_WS.y;
        SmoothScreen((uintptr_t)pawn, headX, headY, rootX, rootY);
        float boxH = fabsf(headY - rootY);
        float boxW = boxH * 0.46f;
        bool boxOk = boxH >= 8.f && boxH <= (float)glHeight * 1.4f;
        ImVec2 boxA(headX - boxW * 0.5f, headY);
        ImVec2 boxB(boxA.x + boxW, boxA.y + boxH);
        bool onScreen = boxB.x >= 0.f && boxA.x <= (float)glWidth && boxB.y >= 0.f && boxA.y <= (float)glHeight;
        ImU32 pawnCol = EspPawnColor(pawn);

        if (espCircle) {
            Draw3DCircle(draw, pawn, pEnemyPawn_rootPos, glHeight, deltaTime, mainCamera);
        }

        if (esp3d) {
            Draw3dBox(draw, (uintptr_t)pawn, pEnemyPawn_rootPos, glHeight, glWidth, mainCamera);
        }

        if (espAlert) {
            Vector2 screen(glWidth, glHeight);
            Vector2 location(headX, headY);
            
            int borders = 0;
            Vector2 topLeft(screen.x * 0.08f, screen.y * 0.08f);
            Vector2 bottomRight(screen.x * 0.92f, screen.y * 0.92f);
            
            if (location.y < topLeft.y) borders |= 1;
            if (location.x > bottomRight.x) borders |= 2;
            if (location.y > bottomRight.y) borders |= 4;
            if (location.x < topLeft.x) borders |= 8;
            
            if (borders) {
                float dotRadius = (glHeight / 1080.0f) * 50.0f;
                float margin = dotRadius + 10.0f;
                float dotX = std::max(margin, std::min(screen.x - margin, location.x));
                float dotY = std::max(margin, std::min(screen.y - margin, location.y));
                draw->AddCircleFilled({dotX, dotY}, dotRadius, IM_COL32(190, 190, 190, 220), 0);
            }
        }

        if (espLine && pawn->m_IsAlive()) {
            ImVec2 from, to;
            if (linePos == 0) {
                from = ImVec2((float)glWidth * 0.5f, (float)glHeight - 3.f);
                to = ImVec2(rootX, rootY);
            } else {
                from = ImVec2((float)glWidth * 0.5f, 20.f);
                to = ImVec2(headX, headY - 8.f);
            }
            draw->AddLine(from, to, pawnCol, 1.15f);
        }

        if (espSkel) {
            DrawSkeleton(draw, pawn, mainCamera, (float)glHeight);
        }

        if (espBox && boxOk && onScreen) {
            DrawCornerBox(draw, boxA, boxB, pawnCol);
        }

        if ((espName || espId) && pawn->m_IsAlive()) {
            char nameBuf[96];
            const char* shown = espName ? EspNameText(pawn) : "";
            if (espName && espId)
                snprintf(nameBuf, sizeof(nameBuf), "%s  %d", shown, (int)pawn->get_TeamSeatId());
            else if (espId)
                snprintf(nameBuf, sizeof(nameBuf), "%d", (int)pawn->get_TeamSeatId());
            else
                snprintf(nameBuf, sizeof(nameBuf), "%s", shown);
            DrawTag(draw, headX, headY - fontH - 8.f, nameBuf, IM_COL32(236, 242, 246, 245), pawnCol);
        }

        if (espHp && pawn->m_IsAlive() && boxOk && onScreen) {
            int get_Health = 0, max_Health = 0;
            AttackableTargetInfo* m_AttackableInfo = pawn->m_AttackableInfo();
            if (m_AttackableInfo) {
                get_Health = m_AttackableInfo->m_Health();
                max_Health = m_AttackableInfo->m_MaxHealth();
            }

            if (get_Health <= 0 || max_Health <= 0) {
                if (espArmor) {
                    int currentArmor = pawn->GetArmorValue();
                    int maxArmor = pawn->GetMaxArmor();
                    if (maxArmor > 0) {
                        float percent = std::clamp((float)currentArmor / (float)maxArmor, 0.0f, 1.0f);
                        float barW = 4.0f;
                        float barX = boxA.x - barW - 4.0f;
                        float fillY = boxA.y + boxH * (1.0f - percent);
                        draw->AddRectFilled(ImVec2(barX, boxA.y), ImVec2(barX + barW, boxB.y), IM_COL32(12, 14, 18, 200));
                        draw->AddRectFilled(ImVec2(barX, fillY), ImVec2(barX + barW, boxB.y), IM_COL32(70, 196, 230, 255));
                    }
                }
            } else {
                float percent = std::clamp((float)get_Health / (float)max_Health, 0.0f, 1.0f);
                float barW = 4.0f;
                float barX = boxA.x - barW - 4.0f;
                float fillY = boxA.y + boxH * (1.0f - percent);
                ImU32 color = percent > 0.55f ? IM_COL32(72, 214, 120, 245) :
                              percent > 0.25f ? IM_COL32(232, 196, 72, 245) :
                              IM_COL32(232, 78, 78, 245);
                draw->AddRectFilled(ImVec2(barX, boxA.y), ImVec2(barX + barW, boxB.y), IM_COL32(12, 14, 18, 210));
                draw->AddRectFilled(ImVec2(barX, fillY), ImVec2(barX + barW, boxB.y), color);
            }
        }

        if (espDist && pawn->m_IsAlive()) {
            char distBuf[16];
            snprintf(distBuf, sizeof(distBuf), "%dm", (int)distanceToMe);
            DrawTag(draw, headX, rootY + 4.0f, distBuf, IM_COL32(210, 230, 236, 235), IM_COL32(86, 214, 242, 90));
        }
    }

    if (checkboxStates["ESP::VEHICLE"]) {
        Vector3 myPos = Vector3::zero();
        Pawn *get_LocalPawn = GamePlay::get_LocalPawn();
        if (get_LocalPawn != nullptr) myPos = get_LocalPawn->get_LastPawnPos();
        
        BaseGame *get_MatchGame = GamePlay::get_MatchGame();
        if (get_MatchGame != nullptr) {
            List<PhysicsVehicle *> *m_AllVehicles = get_MatchGame->m_AllVehicles();
            if (m_AllVehicles != nullptr) {
                PhysicsVehicle **physicsVehicles = m_AllVehicles->getItems();
                if (physicsVehicles) {
                    for (int v = 0; v < m_AllVehicles->getSize(); v++) {
                        PhysicsVehicle *physicsVehicle = physicsVehicles[v];
                        if (!physicsVehicle || !physicsVehicle->get_transform()) continue;
                        Vector3 rootPos = physicsVehicle->get_transform()->get_position();
                        Vector3 screenPos = mainCamera->WorldToScreenPoint(rootPos);
                        if (screenPos.z <= 0) continue;
                        float distanceToMe = Vector3::Distance(myPos, rootPos);
                        char distBuf[16];
                        snprintf(distBuf, sizeof(distBuf), "%dM", (int)distanceToMe);
                        DrawTag(draw, screenPos.x, (float)glHeight - screenPos.y - 22.0f, distBuf, IM_COL32(236, 242, 246, 245), IM_COL32(86, 214, 242, 90));
                    }
                }
            }
        }
    }

    if (!checkboxStates["HAND::CAM"] && checkboxStates["ENEMY::COUNTER"]) {
        char label[48];
        snprintf(label, sizeof(label), "ENEMY  %d    BOT  %d", totalEnemies, totalBots);
        DrawTag(ImGui::GetForegroundDrawList(), (float)glWidth * 0.5f, 16.0f, label, IM_COL32(236, 244, 248, 245), IM_COL32(86, 214, 242, 170));
    }
}
void* Aimbot_Thread() {
    if (!checkboxStates["AIMBOT 360"]) return nullptr;
    auto matchGame = MatchGame::get_MatchGame();
    if (!matchGame) return nullptr;
    Pawn* localPawn = GamePlay::get_LocalPawn();
    if (!localPawn) return nullptr;
    Camera* main = Camera::get_main();
    if (!main) return nullptr;
    Transform* camTransform = main->get_transform();
    if (!camTransform) return nullptr;
    Vector3 camPos = camTransform->get_position();
    Vector3 aimDir = camTransform->get_forward();
    int triggerMode = dropdownStates["#TRIG"];
    bool bTriggerReady = (triggerMode == 0) || (triggerMode == 1 && localPawn->get_IsFiring()) || (triggerMode == 2 && localPawn->IsAiming()) || (triggerMode == 3 && (localPawn->get_IsFiring() || localPawn->IsAiming()));
    if (!bTriggerReady) return nullptr;
    uintptr_t Target = (dropdownStates["#DIST"] == 0) ? GetClosestTarget() : GetInsideFOVTarget();
    Pawn* targetPawn = reinterpret_cast<Pawn*>(Target);
    bool isValidTarget = Target && targetPawn && targetPawn->m_IsAlive() && (checkboxStates["DISABLE::VIS"] || targetPawn->isVisible()) && (!checkboxStates["IGNORE KNOCKED"] || !targetPawn->IsKnocked());
    if (!isValidTarget) return nullptr;
    int targetMode = dropdownStates["#TARG"];
    Vector3 targetPos = GetTargetWorldPosition(targetPawn, targetMode, camPos, aimDir);
    if (targetPos.x == 0 && targetPos.y == 0 && targetPos.z == 0) return nullptr;
    Quaternion currentRot = localPawn->get_AimRotation();
    Vector3 lookDir = targetPos - camPos;
    Quaternion targetRot = Quaternion::LookRotation(lookDir, Vector3::Up());
    float smoothness = std::clamp(seekbarValues["AIM SMOOTHNESS"], 1.0f, 100.0f);
    float aimSpeed = std::clamp(seekbarValues["AIM SPEED"], 1.0f, 100.0f);
    float smoothFactor = 1.0f - expf(-((0.25f + (aimSpeed / 100.0f) * 2.5f) * ImGui::GetIO().DeltaTime * 60.0f));
    smoothFactor = smoothFactor / (1.0f + (smoothness / 100.0f) * 3.0f);
    smoothFactor = std::min(smoothFactor, 0.005f + (aimSpeed / 100.0f) * 0.035f);
    localPawn->set_AimRotation(Quaternion::Slerp(currentRot, targetRot, smoothFactor));
    return nullptr;
}
void DrawESP(ImDrawList *draw) {
    drawESP__Player(draw);
    Aimbot_Thread();
}
