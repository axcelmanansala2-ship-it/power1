#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <atomic>
#include <thread>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include <sys/ucontext.h>
#include <sys/uio.h>
#include <sys/syscall.h>
#include <dlfcn.h>
#include "dobby/dobby.h"
#include "ANOGS/Call_IL2Cpp.h"

static constexpr bool kCallMonitor =
#include "../on/off"
;

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
    if (!thiz) {
        orig_SetUltraFrameRateDeviceInfo(thiz, enableUltraFrameRate, ultraFrameRate, ultraFrameRateBR, ultraFrameRateQualityLimit, customizedFrameRate);
        return;
    }
    if (checkboxStates["FRAME120"]) {
        orig_SetUltraFrameRateDeviceInfo(thiz, true, 120, 120, 100, true);
        return;
    }
    if (checkboxStates["FRAME40"]) {
        orig_SetUltraFrameRateDeviceInfo(thiz, false, 40, 40, 40, false);
        return;
    }
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


struct AnogsRow {
    uint32_t site;
    uint32_t count;
    uintptr_t caller;
    char line[200];
};

static AnogsRow gAnogsRows[2048];
static int gAnogsUsed = 0;
static uint32_t gAnogsSlot[8192];
static std::atomic<uint32_t> gAnogsHits{0};
static std::atomic<int> gAnogsLock{0};
static uintptr_t gAnogsSelf = 0;
struct AnogsSpan { uintptr_t a, b; };
static AnogsSpan gReadSpans[16];
static int gReadN = 0;
static char gAnogsCopyMsg[80] = "";

static bool AnogsLock() {
    int expected = 0;
    return gAnogsLock.compare_exchange_strong(expected, 1, std::memory_order_acquire);
}

static void AnogsUnlock() {
    gAnogsLock.store(0, std::memory_order_release);
}

static void AnogsFmt(char* dst, size_t cap, uintptr_t addr) {
    Dl_info info;
    if (addr > 0x10000 && dladdr((void*)addr, &info) && info.dli_fbase && info.dli_fname) {
        const char* name = strrchr(info.dli_fname, '/');
        name = name ? name + 1 : info.dli_fname;
        snprintf(dst, cap, "%s+0x%X", name, (unsigned)((uintptr_t)addr - (uintptr_t)info.dli_fbase));
        return;
    }
    snprintf(dst, cap, "0x%llX", (unsigned long long)addr);
}

static bool AnogsInSelf(uintptr_t addr) {
    return gAnogsSelf && addr >= gAnogsSelf && addr < gAnogsSelf + 0x400000;
}

static bool AnogsMapped(uintptr_t p) {
    for (int i = 0; i < gReadN; ++i) {
        if (p >= gReadSpans[i].a && p + 48 < gReadSpans[i].b) return true;
    }
    return false;
}

static void AnogsPullString(char* dst, int cap, uintptr_t p) {
    dst[0] = 0;
    if (cap < 2 || !AnogsMapped(p)) return;
    int n = 0;
    const unsigned char* s = (const unsigned char*)p;
    for (; n < cap - 1 && n < 48; ++n) {
        unsigned char c = s[n];
        if (c == 0) break;
        if (c < 32 || c > 126) { n = 0; break; }
    }
    if (n < 4) return;
    memcpy(dst, s, (size_t)n);
    dst[n] = 0;
}

static void AnogsOnCall(uintptr_t site, uintptr_t caller, uintptr_t arg0, uintptr_t arg1) {
    gAnogsHits.fetch_add(1, std::memory_order_relaxed);
    if (!site || site < 0x10000) return;
    if (caller && AnogsInSelf(caller)) return;
    if (!gAnogsSelf) {
        Dl_info info;
        if (dladdr((void*)&AnogsOnCall, &info) && info.dli_fbase) gAnogsSelf = (uintptr_t)info.dli_fbase;
    }
    uint32_t siteOff = (libanogs_base && site >= libanogs_base) ? (uint32_t)(site - libanogs_base) : (uint32_t)site;
    uint32_t h = (siteOff ^ (uint32_t)(caller >> 2)) & 8191u;
    bool locked = false;
    for (int spin = 0; spin < 8; ++spin) {
        if (AnogsLock()) { locked = true; break; }
    }
    if (!locked) return;
    for (int p = 0; p < 16; ++p) {
        uint32_t slot = (h + (uint32_t)p) & 8191u;
        uint32_t mark = gAnogsSlot[slot];
        if (mark == 0) {
            if (gAnogsUsed >= 2048) break;
            AnogsRow& row = gAnogsRows[gAnogsUsed];
            row.site = siteOff;
            row.caller = caller;
            row.count = 1;
            char from[64], str0[49], str1[49];
            if (caller) AnogsFmt(from, sizeof(from), caller);
            else from[0] = 0;
            AnogsPullString(str0, sizeof(str0), arg0);
            AnogsPullString(str1, sizeof(str1), arg1);
            const char* quote = str0[0] ? str0 : str1;
            if (from[0] && quote[0]) snprintf(row.line, sizeof(row.line), "%s -> libanogs.so+0x%X  \"%s\"", from, siteOff, quote);
            else if (from[0]) snprintf(row.line, sizeof(row.line), "%s -> libanogs.so+0x%X", from, siteOff);
            else if (quote[0]) snprintf(row.line, sizeof(row.line), "libanogs.so+0x%X  \"%s\"", siteOff, quote);
            else snprintf(row.line, sizeof(row.line), "libanogs.so+0x%X", siteOff);
            gAnogsSlot[slot] = (uint32_t)gAnogsUsed + 1;
            gAnogsUsed++;
            break;
        }
        AnogsRow& row = gAnogsRows[mark - 1];
        if (row.site == siteOff && row.caller == caller) {
            row.count++;
            break;
        }
    }
    AnogsUnlock();
}

static AnogsSpan gAnoExec[8];
static int gAnoExecN = 0;
static AnogsSpan gCodeSpans[24];
static int gCodeN = 0;

static bool AnogsInSpans(uintptr_t p, const AnogsSpan* spans, int n) {
    for (int i = 0; i < n; ++i) if (p >= spans[i].a && p < spans[i].b) return true;
    return false;
}

static bool AnogsVmRead(void* dst, uintptr_t src, size_t n) {
    if (!dst || !src || !n) return false;
    struct iovec local;
    struct iovec remote;
    local.iov_base = dst;
    local.iov_len = n;
    remote.iov_base = (void*)src;
    remote.iov_len = n;
    return syscall(__NR_process_vm_readv, (long)getpid(), &local, 1L, &remote, 1L, 0L) == (long)n;
}

static void AnogsRefreshMaps() {
    AnogsSpan execs[8], codes[24], reads[16];
    int execN = 0, codeN = 0, readN = 0;
    FILE* fp = fopen("/proc/self/maps", "r");
    if (!fp) return;
    char line[768];
    while (fgets(line, sizeof(line), fp)) {
        unsigned long s = 0, e = 0;
        char perm[8] = {0};
        if (sscanf(line, "%lx-%lx %7s", &s, &e, perm) < 3 || e <= s) continue;
        bool ano = strstr(line, "libanogs.so") != nullptr;
        bool uni = strstr(line, "libunity.so") != nullptr;
        if (perm[2] == 'x' && e - s < 64ul * 1024 * 1024 && codeN < 24) {
            codes[codeN].a = (uintptr_t)s;
            codes[codeN].b = (uintptr_t)e;
            codeN++;
        }
        if (ano && perm[2] == 'x' && execN < 8) {
            execs[execN].a = (uintptr_t)s;
            execs[execN].b = (uintptr_t)e;
            execN++;
        }
        if ((ano || uni) && perm[0] == 'r' && readN < 16 && e - s < 64ul * 1024 * 1024) {
            reads[readN].a = (uintptr_t)s;
            reads[readN].b = (uintptr_t)e;
            readN++;
        }
    }
    fclose(fp);
    for (int i = 0; i < execN; ++i) gAnoExec[i] = execs[i];
    for (int i = 0; i < codeN; ++i) gCodeSpans[i] = codes[i];
    for (int i = 0; i < readN; ++i) gReadSpans[i] = reads[i];
    gAnoExecN = execN;
    gCodeN = codeN;
    gReadN = readN;
}

struct AnogsPub {
    AnogsSpan s[8];
    int n;
};
static AnogsPub gAnogsPub[2];
static std::atomic<int> gAnogsPubIdx{0};

struct AnogsSnap {
    uintptr_t site;
    uintptr_t caller;
    uintptr_t a0;
    uintptr_t a1;
    uintptr_t fp;
};
static AnogsSnap gAnogsSnap[1024];
static std::atomic<uint32_t> gAnogsSnapSeq[1024];
static std::atomic<uint32_t> gAnogsSnapW{0};
static uint32_t gAnogsSnapR = 0;
static struct sigaction gAnogsOldSig;
static std::atomic<int> gAnogsSigReady{0};

struct AnogsFunc {
    uintptr_t lo;
    uintptr_t hi;
    uintptr_t entry;
};
static AnogsFunc gAnogsFuncs[4096];
static int gAnogsFuncN = 0;

static void AnogsPublishExec() {
    int cur = gAnogsPubIdx.load(std::memory_order_relaxed) & 1;
    int next = cur ^ 1;
    int n = gAnoExecN;
    if (n > 8) n = 8;
    gAnogsPub[next].n = n;
    for (int i = 0; i < n; ++i) gAnogsPub[next].s[i] = gAnoExec[i];
    gAnogsPubIdx.store(next, std::memory_order_release);
}

static bool AnogsSigHit(uintptr_t pc) {
    int idx = gAnogsPubIdx.load(std::memory_order_acquire);
    const AnogsPub* pub = &gAnogsPub[idx & 1];
    for (int i = 0; i < pub->n; ++i) {
        if (pc >= pub->s[i].a && pc < pub->s[i].b) return true;
    }
    return false;
}

static void AnogsPushSnap(uintptr_t site, uintptr_t caller, uintptr_t a0, uintptr_t a1, uintptr_t fp) {
    uint32_t i = gAnogsSnapW.fetch_add(1, std::memory_order_relaxed) & 1023u;
    gAnogsSnapSeq[i].store(1, std::memory_order_relaxed);
    gAnogsSnap[i].site = site;
    gAnogsSnap[i].caller = caller;
    gAnogsSnap[i].a0 = a0;
    gAnogsSnap[i].a1 = a1;
    gAnogsSnap[i].fp = fp;
    gAnogsSnapSeq[i].store(2, std::memory_order_release);
}

static void AnogsOnSig(int sig, siginfo_t* info, void* ctx) {
#if defined(__aarch64__)
    ucontext_t* uc = (ucontext_t*)ctx;
    if (uc) {
        uintptr_t pc = (uintptr_t)uc->uc_mcontext.pc;
        if (AnogsSigHit(pc)) {
            AnogsPushSnap(pc,
                (uintptr_t)uc->uc_mcontext.regs[30],
                (uintptr_t)uc->uc_mcontext.regs[0],
                (uintptr_t)uc->uc_mcontext.regs[1],
                (uintptr_t)uc->uc_mcontext.regs[29]);
        }
    }
#else
    (void)ctx;
#endif
    if (gAnogsOldSig.sa_flags & SA_SIGINFO) {
        if (gAnogsOldSig.sa_sigaction) gAnogsOldSig.sa_sigaction(sig, info, ctx);
    } else if (gAnogsOldSig.sa_handler && gAnogsOldSig.sa_handler != SIG_DFL && gAnogsOldSig.sa_handler != SIG_IGN) {
        gAnogsOldSig.sa_handler(sig);
    }
}

static void AnogsInstallSig() {
    int expected = 0;
    if (!gAnogsSigReady.compare_exchange_strong(expected, 1)) return;
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    memset(&gAnogsOldSig, 0, sizeof(gAnogsOldSig));
    sa.sa_sigaction = AnogsOnSig;
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGWINCH);
    if (sigaction(SIGWINCH, &sa, &gAnogsOldSig) != 0) gAnogsSigReady.store(0);
}

static uintptr_t AnogsEntry(uintptr_t pc) {
    if (gAnoExecN == 0 || !AnogsInSpans(pc, gAnoExec, gAnoExecN)) return pc & ~(uintptr_t)3;
    uintptr_t aligned = pc & ~(uintptr_t)3;
    for (int i = 0; i < gAnogsFuncN; ++i) {
        if (aligned >= gAnogsFuncs[i].lo && aligned < gAnogsFuncs[i].hi) return gAnogsFuncs[i].entry;
    }
    uintptr_t spanA = 0, spanB = 0;
    for (int i = 0; i < gAnoExecN; ++i) {
        if (aligned >= gAnoExec[i].a && aligned < gAnoExec[i].b) {
            spanA = gAnoExec[i].a;
            spanB = gAnoExec[i].b;
            break;
        }
    }
    if (!spanB) return aligned;
    uintptr_t win = aligned > 0x3000 ? aligned - 0x3000 : spanA;
    if (win < spanA) win = spanA;
    win &= ~(uintptr_t)3;
    static uint8_t buf[0x3008];
    size_t n = (size_t)(aligned + 4 - win);
    if (n > sizeof(buf)) {
        win = (aligned + 4 - sizeof(buf)) & ~(uintptr_t)3;
        if (win < spanA) win = spanA;
        n = (size_t)(aligned + 4 - win);
        if (n > sizeof(buf)) n = sizeof(buf);
    }
    n &= ~(size_t)3;
    if (n < 4 || !AnogsVmRead(buf, win, n)) return aligned;
    uintptr_t entry = aligned;
    for (uintptr_t a = aligned; ; a -= 4) {
        uint32_t insn = 0;
        memcpy(&insn, buf + (size_t)(a - win), 4);
        if ((insn & 0xFFC07FFFu) == 0xA9807BFDu || (insn & 0xFFC07FFFu) == 0xA9007BFDu) {
            entry = a;
            if (a >= win + 4) {
                uint32_t prev = 0;
                memcpy(&prev, buf + (size_t)(a - 4 - win), 4);
                if (prev == 0xD503233Fu || prev == 0xD503245Fu) entry = a - 4;
            }
            break;
        }
        if (a <= win) break;
    }
    uintptr_t hi = entry + 4;
    uintptr_t end = spanB < entry + 0x1800 ? spanB : entry + 0x1800;
    static uint8_t fwd[0x1800];
    size_t fn = (size_t)(end - entry) & ~(size_t)3;
    if (fn >= 8 && fn <= sizeof(fwd) && AnogsVmRead(fwd, entry, fn)) {
        hi = entry + fn;
        for (size_t off = 4; off + 4 <= fn; off += 4) {
            uint32_t insn = 0;
            memcpy(&insn, fwd + off, 4);
            if (insn == 0xD65F03C0u) {
                hi = entry + off + 4;
                break;
            }
        }
    }
    if (hi <= entry) hi = entry + 4;
    if (gAnogsFuncN < 4096) {
        gAnogsFuncs[gAnogsFuncN].lo = entry;
        gAnogsFuncs[gAnogsFuncN].hi = hi;
        gAnogsFuncs[gAnogsFuncN].entry = entry;
        gAnogsFuncN++;
    }
    return entry;
}

static void AnogsNote(uintptr_t site, uintptr_t caller, uintptr_t a0, uintptr_t a1) {
    if (!site || !AnogsInSpans(site, gAnoExec, gAnoExecN)) return;
    if (caller && AnogsInSelf(caller)) caller = 0;
    AnogsOnCall(AnogsEntry(site), caller, a0, a1);
}

static void AnogsScanStack(uintptr_t start, uintptr_t end) {
    if (end <= start || end - start > 8ul * 1024 * 1024 || gAnoExecN == 0) return;
    static uint8_t chunk[32 * 1024];
    for (uintptr_t addr = start; addr + 8 <= end; ) {
        size_t n = (size_t)(end - addr);
        if (n > sizeof(chunk)) n = sizeof(chunk);
        n &= ~((size_t)7);
        if (n < 8) break;
        if (!AnogsVmRead(chunk, addr, n)) {
            addr += 4096;
            continue;
        }
        for (size_t i = 0; i + 8 <= n; i += 8) {
            uintptr_t v = 0;
            memcpy(&v, chunk + i, 8);
            if (!AnogsInSpans(v, gAnoExec, gAnoExecN)) continue;
            uintptr_t caller = 0;
            uintptr_t inside = 0;
            uintptr_t text = 0;
            for (int d = -16; d <= 16; d += 8) {
                if (d == 0) continue;
                if (d < 0 && i < (size_t)(-d)) continue;
                if (d > 0 && i + (size_t)d + 8 > n) continue;
                uintptr_t near = 0;
                memcpy(&near, chunk + i + d, 8);
                if (near != v && AnogsInSpans(near, gCodeSpans, gCodeN)) {
                    if (!AnogsInSpans(near, gAnoExec, gAnoExecN)) {
                        if (!caller) caller = near;
                    } else if (!inside) {
                        inside = near;
                    }
                }
                if (!text && AnogsMapped(near)) text = near;
            }
            if (!caller) caller = inside;
            AnogsNote(v, caller, text, 0);
        }
        addr += n;
    }
}

static void AnogsWalkFp(uintptr_t fp, uintptr_t caller) {
    for (int i = 0; i < 20; ++i) {
        if (!fp || (fp & 7)) break;
        uintptr_t words[2] = {0, 0};
        if (!AnogsVmRead(words, fp, sizeof(words))) break;
        uintptr_t prev = words[0];
        uintptr_t lr = words[1];
        if (AnogsInSpans(caller, gAnoExec, gAnoExecN)) AnogsNote(caller, lr, 0, 0);
        if (!prev || prev <= fp || prev - fp > 0x40000) break;
        caller = lr;
        fp = prev;
    }
}

static void AnogsDrain() {
    uint32_t w = gAnogsSnapW.load(std::memory_order_acquire);
    if (w - gAnogsSnapR > 1024u) gAnogsSnapR = w - 1024u;
    while (gAnogsSnapR != w) {
        uint32_t i = gAnogsSnapR & 1023u;
        if (gAnogsSnapSeq[i].load(std::memory_order_acquire) != 2) break;
        AnogsSnap snap = gAnogsSnap[i];
        gAnogsSnapSeq[i].store(0, std::memory_order_relaxed);
        gAnogsSnapR++;
        AnogsNote(snap.site, snap.caller, snap.a0, snap.a1);
        AnogsWalkFp(snap.fp, snap.caller);
        w = gAnogsSnapW.load(std::memory_order_acquire);
    }
}

static int AnogsParseTid(const char* s) {
    int n = 0;
    if (!s || *s < '0' || *s > '9') return 0;
    for (; *s >= '0' && *s <= '9'; ++s) n = n * 10 + (*s - '0');
    return n;
}

static void AnogsPokeThreads() {
    if (gAnogsSigReady.load(std::memory_order_acquire) != 1 || gAnoExecN == 0) return;
    int self = (int)syscall(__NR_gettid);
    DIR* d = opendir("/proc/self/task");
    if (!d) return;
    int sent = 0;
    struct dirent* de;
    while ((de = readdir(d)) != nullptr) {
        int tid = AnogsParseTid(de->d_name);
        if (tid <= 0 || tid == self) continue;
        if (syscall(__NR_tgkill, (long)getpid(), (long)tid, SIGWINCH) == 0) sent++;
        if (sent >= 64) break;
    }
    closedir(d);
}

static bool AnogsStackSize(size_t len) {
    if (len < 128ul * 1024 || len > 1024ul * 1024 + 128ul * 1024) return false;
    size_t p = 128ul * 1024;
    for (;;) {
        size_t lo = p > 64ul * 1024 ? p - 64ul * 1024 : p;
        if (len >= lo && len <= p + 64ul * 1024) return true;
        if (p >= 1024ul * 1024) break;
        p *= 2;
    }
    return false;
}

static void AnogsScanStacks() {
    FILE* fp = fopen("/proc/self/maps", "r");
    if (!fp) return;
    char line[768];
    size_t budget = 32ul * 1024 * 1024;
    while (fgets(line, sizeof(line), fp) && budget > 0) {
        unsigned long s = 0, e = 0;
        char perm[8] = {0};
        if (sscanf(line, "%lx-%lx %7s", &s, &e, perm) < 3 || e <= s) continue;
        if (perm[0] != 'r' || perm[1] != 'w' || perm[2] == 'x') continue;
        size_t len = (size_t)(e - s);
        if (len < 64 * 1024 || len > 8ul * 1024 * 1024 || len > budget) continue;
        bool named = strstr(line, "[stack]") || strstr(line, "stack_and_tls") || strstr(line, "anon:stack");
        bool heapish = strstr(line, "[heap]") || strstr(line, "malloc") || strstr(line, "scudo")
            || strstr(line, "dalvik") || strstr(line, "jit") || strstr(line, "ashmem")
            || strstr(line, "linker") || strstr(line, ".so");
        bool anon = strchr(line, '/') == nullptr;
        if (heapish || (!named && !(anon && AnogsStackSize(len)))) continue;
        AnogsScanStack((uintptr_t)s, (uintptr_t)e);
        budget -= len;
    }
    fclose(fp);
}

static bool AnogsMonitorOn() {
    if (!kCallMonitor) return false;
    return checkboxStates["ANOGS::MONITOR"];
}

static void AnogsSampleSelf() {
    static int tick = 0;
    if (!AnogsMonitorOn() || (++tick & 31) != 0 || gAnoExecN == 0) return;
    int mark = 0;
    uintptr_t anchor = (uintptr_t)&mark;
    uintptr_t fp = (uintptr_t)__builtin_frame_address(0);
    for (int i = 0; i < 16; ++i) {
        if (!fp || (fp & 7) || fp + 16 < anchor || fp > anchor + 0x40000) break;
        uintptr_t prev = 0, lr = 0;
        memcpy(&prev, (void*)fp, 8);
        memcpy(&lr, (void*)(fp + 8), 8);
        if (AnogsInSpans(lr, gAnoExec, gAnoExecN)) AnogsOnCall(lr, 0, 0, 0);
        if (prev <= fp) break;
        fp = prev;
    }
}

static void AnogsWatch() {
    sleep(5);
    int phase = 0;
    for (;;) {
        if (!AnogsMonitorOn()) {
            usleep(200 * 1000);
            continue;
        }
        if (gAnogsSigReady.load(std::memory_order_acquire) != 1) AnogsInstallSig();
        AnogsRefreshMaps();
        AnogsPublishExec();
        AnogsDrain();
        if ((phase % 8) == 0) AnogsScanStacks();
        AnogsPokeThreads();
        phase++;
        usleep(40 * 1000);
    }
}

static int hook_anogs_0() { if constexpr (kCallMonitor) AnogsOnCall(libanogs_base + 0x204218, (uintptr_t)__builtin_return_address(0), 0, 0); return 0; }
static int hook_anogs_1() { if constexpr (kCallMonitor) AnogsOnCall(libanogs_base + 0x45B334, (uintptr_t)__builtin_return_address(0), 0, 0); return 0; }
static int hook_anogs_2() { if constexpr (kCallMonitor) AnogsOnCall(libanogs_base + 0x44A714, (uintptr_t)__builtin_return_address(0), 0, 0); return 0; }
static int hook_anogs_3() { if constexpr (kCallMonitor) AnogsOnCall(libanogs_base + 0x48F964, (uintptr_t)__builtin_return_address(0), 0, 0); return 0; }

static void AnogsCopyRows(const AnogsRow* view, int used) {
    std::string blob;
    blob.reserve((size_t)used * 96);
    for (int i = 0; i < used; ++i) {
        char head[240];
        snprintf(head, sizeof(head), "x%u  %s\n", view[i].count, view[i].line);
        blob += head;
    }
    ImGui::SetClipboardText(blob.c_str());
    const char* paths[] = {
        "/sdcard/anogs_calls.txt",
        "/storage/emulated/0/anogs_calls.txt",
        "/sdcard/Android/data/com.garena.game.codm/files/anogs_calls.txt"
    };
    for (const char* path : paths) {
        FILE* out = fopen(path, "w");
        if (!out) continue;
        fwrite(blob.data(), 1, blob.size(), out);
        fclose(out);
        snprintf(gAnogsCopyMsg, sizeof(gAnogsCopyMsg), "Copied %d  %s", used, path);
        return;
    }
    snprintf(gAnogsCopyMsg, sizeof(gAnogsCopyMsg), "Copied %d", used);
}

static void DrawAnogsTab() {
    if (!kCallMonitor) return;
    static AnogsRow view[2048];
    static int used = 0;
    static uint32_t hits = 0;
    if (AnogsLock()) {
        hits = gAnogsHits.load(std::memory_order_relaxed);
        used = gAnogsUsed;
        for (int i = 0; i < used; ++i) view[i] = gAnogsRows[i];
        AnogsUnlock();
    }

    ImGui::BeginChild("##AnogsTab", ImVec2(0, 0), true);
    ImGui::Checkbox("Full library monitor", checkboxStates["ANOGS::MONITOR"].ptr());
    ImGui::Text("hits %u   callers %d", hits, used);
    ImGui::TextUnformatted("caller -> libanogs function  \"string\"");
    ImGui::TextUnformatted("Monitor starts off. The 4 patches stay on when it is off.");
    if (ImGui::Button("Copy all")) AnogsCopyRows(view, used);
    ImGui::SameLine();
    if (ImGui::Button("Clear")) {
        if (AnogsLock()) {
            gAnogsUsed = 0;
            gAnogsHits.store(0, std::memory_order_relaxed);
            memset(gAnogsSlot, 0, sizeof(gAnogsSlot));
            AnogsUnlock();
        }
    }
    if (gAnogsCopyMsg[0]) ImGui::TextWrapped("%s", gAnogsCopyMsg);
    ImGui::Separator();
    ImGui::BeginChild("##AnogsLog", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    if (used == 0) {
        ImGui::TextUnformatted("Waiting for libanogs calls.");
    } else {
        ImGuiListClipper clipper;
        clipper.Begin(used);
        while (clipper.Step()) {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
                ImGui::PushID(i);
                char head[240];
                snprintf(head, sizeof(head), "x%u  %s", view[i].count, view[i].line);
                if (ImGui::Selectable(head, false)) {
                    ImGui::SetClipboardText(view[i].line);
                    snprintf(gAnogsCopyMsg, sizeof(gAnogsCopyMsg), "Copied 1 line");
                }
                ImGui::PopID();
            }
        }
    }
    ImGui::EndChild();
    ImGui::EndChild();
}


//FPSHIGH
extern std::atomic<bool> gMatchLive;
static int (*orig_GetFrameRateValue)(void* instance, int fl, int mode) = nullptr;
static int hook_GetFrameRateValue(void* instance, int fl, int mode) {
    if (!orig_GetFrameRateValue) return 60;
    if (instance != nullptr && checkboxStates["FRAME120"]) return 120;
    if (instance != nullptr && checkboxStates["FRAME40"]) return 40;
    return orig_GetFrameRateValue(instance, fl, mode);
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
    if constexpr (kCallMonitor) AnogsSampleSelf();
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

static constexpr int kWallShotCount = 8;
static constexpr float kWallShotDist = 40.f;

struct WallSnap {
    uintptr_t addr;
    int walls;
    int pve;
    float dist;
};
static WallSnap gWallSnap[2];

static bool WallShotIsInstant(void* instance) {
    if (!Tools::IsPtrValid(instance)) return false;
    auto* obj = reinterpret_cast<Il2CppObject*>(instance);
    if (Tools::IsPtrValid(obj->klass)) {
        const char* name = obj->klass->_1.name;
        if (name && Tools::IsPtrValid((void*)name) && Tools::IsPtrValid((void*)(name + 24))) {
            const char* prefix = "WeaponFireComponent_Instant";
            bool match = true;
            for (int i = 0; prefix[i]; ++i) {
                if (name[i] != prefix[i]) { match = false; break; }
            }
            if (match) return true;
        }
    }
    uintptr_t addr = (uintptr_t)instance;
    if (!Tools::IsPtrValid((void*)(addr + 0x3F4))) return false;
    int marker = *reinterpret_cast<int*>(addr + 0x364);
    int walls = *reinterpret_cast<int*>(addr + 0x3EC);
    if (marker < 0x40000000 || marker >= 0x44800000) return false;
    return walls >= 0 && walls <= kWallShotCount;
}

static void WallShotRestoreSlot(WallSnap& slot) {
    if (!slot.addr) return;
    if (Tools::IsPtrValid((void*)(slot.addr + 0x3F4))) {
        *reinterpret_cast<int*>(slot.addr + 0x3EC) = slot.walls;
        *reinterpret_cast<int*>(slot.addr + 0x3F0) = slot.pve;
        *reinterpret_cast<float*>(slot.addr + 0x3F4) = slot.dist;
    }
    slot.addr = 0;
}

static void WallShotRestoreAll() {
    WallShotRestoreSlot(gWallSnap[0]);
    WallShotRestoreSlot(gWallSnap[1]);
}

static void WallShotKeep(void* fire, bool fromFire) {
    if (!fromFire && !WallShotIsInstant(fire)) return;
    uintptr_t addr = (uintptr_t)fire;
    if (!Tools::IsPtrValid((void*)(addr + 0x3F4))) return;
    WallSnap* slot = nullptr;
    for (int i = 0; i < 2; ++i) {
        if (gWallSnap[i].addr == addr) slot = &gWallSnap[i];
    }
    if (!slot) {
        for (int i = 0; i < 2; ++i) {
            if (!gWallSnap[i].addr) { slot = &gWallSnap[i]; break; }
        }
    }
    if (!slot) return;
    if (slot->addr != addr) {
        int walls = *reinterpret_cast<int*>(addr + 0x3EC);
        int pve = *reinterpret_cast<int*>(addr + 0x3F0);
        float dist = *reinterpret_cast<float*>(addr + 0x3F4);
        if (walls == kWallShotCount && pve == kWallShotCount && dist == kWallShotDist) {
            walls = 0;
            pve = 0;
            dist = 0.f;
        }
        slot->addr = addr;
        slot->walls = walls;
        slot->pve = pve;
        slot->dist = dist;
    }
    *reinterpret_cast<int*>(addr + 0x3EC) = kWallShotCount;
    *reinterpret_cast<int*>(addr + 0x3F0) = kWallShotCount;
    *reinterpret_cast<float*>(addr + 0x3F4) = kWallShotDist;
}

static void* (*wall_get_Fire)(void*) = nullptr;
static void* (*wall_get_CurrentFire)(void*) = nullptr;

static void ApplyWallShot() {
    if (!checkboxStates["WALL::SHOT"]) {
        WallShotRestoreAll();
        return;
    }
    auto localFn = FastFuncs::Gameplay_get_LocalPawn_Fast;
    auto weaponFn = FastFuncs::Pawn_get_CurrentWeapon_Fast;
    if (!localFn || !weaponFn) return;
    Pawn* pawn = localFn();
    if (!Tools::IsPtrValid(pawn)) return;
    void* weapon = weaponFn(pawn);
    if (!Tools::IsPtrValid(weapon)) return;
    if (!wall_get_Fire) {
        uintptr_t base = libunity_base ? libunity_base : (uintptr_t)getAbsoluteAddress(DOS("libunity.so"), 0);
        if (!base) return;
        wall_get_Fire = (void* (*)(void*))(base + 0x546E818);
        wall_get_CurrentFire = (void* (*)(void*))(base + 0x5472E48);
    }
    void* primary = wall_get_Fire ? wall_get_Fire(weapon) : nullptr;
    void* current = wall_get_CurrentFire ? wall_get_CurrentFire(weapon) : nullptr;
    for (int i = 0; i < 2; ++i) {
        if (!gWallSnap[i].addr) continue;
        if (gWallSnap[i].addr == (uintptr_t)primary || gWallSnap[i].addr == (uintptr_t)current) continue;
        WallShotRestoreSlot(gWallSnap[i]);
    }
    if (primary) WallShotKeep(primary, false);
    if (current && current != primary) WallShotKeep(current, false);
}

struct LowGfxSaved {
    bool have;
    int tex;
    int shadows;
    float shadowDist;
    int lights;
    int aniso;
    int unityQual;
    int sysLevel;
    bool sky;
    bool bloomBr, bloomMp, bloomPve;
    bool godBr, godPve;
    bool waterBr, waterPve;
};
static LowGfxSaved gLowGfx;

typedef void* (*LowGfxSysInstFn)();
typedef bool (*LowGfxSetSysLevelFn)(void*, int, bool);
typedef int (*LowGfxGetSysLevelFn)(void*);
static LowGfxSysInstFn gLowGfxSysInst = nullptr;
static LowGfxSetSysLevelFn gLowGfxSetSysLevel = nullptr;
static LowGfxGetSysLevelFn gLowGfxGetSysLevel = nullptr;

static bool LowGfxResolveSystemSetting() {
    if (gLowGfxSysInst) return true;
    static const char* images[] = {"Assembly-CSharp", "Assembly-CSharp.dll"};
    for (const char* image : images) {
        void* getInst = Il2CppGetMethodOffset(image, "GameEngine", "SystemSetting", "get_Instance", 0);
        void* setLvl = Il2CppGetMethodOffset(image, "GameEngine", "SystemSetting", "SetUserSettingLevel", 2);
        void* getLvl = Il2CppGetMethodOffset(image, "GameEngine", "SystemSetting", "get_ActiveSettingLevel", 0);
        if (getInst && setLvl && getLvl) {
            gLowGfxSysInst = (LowGfxSysInstFn)getInst;
            gLowGfxSetSysLevel = (LowGfxSetSysLevelFn)setLvl;
            gLowGfxGetSysLevel = (LowGfxGetSysLevelFn)getLvl;
            return true;
        }
    }
    return false;
}

struct LowGfxFns {
    bool ready;
    int (*getTex)();
    void (*setTex)(int);
    int (*getShadows)();
    void (*setShadows)(int);
    float (*getShadowDist)();
    void (*setShadowDist)(float);
    int (*getLights)();
    void (*setLights)(int);
    int (*getAniso)();
    void (*setAniso)(int);
    int (*getQual)();
    void (*setQual)(int, bool);
    bool (*getSky)();
    void (*setSky)(bool);
    bool (*getBloomBr)();
    void (*setBloomBr)(bool);
    bool (*getBloomMp)();
    void (*setBloomMp)(bool);
    bool (*getBloomPve)();
    void (*setBloomPve)(bool);
    bool (*getGodBr)();
    void (*setGodBr)(bool);
    bool (*getGodPve)();
    void (*setGodPve)(bool);
    bool (*getWaterBr)();
    void (*setWaterBr)(bool);
    bool (*getWaterPve)();
    void (*setWaterPve)(bool);
};
static LowGfxFns gLowGfxFns;

static bool LowGfxBind() {
    if (gLowGfxFns.ready) return true;
    uintptr_t base = libunity_base ? libunity_base : (uintptr_t)getAbsoluteAddress(DOS("libunity.so"), 0);
    if (!base) return false;
    gLowGfxFns.getTex = (int (*)())(base + 0x53295B4);
    gLowGfxFns.setTex = (void (*)(int))(base + 0x53295C4);
    gLowGfxFns.getShadows = (int (*)())(base + 0x532936C);
    gLowGfxFns.setShadows = (void (*)(int))(base + 0x532937C);
    gLowGfxFns.getShadowDist = (float (*)())(base + 0x5329420);
    gLowGfxFns.setShadowDist = (void (*)(float))(base + 0x5329430);
    gLowGfxFns.getLights = (int (*)())(base + 0x5329314);
    gLowGfxFns.setLights = (void (*)(int))(base + 0x5329324);
    gLowGfxFns.getAniso = (int (*)())(base + 0x53295E0);
    gLowGfxFns.setAniso = (void (*)(int))(base + 0x53295F0);
    gLowGfxFns.getQual = (int (*)())(base + 0x532929C);
    gLowGfxFns.setQual = (void (*)(int, bool))(base + 0x53292AC);
    gLowGfxFns.getSky = (bool (*)())(base + 0x52ED91C);
    gLowGfxFns.setSky = (void (*)(bool))(base + 0x52ED92C);
    gLowGfxFns.getBloomBr = (bool (*)())(base + 0xAFBD104);
    gLowGfxFns.setBloomBr = (void (*)(bool))(base + 0xAFBD160);
    gLowGfxFns.getBloomMp = (bool (*)())(base + 0xAFBD358);
    gLowGfxFns.setBloomMp = (void (*)(bool))(base + 0xAFBD3B4);
    gLowGfxFns.getBloomPve = (bool (*)())(base + 0xAFBD5AC);
    gLowGfxFns.setBloomPve = (void (*)(bool))(base + 0xAFBD608);
    gLowGfxFns.getGodBr = (bool (*)())(base + 0xAFBEBD4);
    gLowGfxFns.setGodBr = (void (*)(bool))(base + 0xAFBEC30);
    gLowGfxFns.getGodPve = (bool (*)())(base + 0xAFBEE28);
    gLowGfxFns.setGodPve = (void (*)(bool))(base + 0xAFBEE84);
    gLowGfxFns.getWaterBr = (bool (*)())(base + 0xAFBE540);
    gLowGfxFns.setWaterBr = (void (*)(bool))(base + 0xAFBE59C);
    gLowGfxFns.getWaterPve = (bool (*)())(base + 0xAFBE734);
    gLowGfxFns.setWaterPve = (void (*)(bool))(base + 0xAFBE790);
    gLowGfxFns.ready = true;
    return true;
}

static void LowGfxRestore() {
    if (!gLowGfx.have || !gLowGfxFns.ready) return;
    gLowGfxFns.setTex(gLowGfx.tex);
    if (gLowGfxFns.setQual) gLowGfxFns.setQual(gLowGfx.unityQual, true);
    if (gLowGfx.sysLevel > 0 && gLowGfxSysInst && gLowGfxSetSysLevel) {
        void* sys = gLowGfxSysInst();
        if (sys) gLowGfxSetSysLevel(sys, gLowGfx.sysLevel, false);
    }
    gLowGfxFns.setShadows(gLowGfx.shadows);
    gLowGfxFns.setShadowDist(gLowGfx.shadowDist);
    gLowGfxFns.setLights(gLowGfx.lights);
    gLowGfxFns.setAniso(gLowGfx.aniso);
    gLowGfxFns.setSky(gLowGfx.sky);
    gLowGfxFns.setBloomBr(gLowGfx.bloomBr);
    gLowGfxFns.setBloomMp(gLowGfx.bloomMp);
    gLowGfxFns.setBloomPve(gLowGfx.bloomPve);
    gLowGfxFns.setGodBr(gLowGfx.godBr);
    gLowGfxFns.setGodPve(gLowGfx.godPve);
    gLowGfxFns.setWaterBr(gLowGfx.waterBr);
    gLowGfxFns.setWaterPve(gLowGfx.waterPve);
    gLowGfx.have = false;
}

static void ApplyLowGfx() {
    if (!LowGfxBind()) return;
    if (!checkboxStates["LOWGFX"]) {
        LowGfxRestore();
        return;
    }
    float t = seekbarValues["LOW::GFX"] / 100.f;
    if (t < 0.f) t = 0.f;
    if (t > 1.f) t = 1.f;
    if (!gLowGfx.have) {
        gLowGfx.tex = gLowGfxFns.getTex();
        gLowGfx.shadows = gLowGfxFns.getShadows();
        gLowGfx.shadowDist = gLowGfxFns.getShadowDist();
        gLowGfx.lights = gLowGfxFns.getLights();
        gLowGfx.aniso = gLowGfxFns.getAniso();
        gLowGfx.unityQual = gLowGfxFns.getQual ? gLowGfxFns.getQual() : 0;
        gLowGfx.sysLevel = 0;
        if (LowGfxResolveSystemSetting() && gLowGfxSysInst && gLowGfxGetSysLevel) {
            void* sys = gLowGfxSysInst();
            if (sys) gLowGfx.sysLevel = gLowGfxGetSysLevel(sys);
        }
        gLowGfx.sky = gLowGfxFns.getSky();
        gLowGfx.bloomBr = gLowGfxFns.getBloomBr();
        gLowGfx.bloomMp = gLowGfxFns.getBloomMp();
        gLowGfx.bloomPve = gLowGfxFns.getBloomPve();
        gLowGfx.godBr = gLowGfxFns.getGodBr();
        gLowGfx.godPve = gLowGfxFns.getGodPve();
        gLowGfx.waterBr = gLowGfxFns.getWaterBr();
        gLowGfx.waterPve = gLowGfxFns.getWaterPve();
        gLowGfx.have = true;
    }
    int tex = (int)(t * 2.f + 0.5f);
    if (tex < 0) tex = 0;
    if (tex > 2) tex = 2;
    gLowGfxFns.setTex(tex);
    if (gLowGfxFns.setQual) {
        if (t >= 0.25f) gLowGfxFns.setQual(0, true);
        else gLowGfxFns.setQual(gLowGfx.unityQual, false);
    }
    if (LowGfxResolveSystemSetting() && t >= 0.34f) {
        void* sys = gLowGfxSysInst();
        if (sys && gLowGfxSetSysLevel) {
            int lvl = t >= 0.67f ? 1 : 2; // UserSettingLevel VeryLow / Low in 1.6.59 dump
            gLowGfxSetSysLevel(sys, lvl, false);
        }
    }
    gLowGfxFns.setAniso(t >= 0.34f ? 0 : gLowGfx.aniso);
    bool flat = t >= 0.34f;
    gLowGfxFns.setBloomBr(flat ? false : gLowGfx.bloomBr);
    gLowGfxFns.setBloomMp(flat ? false : gLowGfx.bloomMp);
    gLowGfxFns.setBloomPve(flat ? false : gLowGfx.bloomPve);
    gLowGfxFns.setGodBr(flat ? false : gLowGfx.godBr);
    gLowGfxFns.setGodPve(flat ? false : gLowGfx.godPve);
    gLowGfxFns.setWaterBr(flat ? false : gLowGfx.waterBr);
    gLowGfxFns.setWaterPve(flat ? false : gLowGfx.waterPve);
    bool brutal = t >= 0.67f;
    gLowGfxFns.setShadows(brutal ? 0 : gLowGfx.shadows);
    gLowGfxFns.setShadowDist(brutal ? 0.f : gLowGfx.shadowDist);
    gLowGfxFns.setLights(brutal ? 0 : gLowGfx.lights);
}

static int (*orig_get_MaxThroughWall)(void* instance);
static int hook_get_MaxThroughWall(void* instance) {
    if (!orig_get_MaxThroughWall) return 0;
    if (instance && checkboxStates["WALL::SHOT"]) {
        uintptr_t addr = (uintptr_t)instance;
        for (int i = 0; i < 2; ++i) {
            if (gWallSnap[i].addr == addr) {
                WallShotKeep(instance, true);
                return kWallShotCount;
            }
        }
        auto localFn = FastFuncs::Gameplay_get_LocalPawn_Fast;
        void* local = localFn ? (void*)localFn() : nullptr;
        void* owner = Tools::IsPtrValid((void*)(addr + 0x28))
            ? *reinterpret_cast<void**>(addr + 0x28) : nullptr;
        if (local && owner == local) {
            WallShotKeep(instance, true);
            return kWallShotCount;
        }
    }
    return orig_get_MaxThroughWall(instance);
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
    static void* anogsOrig[4] = {};
    DOSHOOK((void*)(libanogs_base + GET_OFF("0x204218")), (void*)hook_anogs_0, (void**)&anogsOrig[0]);
    DOSHOOK((void*)(libanogs_base + GET_OFF("0x45B334")), (void*)hook_anogs_1, (void**)&anogsOrig[1]);
    DOSHOOK((void*)(libanogs_base + GET_OFF("0x44A714")), (void*)hook_anogs_2, (void**)&anogsOrig[2]);
    DOSHOOK((void*)(libanogs_base + GET_OFF("0x48F964")), (void*)hook_anogs_3, (void**)&anogsOrig[3]);

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

// Wall shot: local gun keeps traveling after cover
DOSHOOK((void*)(libunity_base + GET_OFF("0xD529DDC")), (void*)hook_get_MaxThroughWall, (void**)&orig_get_MaxThroughWall);

// No Shake
DOSHOOK((void*)(libunity_base + GET_OFF("0x669780C")), (void*)&hook_UpdateCameraShake, (void**)&orig_UpdateCameraShake);

// Hitbox
DOSHOOK((void*)(libunity_base + GET_OFF("0xC2BE0C8")), (void*)&hook_SingleLineCheckPhysics, (void**)&orig_SingleLineCheckPhysics);

    if constexpr (kCallMonitor) std::thread(AnogsWatch).detach();
}
