#pragma once

#include <cstdint>
#include <string>
#include <atomic>
//#include "ImGui/telelog.h"

typedef unsigned long long ulong;

uintptr_t get_off(const char* str) {
    return std::stoul(str, nullptr, 16);
}
// Add DOS version
#define GET_OFF(offset) get_off(DOS(offset))

#define BYPASS(offset) do { \
    void *dummy; \
DOSHOOK((void*)(libanogs_base + GET_OFF(offset)), (void*)hook_bypass, &dummy); \
} while(0)

#define PATCH(offset) do { \
    uintptr_t target = libanogs_base + GET_OFF(offset); \
    size_t page = sysconf(_SC_PAGESIZE); \
    uintptr_t start = target & ~(page - 1); \
    mprotect((void*)start, page, PROT_READ | PROT_WRITE | PROT_EXEC); \
    *(uint32_t*)target = 0xD2800000; \
    *(uint32_t*)(target + 4) = 0xD65F03C0; \
    mprotect((void*)start, page, PROT_READ | PROT_EXEC); \
    __builtin___clear_cache((char*)start, (char*)(start + page)); \
} while(0)

struct CSLobby_Offline_Ntf {
    int reason;  // offset 0x18
    int type;    // offset 0x1C  
    const char* message; // offset 0x20
};



typedef uint64_t _BYTE;
typedef uint64_t _QWORD;

auto DOSHOOK = [](void *target, void *replace, void **backup) {
    if (!target) return;
    if (DobbyHook(target, replace, backup) == 0) {
        void *page = (void *)((uintptr_t)(*backup) & ~(getpagesize() - 1));
        mprotect(page, getpagesize(), PROT_READ | PROT_EXEC); // Hide trampoline
    }
};


static uintptr_t libanogs_base = 0;
static uintptr_t libunity_base = 0;

#include "Hacks/Feature/WeaponCategoryConfig.h"

// ================= OFFSET ENCRYPTION =================


// CODM 1.6.59 — jnmaini Dump 1.6.59.cs, libunity.so. Not the MLBB dump.cs.
typedef void (*SetUltraFrameRateDeviceInfo_t)(void *, bool, int, int, int, bool);
static SetUltraFrameRateDeviceInfo_t orig_SetUltraFrameRateDeviceInfo;

static void hooked_SetUltraFrameRateDeviceInfo(void *thiz, bool enableUltraFrameRate, int ultraFrameRate, int ultraFrameRateBR, int ultraFrameRateQualityLimit, bool customizedFrameRate) {
    if (!orig_SetUltraFrameRateDeviceInfo) return;
    orig_SetUltraFrameRateDeviceInfo(thiz, enableUltraFrameRate, ultraFrameRate, ultraFrameRateBR, ultraFrameRateQualityLimit, customizedFrameRate);
}

//fastscope
static float (*orig_get_AimingTime)(void* instance);
static float hook_get_AimingTime(void* instance) {
    if (instance != NULL) {
        float slider = seekbarValues["FAST::SCOPE"];
        float t = slider / 100.0f;
        float originalTime = orig_get_AimingTime(instance);
        float minTime = 0.01f;
        float finalTime = originalTime * (1.0f - t) + (minTime * t);
        if (finalTime < 0.01f) finalTime = 0.01f;
        return finalTime;
    }
    return orig_get_AimingTime(instance);
}

static void (*orig_OpenAim)(void* instance);
static void hook_OpenAim(void* instance) {
    if (instance != NULL) {
        float slider = seekbarValues["FAST::SCOPE"];
        float t = slider / 100.0f;
        if (t >= 0.99f) {
            orig_OpenAim(instance);
            return;
        }
    }
    orig_OpenAim(instance);
}

static void (*orig_CloseAim)(void* instance);
static void hook_CloseAim(void* instance) {
    if (instance != NULL) {
        float slider = seekbarValues["FAST::SCOPE"];
        float t = slider / 100.0f;
        if (t >= 0.99f) {
            orig_CloseAim(instance);
            return;
        }
    }
    orig_CloseAim(instance);
}

static bool (*orig_get_IsAiming)(void* instance);
static bool hook_get_IsAiming(void* instance) {
    if (instance != NULL) {
        float slider = seekbarValues["FAST::SCOPE"];
        float t = slider / 100.0f;
        if (t >= 0.99f) {
            return true;
        }
    }
    return orig_get_IsAiming(instance);
}



//AIM ASSIST
/// AIM ASSIST - Add these function pointer declarations
static float (*new_aim1)(void* instance, float dist, float toQuasiDist, bool isMPPVE, float aimAngle, bool gamepadInput);

static float newaim1(void* instance, float dist, float toQuasiDist, bool isMPPVE, float aimAngle, bool gamepadInput) {
    if (!new_aim1) return 0.0f;  // Add this line
    float assistStrength = seekbarValues["AIM::ASSIST"];
    if (instance && assistStrength > 0.0f) {
        return assistStrength;
    }
    return new_aim1(instance, dist, toQuasiDist, isMPPVE, aimAngle, gamepadInput);
}
static float (*new_aim2)(void* instance, float assistDis, float dis, float angle, float rotateTime, bool gamepadInput);
static float newaim2(void* instance, float assistDis, float dis, float angle, float rotateTime, bool gamepadInput) {
    if (!new_aim2) return 0.0f;  // Add this line
    float assistStrength = seekbarValues["AIM::ASSIST"];
    if (instance && assistStrength > 0.0f) {
        return assistStrength;
    }
    return new_aim2(instance, assistDis, dis, angle, rotateTime, gamepadInput);
}

//AUTO SKIP TUTORIAL
static bool (*orig_IsTutorialEnabled)(void *instance);
static bool hook_IsTutorialEnabled(void *instance)
{
    return false;
}


 
static bool (*orig_get_AdvanceUAVEnabled)(void *instance);
static bool hook_get_AdvanceUAVEnabled(void *instance) {
if (instance != NULL) {
if (checkboxStates["ADVANCE::UAV"]) {
return true;
}
}
return orig_get_AdvanceUAVEnabled(instance);
}


static float (*orig_GetScaleRecoil)(void* instance);
static float hook_GetScaleRecoil(void* instance) {
    if (!orig_GetScaleRecoil) return 0.0f;
    if (!instance) return orig_GetScaleRecoil(instance);
    
    float slider = seekbarValues["NO::RECOIL"];
    float t = slider / 100.0f;
    float originalRecoil = orig_GetScaleRecoil(instance);
    float minRecoil = 0.00001f;
    return originalRecoil * (1.0f - t) + (minRecoil * t);
}

static Vector2 (*orig_GetPunchAngle)(void* instance);
static Vector2 hook_GetPunchAngle(void* instance) {
    if (!orig_GetPunchAngle) return Vector2{0, 0};
    if (!instance) return orig_GetPunchAngle(instance);
    
    float slider = seekbarValues["NO::RECOIL"];
    float t = slider / 100.0f;
    Vector2 originalPunch = orig_GetPunchAngle(instance);
    float minPunch = 0.00001f;
    return Vector2{
        originalPunch.x * (1.0f - t) + (minPunch * t),
        originalPunch.y * (1.0f - t) + (minPunch * t)
    };
}


static float (*orig_get_ChangeClipTime)(void* instance);
static float hook_get_ChangeClipTime(void* instance) {
    if (instance != NULL) {
        float slider = seekbarValues["NO::RELOAD"];
        if (slider >= 99.0f) return 0.01f;
        float t = slider / 100.0f;
        float originalTime = orig_get_ChangeClipTime(instance);
        return originalTime * (1.0f - t);
    }
    return orig_get_ChangeClipTime(instance);
}

static float (*orig_get_ChangeClipStartTime)(void* instance);
static float hook_get_ChangeClipStartTime(void* instance) {
    if (instance != NULL) {
        float slider = seekbarValues["NO::RELOAD"];
        if (slider >= 99.0f) return 0.01f;
        float t = slider / 100.0f;
        float originalTime = orig_get_ChangeClipStartTime(instance);
        return originalTime * (1.0f - t);
    }
    return orig_get_ChangeClipStartTime(instance);
}

static float (*orig_get_ChangeClipEndTime)(void* instance);
static float hook_get_ChangeClipEndTime(void* instance) {
    if (instance != NULL) {
        float slider = seekbarValues["NO::RELOAD"];
        if (slider >= 99.0f) return 0.01f;
        float t = slider / 100.0f;
        float originalTime = orig_get_ChangeClipEndTime(instance);
        return originalTime * (1.0f - t);
    }
    return orig_get_ChangeClipEndTime(instance);
}



static float (*orig_GetRealSpreadModifier)(void* instance);
static float hook_GetRealSpreadModifier(void* instance) {
    if (instance != NULL) {
        float slider = seekbarValues["NO::SPREAD"];
        if (slider >= 99.0f) return 0.0f;
        float t = slider / 100.0f;
        float originalModifier = orig_GetRealSpreadModifier(instance);
        return originalModifier * (1.0f - t);
    }
    return orig_GetRealSpreadModifier(instance);
}


static Vector2 (*orig_GetShotSpread)(void* instance);
static Vector2 hook_GetShotSpread(void* instance) {
    if (!instance) return orig_GetShotSpread(instance);
    float slider = seekbarValues["NO::SPREAD"];
    Vector2 original = orig_GetShotSpread(instance);
    float t = slider / 100.0f;
    return Vector2{original.x * (1.0f - t), original.y * (1.0f - t)};
}



	
static bool ( * orig_SingleLineCheckPhysics)(void * instance, int hitType, void * hitTarget, void * hitCollider, Vector3 startPos, Vector3 dir, void * impactInfo);
static bool hook_SingleLineCheckPhysics(void * instance, int hitType, void * hitTarget, void * hitCollider, Vector3 startPos, Vector3 dir, void * impactInfo) {
if (instance != NULL) {
    if (instance != nullptr && checkboxStates["HITBOX"]) {
return (true, hitType, hitTarget, hitCollider, startPos, dir, impactInfo);
}
}
return orig_SingleLineCheckPhysics(instance, hitType, hitTarget, hitCollider, startPos, dir, impactInfo);
}

static float (*orig_UpdateCameraShake)(void *instance);
static float hook_UpdateCameraShake(void *instance) {
if (instance != NULL) {
if (checkboxStates["NO::SHAKE"]) {
return 0;
}
}
return orig_UpdateCameraShake(instance);
}

// --- No Parachute ---
static void (*orig_OpenParachute)(void *instance, bool isAuto);
static void hook_OpenParachute(void *instance, bool isAuto) {
    if (instance != NULL) {
        if (checkboxStates["NO::PARACHUTE"]) {
            return; // do nothing
        }
    }
    return orig_OpenParachute(instance, isAuto);
}


static bool (*orig_get_EnableVulkan)(void *instance);
static bool hook_get_EnableVulkan(void *instance) {
    return false; 
}

static bool (*orig_ShouldCurDeviceEnableVK)();
static bool hook_ShouldCurDeviceEnableVK() {
    return false; 
}

static void (*orig_set_EnableVulkan)(void *instance, bool value);
static void hook_set_EnableVulkan(void *instance, bool value) {
    return orig_set_EnableVulkan(instance, false);
}
//unlock all graphics
static void (*orig_SetEnableVSync)(void* thiz);
static void hook_SetEnableVSync(void* thiz) {
    if (orig_SetEnableVSync) orig_SetEnableVSync(thiz);
}


    
// --- Swimming Speed Scale ---
static float (*orig_CalcSwimmingSpeedScale)(void *instance) = nullptr;
static float hook_CalcSwimmingSpeedScale(void *instance) {
    if (!orig_CalcSwimmingSpeedScale) return 1.0f;
    if (instance != nullptr && checkboxStates["FAST::SWIM"]) {
        return 3.0f; 
    }
    return orig_CalcSwimmingSpeedScale(instance);
}

//FOV BR
static float (*orig_GetMaxFov_3p)(void *instance) = nullptr;
static float hook_GetMaxFov_3p(void *instance) {
    if (instance != nullptr && seekbarValues["FOV"] > 0.0f) {
        return seekbarValues["FOV"]; // slider value only if user moved it
    }
    return orig_GetMaxFov_3p(instance); // otherwise original
}

//FOV MP
static float (*orig_GetMaxFov_1p)(void *instance) = nullptr;
static float hook_GetMaxFov_1p(void *instance) {
    if (instance != nullptr && seekbarValues["FOV1"] > 0.0f) {
        return seekbarValues["FOV1"]; // slider value only if user moved it
    }
    return orig_GetMaxFov_1p(instance); // otherwise original
}

// -------SNOWB
static float (*orig_get_m_PhysSkisMaxSpeed)(void* instance) = nullptr;
static float hook_get_m_PhysSkisMaxSpeed(void* instance) {
    if (instance != nullptr && seekbarValues["SizeSnow"] > 0.0f) {
        return seekbarValues["SizeSnow"];
    }
    return orig_get_m_PhysSkisMaxSpeed(instance);
}


int hook_bypass(){
return 0;
}


//FPSHIGH
extern std::atomic<bool> gMatchLive;
static int (*orig_GetFrameRateValue)(void* instance, int fl, int mode) = nullptr;
static int hook_GetFrameRateValue(void* instance, int fl, int mode) {
    if (!orig_GetFrameRateValue) return 60;
    if (instance != nullptr && gMatchLive.load() && checkboxStates["FRAME120"]) return 120;
    if (instance != nullptr && gMatchLive.load() && checkboxStates["FRAME40"]) return 40;
    return orig_GetFrameRateValue(instance, fl, mode);
}

// CODM 1.6.59 skin ownership. Client checks only. Server still accepts or rejects the equip.
static bool SkinUnlockOn() {
    return checkboxStates["SKINUNLOCK"];
}

static bool (*orig_Loadout_IsHasInventoryData)(uint32_t itemID) = nullptr;
static bool hook_Loadout_IsHasInventoryData(uint32_t itemID) {
    if (SkinUnlockOn()) return true;
    if (!orig_Loadout_IsHasInventoryData) return false;
    return orig_Loadout_IsHasInventoryData(itemID);
}

static bool (*orig_Squad_IsHasInventoryData)(void* thiz, uint32_t itemID) = nullptr;
static bool hook_Squad_IsHasInventoryData(void* thiz, uint32_t itemID) {
    if (SkinUnlockOn()) return true;
    if (!orig_Squad_IsHasInventoryData) return false;
    return orig_Squad_IsHasInventoryData(thiz, itemID);
}

static bool (*orig_Own_IsHasInventoryData)(void* thiz, uint32_t itemID) = nullptr;
static bool hook_Own_IsHasInventoryData(void* thiz, uint32_t itemID) {
    if (SkinUnlockOn()) return true;
    if (!orig_Own_IsHasInventoryData) return false;
    return orig_Own_IsHasInventoryData(thiz, itemID);
}

static bool (*orig_IsOwnPermanentItem)(void* thiz, uint32_t id, uint32_t ownerId) = nullptr;
static bool hook_IsOwnPermanentItem(void* thiz, uint32_t id, uint32_t ownerId) {
    if (SkinUnlockOn()) return true;
    if (!orig_IsOwnPermanentItem) return false;
    return orig_IsOwnPermanentItem(thiz, id, ownerId);
}

static bool (*orig_IsCharacterLocked)(void* thiz, uint32_t characterID) = nullptr;
static bool hook_IsCharacterLocked(void* thiz, uint32_t characterID) {
    if (SkinUnlockOn()) return false;
    if (!orig_IsCharacterLocked) return false;
    return orig_IsCharacterLocked(thiz, characterID);
}

static bool (*orig_IsLockBaseRole)(void* thiz, uint32_t roleBaseID, void* ownRoles) = nullptr;
static bool hook_IsLockBaseRole(void* thiz, uint32_t roleBaseID, void* ownRoles) {
    if (SkinUnlockOn()) return false;
    if (!orig_IsLockBaseRole) return false;
    return orig_IsLockBaseRole(thiz, roleBaseID, ownRoles);
}

static bool (*orig_IsShowLockIcon)(void* thiz, int gameType, uint32_t itemId) = nullptr;
static bool hook_IsShowLockIcon(void* thiz, int gameType, uint32_t itemId) {
    if (SkinUnlockOn()) return false;
    if (!orig_IsShowLockIcon) return false;
    return orig_IsShowLockIcon(thiz, gameType, itemId);
}

static bool (*orig_IsSkinCamoConceptUnlocked)(int weaponType, uint32_t baseWeaponID, uint32_t camosConceptType) = nullptr;
static bool hook_IsSkinCamoConceptUnlocked(int weaponType, uint32_t baseWeaponID, uint32_t camosConceptType) {
    if (SkinUnlockOn()) return true;
    if (!orig_IsSkinCamoConceptUnlocked) return false;
    return orig_IsSkinCamoConceptUnlocked(weaponType, baseWeaponID, camosConceptType);
}

static bool (*orig_CamosConceptUnlocked)(uint32_t camosConceptID) = nullptr;
static bool hook_CamosConceptUnlocked(uint32_t camosConceptID) {
    if (SkinUnlockOn()) return true;
    if (!orig_CamosConceptUnlocked) return false;
    return orig_CamosConceptUnlocked(camosConceptID);
}

static bool (*orig_SpecialCamoUnlocked)(uint32_t baseWeaponID) = nullptr;
static bool hook_SpecialCamoUnlocked(uint32_t baseWeaponID) {
    if (SkinUnlockOn()) return true;
    if (!orig_SpecialCamoUnlocked) return false;
    return orig_SpecialCamoUnlocked(baseWeaponID);
}

static bool (*orig_IsWeaponSkinUnlock)(void* thiz, uint32_t skinId) = nullptr;
static bool hook_IsWeaponSkinUnlock(void* thiz, uint32_t skinId) {
    if (SkinUnlockOn()) return true;
    if (!orig_IsWeaponSkinUnlock) return false;
    return orig_IsWeaponSkinUnlock(thiz, skinId);
}

static bool (*orig_IsOwnedAppearance)(uint32_t itemId, uint32_t itemSubType) = nullptr;
static bool hook_IsOwnedAppearance(uint32_t itemId, uint32_t itemSubType) {
    if (SkinUnlockOn()) return true;
    if (!orig_IsOwnedAppearance) return false;
    return orig_IsOwnedAppearance(itemId, itemSubType);
}

static bool (*orig_CanEuippedRoleSkin)(void* thiz, uint32_t itemID) = nullptr;
static bool hook_CanEuippedRoleSkin(void* thiz, uint32_t itemID) {
    if (SkinUnlockOn()) return true;
    if (!orig_CanEuippedRoleSkin) return false;
    return orig_CanEuippedRoleSkin(thiz, itemID);
}

static bool (*orig_CanEuippedRoleSkin2)(void* roleConfig, void* roleSkinConfig) = nullptr;
static bool hook_CanEuippedRoleSkin2(void* roleConfig, void* roleSkinConfig) {
    if (SkinUnlockOn()) return true;
    if (!orig_CanEuippedRoleSkin2) return false;
    return orig_CanEuippedRoleSkin2(roleConfig, roleSkinConfig);
}

static bool (*orig_CanEquipRoleSkin)(void* playerInfo, uint32_t itemId) = nullptr;
static bool hook_CanEquipRoleSkin(void* playerInfo, uint32_t itemId) {
    if (SkinUnlockOn()) return true;
    if (!orig_CanEquipRoleSkin) return false;
    return orig_CanEquipRoleSkin(playerInfo, itemId);
}

// Typed IDs. Dump 1.6.59 has no skin catalog; RoleSkinConfig.ID and WeaponCamosConfig.CamoItemID are filled at runtime.
uint32_t gWeaponSkinId = 0;
uint32_t gSuitSkinId = 0;
static uint32_t (*fn_GetBaseWeaponID)(uint32_t weaponID) = nullptr;
static void* (*fn_GetConceptList)(uint32_t baseWeaponId) = nullptr;
static void* (*fn_GetCamosByConcept)(uint32_t conceptId) = nullptr;
static void* (*fn_CurrentWeapon)() = nullptr;
static int (*fn_GetItemID)(void* weapon) = nullptr;
static uint32_t (*fn_GetCurSkinID)(void* weapon) = nullptr;

static bool SkinSwapOn() {
    return checkboxStates["SKINSWAP"];
}

static uint32_t (*orig_Weapon_get_CurSkinID)(void* thiz) = nullptr;
static uint32_t hook_Weapon_get_CurSkinID(void* thiz) {
    uint32_t id = orig_Weapon_get_CurSkinID ? orig_Weapon_get_CurSkinID(thiz) : 0;
    if (SkinSwapOn() && gWeaponSkinId != 0 && id == 0) return gWeaponSkinId;
    return id;
}

static uint32_t (*orig_get_NetSuitID)(void* thiz) = nullptr;
static uint32_t hook_get_NetSuitID(void* thiz) {
    uint32_t id = orig_get_NetSuitID ? orig_get_NetSuitID(thiz) : 0;
    if (SkinSwapOn() && gSuitSkinId != 0 && id == 0) return gSuitSkinId;
    return id;
}

static int (*orig_GetModelAssetID)(int assetID, uint32_t skinID, bool for1P, uint8_t skinType, bool isPawn, bool isWeapon, uint64_t roleID) = nullptr;
static int hook_GetModelAssetID(int assetID, uint32_t skinID, bool for1P, uint8_t skinType, bool isPawn, bool isWeapon, uint64_t roleID) {
    if (!orig_GetModelAssetID) return assetID;
    if (SkinSwapOn() && skinID == 0) {
        if (isWeapon && gWeaponSkinId != 0) skinID = gWeaponSkinId;
        else if (isPawn && gSuitSkinId != 0) skinID = gSuitSkinId;
    }
    return orig_GetModelAssetID(assetID, skinID, for1P, skinType, isPawn, isWeapon, roleID);
}

static int (*orig_GetModelAssetID2)(int assetID, uint32_t skinID, bool for1P, uint64_t roleID, bool isWeapon, bool isUsePawnSkin) = nullptr;
static int hook_GetModelAssetID2(int assetID, uint32_t skinID, bool for1P, uint64_t roleID, bool isWeapon, bool isUsePawnSkin) {
    if (!orig_GetModelAssetID2) return assetID;
    if (SkinSwapOn() && skinID == 0) {
        if (isUsePawnSkin && gSuitSkinId != 0) skinID = gSuitSkinId;
        else if (isWeapon && gWeaponSkinId != 0) skinID = gWeaponSkinId;
    }
    return orig_GetModelAssetID2(assetID, skinID, for1P, roleID, isWeapon, isUsePawnSkin);
}

static void (*orig_SkinDoChange)(void* root, int weaponID, uint32_t skinID, bool is1P, uint8_t skinType, void* ultSkinIndex, bool isUI, uint64_t roleID) = nullptr;
static void hook_SkinDoChange(void* root, int weaponID, uint32_t skinID, bool is1P, uint8_t skinType, void* ultSkinIndex, bool isUI, uint64_t roleID) {
    if (!orig_SkinDoChange) return;
    if (SkinSwapOn() && skinID == 0 && gWeaponSkinId != 0) skinID = gWeaponSkinId;
    orig_SkinDoChange(root, weaponID, skinID, is1P, skinType, ultSkinIndex, isUI, roleID);
}

static void (*o_OnReSpawnPlayer)(void *, Vector3) = nullptr;
static void h_OnReSpawnPlayer(void *instance, Vector3 serverSyncPos) {
    if (o_OnReSpawnPlayer) {
        o_OnReSpawnPlayer(instance, serverSyncPos);
    }
    if (instance && checkboxStates["WALLHACK"]) {
        ((void (*)(void *, bool))(libunity_base + GET_OFF("0x5565680")))(instance, true);
    }
}

static void (*orig_CheckCrouch)(void *instance);
static void hook_CheckCrouch(void *instance) {
    if (!orig_CheckCrouch) return;
    if (instance != NULL && checkboxStates["NO CROUCH"]) return;
    orig_CheckCrouch(instance);
}

static void (*orig_Tick)(void* instance, float deltaTime);
static void hook_Tick(void* instance, float deltaTime) {
    if (instance != NULL) {
        float slider = seekbarValues["FAST::LOOT"];
        if (slider >= 99.0f) {
            float* finishCountdown = (float*)((uintptr_t)instance + 0x40);
            if (*finishCountdown > 0.0f) {
                *finishCountdown = 0.0f; // Force instant completion
            }
        }
    }
    orig_Tick(instance, deltaTime);
}




void (*orig_GameUIScene_OnKickOff)(void* instance, void* msg);
void hooked_GameUIScene_OnKickOff(void* instance, void* msg) {
    return;
}





// ================= Speedhack =================

static float (*orig_get_deltaTime)() = nullptr;
static float (*orig_get_fixedDeltaTime)() = nullptr;
float GetSpeedMultiplier() {
    float speed = seekbarValues["SPEED_HACK"];
    if (speed > 0.0f) {
        if (speed > 100.0f) speed = 100.0f;
        return 1.0f + (speed / 100.0f) * 10.0f;
    }
    return 1.0f;
}

float hooked_get_deltaTime() { 
    if (!orig_get_deltaTime) return 0.016f;
    return orig_get_deltaTime() * GetSpeedMultiplier(); 
}

float hooked_get_fixedDeltaTime() { 
    if (!orig_get_fixedDeltaTime) return 0.02f;
    return orig_get_fixedDeltaTime() * GetSpeedMultiplier(); 
}

//====================MISDIRECTION=====================
static bool g_IllusionForced = false;
static void (*orig_IllusionTick)(void*, float);
static void hook_IllusionTick(void* instance, float dt) {
    orig_IllusionTick(instance, dt);
    
    if (!instance || !checkboxStates["AUTO::MISDIRECTION"]) {
        g_IllusionForced = false;
        return;
    }
    
    *(bool*)((uintptr_t)instance + 0x41) = true;
    g_IllusionForced = true;
}

static bool (*orig_GetEnableVulkan)(void* instance);
static bool hook_GetEnableVulkan(void* instance) {
    return false;
}

static bool (*orig_GetUseVulkanVRS)(void* instance);
static bool hook_GetUseVulkanVRS(void* instance) {
    return false;
}



static void hooking() {
    static bool init = false; 
    if (init) return; 
    init = true;
   
    if (libanogs_base == 0) {
        libanogs_base = getAbsoluteAddress(DOS("libanogs.so"), 0); 
    }
    if (libunity_base == 0) {
        libunity_base = getAbsoluteAddress(DOS("libunity.so"), 0); 
    }
    fn_GetBaseWeaponID = (uint32_t (*)(uint32_t))(libunity_base + GET_OFF("0x5A4C18C"));
    fn_GetConceptList = (void* (*)(uint32_t))(libunity_base + GET_OFF("0x85D45CC"));
    fn_GetCamosByConcept = (void* (*)(uint32_t))(libunity_base + GET_OFF("0x85D4C74"));
    fn_CurrentWeapon = (void* (*)())(libunity_base + GET_OFF("0x80167E8"));
    fn_GetItemID = (int (*)(void*))(libunity_base + GET_OFF("0xC0D0088"));
    fn_GetCurSkinID = (uint32_t (*)(void*))(libunity_base + GET_OFF("0x5471B10"));

BYPASS("0x204218"); 
BYPASS("0x45B334");  
BYPASS("0x44A714");  
BYPASS("0x48F964"); 

//FEATURES

DOSHOOK((void*)(libunity_base + GET_OFF("0x8058220")), (void*)hook_IllusionTick, (void**)&orig_IllusionTick);  // MISDIRECTION
//ANTIKICK //public class GameUIScene : UIScene
DOSHOOK((void*)(libunity_base + GET_OFF("0x73669C4")), (void*)hooked_GameUIScene_OnKickOff, (void**)&orig_GameUIScene_OnKickOff);
//ANTIKICK


//FAST LOOT
DOSHOOK((void*)(libunity_base + GET_OFF("0xA7EAB18")), (void*)hook_Tick, (void**)&orig_Tick);

// No Crouch
DOSHOOK((void*)(libunity_base + GET_OFF("0x66B882C")), (void*)hook_CheckCrouch, (void**)&orig_CheckCrouch);

//speedhack
DOSHOOK((void*)(libunity_base + GET_OFF("0x530EF10")), (void*)hooked_get_deltaTime, (void**)&orig_get_deltaTime);
DOSHOOK((void*)(libunity_base + GET_OFF("0x534D8E4")), (void*)hooked_get_fixedDeltaTime, (void**)&orig_get_fixedDeltaTime);

// Wallhack
DOSHOOK((void*)(libunity_base + GET_OFF("0x53D2114")), (void*)&h_OnReSpawnPlayer, (void**)&o_OnReSpawnPlayer);

// CODM Dump 1.6.59.cs on libunity.so
DOSHOOK((void*)(libunity_base + GET_OFF("0xAFAC73C")), (void*)hook_GetFrameRateValue, (void**)&orig_GetFrameRateValue);
DOSHOOK((void*)(libunity_base + GET_OFF("0xAFAB394")), (void*)&hooked_SetUltraFrameRateDeviceInfo, (void**)&orig_SetUltraFrameRateDeviceInfo);

// Snowboard Speed
DOSHOOK((void*)(libunity_base + GET_OFF("0x53FE7FC")), (void*)hook_get_m_PhysSkisMaxSpeed, (void**)&orig_get_m_PhysSkisMaxSpeed);

// FOV
DOSHOOK((void*)(libunity_base + GET_OFF("0xC0B472C")), (void*)hook_GetMaxFov_3p, (void**)&orig_GetMaxFov_3p);
DOSHOOK((void*)(libunity_base + GET_OFF("0xC0B4458")), (void*)hook_GetMaxFov_1p, (void**)&orig_GetMaxFov_1p);

// Fast Swim
DOSHOOK((void*)(libunity_base + GET_OFF("0x5597B14")), (void*)hook_CalcSwimmingSpeedScale, (void**)&orig_CalcSwimmingSpeedScale);

// Graphics
DOSHOOK((void*)(libunity_base + GET_OFF("0xAFAD9D4")), (void*)hook_SetEnableVSync, (void**)&orig_SetEnableVSync);
DOSHOOK((void*)(libunity_base + GET_OFF("0xAFB9F44")), (void*)hook_get_EnableVulkan, (void**)&orig_get_EnableVulkan);


// No Parachute
DOSHOOK((void*)(libunity_base + GET_OFF("0x5A88C48")), (void*)hook_OpenParachute, (void**)&orig_OpenParachute);
/*
//Fast scope
DOSHOOK((void*)(libunity_base + GET_OFF("0x5474160")), (void*)hook_get_AimingTime, (void**)&orig_get_AimingTime);
DOSHOOK((void*)(libunity_base + GET_OFF("0x54B8198")), (void*)hook_OpenAim, (void**)&orig_OpenAim);
DOSHOOK((void*)(libunity_base + GET_OFF("0x54B900C")), (void*)hook_CloseAim, (void**)&orig_CloseAim);
DOSHOOK((void*)(libunity_base + GET_OFF("0x54F60A4")), (void*)hook_get_IsAiming, (void**)&orig_get_IsAiming);
*/
// Aim Assist
DOSHOOK((void*)(libunity_base + GET_OFF("0x54F0E64")), (void*)&newaim1, (void**)&new_aim1);
DOSHOOK((void*)(libunity_base + GET_OFF("0x66BD5E8")), (void*)&newaim2, (void**)&new_aim2);

// Skip Tutorial
DOSHOOK((void*)(libunity_base + GET_OFF("0xCCC3240")), (void*)&hook_IsTutorialEnabled, (void**)&orig_IsTutorialEnabled);

// Advance UAV
DOSHOOK((void*)(libunity_base + GET_OFF("0x6AE9A70")), (void*)&hook_get_AdvanceUAVEnabled, (void**)&orig_get_AdvanceUAVEnabled);


DOSHOOK((void*)(libunity_base + GET_OFF("0xD535A6C")), (void*)hook_GetScaleRecoil, (void**)&orig_GetScaleRecoil);
DOSHOOK((void*)(libunity_base + GET_OFF("0xD5359E4")), (void*)hook_GetPunchAngle, (void**)&orig_GetPunchAngle);

// No Reload
DOSHOOK((void*)(libunity_base + GET_OFF("0xC2B5B58")), (void*)hook_get_ChangeClipTime, (void**)&orig_get_ChangeClipTime);
DOSHOOK((void*)(libunity_base + GET_OFF("0xC2B5D88")), (void*)hook_get_ChangeClipStartTime, (void**)&orig_get_ChangeClipStartTime);
DOSHOOK((void*)(libunity_base + GET_OFF("0xC2B5F90")), (void*)hook_get_ChangeClipEndTime, (void**)&orig_get_ChangeClipEndTime);

// No Spread
DOSHOOK((void*)(libunity_base + GET_OFF("0xD53408C")), (void*)hook_GetRealSpreadModifier, (void**)&orig_GetRealSpreadModifier);
DOSHOOK((void*)(libunity_base + GET_OFF("0xC2BDF40")), (void*)hook_GetShotSpread, (void**)&orig_GetShotSpread);

// No Shake
DOSHOOK((void*)(libunity_base + GET_OFF("0x669780C")), (void*)&hook_UpdateCameraShake, (void**)&orig_UpdateCameraShake);

// Hitbox
DOSHOOK((void*)(libunity_base + GET_OFF("0xC2BE0C8")), (void*)&hook_SingleLineCheckPhysics, (void**)&orig_SingleLineCheckPhysics);

}