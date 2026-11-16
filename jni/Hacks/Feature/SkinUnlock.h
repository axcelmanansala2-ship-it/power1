#pragma once
// CODM 1.6.59 skin catalog. Field offsets are absolute on the config object
// (dump.cs already includes the object header).
//
// 1.6.59 changes vs the old unlock source:
//   WeaponFireEffectConfigConfig.Id            0x84 -> 0x80
//   AssetIdForBulletSmoke1P                    0x18 -> 0x1C
//   ItemResourceConfig sprites are ushort indexes, not string pointers.
//     FxAssetID 0x18, ID 0x1C, InventoryModelID 0x20, ModelAssetIDRaw 0x24
//   WeaponConfConfig.IsDouble is new at 0x44.
//     WinnerCircleWeaponMountType moved to 0x48 and is not written.
//   WeaponConfConfig.get_Name                  0x85D6744
//   RoleConfConfig (characters): BaseCharacterID 0x10, ID 0x34,
//     ColorID 0x22, RolePackID 0x44, get_Name 0xBAE8444
//   CharacterModelConfig.ItemID ulong 0x78, mesh ints 0x18..0x44
//   ItemResourceConfig.AvatarModelID ulong 0x28

#include <atomic>
#include <cstdint>
#include <cctype>
#include <cstring>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <pthread.h>
#include <unistd.h>

static constexpr uintptr_t kSkinRvaItem2Inventory = 0x73BAC94;
static constexpr uintptr_t kSkinRvaWeaponConf     = 0x85D5120;
static constexpr uintptr_t kSkinRvaWeaponExtra    = 0x85D6A68;
static constexpr uintptr_t kSkinRvaWeaponFire     = 0x85D8664;
static constexpr uintptr_t kSkinRvaWeaponAsset    = 0x85D0488;
static constexpr uintptr_t kSkinRvaMythicArmor    = 0x8163400;
static constexpr uintptr_t kSkinRvaMythicSight    = 0x8163A08;
static constexpr uintptr_t kSkinRvaKillEffect     = 0x8148794;
static constexpr uintptr_t kSkinRvaItemResource   = 0x8145590;
static constexpr uintptr_t kSkinRvaWeaponName     = 0x85D6744;
static constexpr uintptr_t kSkinRvaRoleConf       = 0xBAE6244;
static constexpr uintptr_t kSkinRvaRoleName       = 0xBAE8444;
static constexpr uintptr_t kSkinRvaCharModel      = 0xA30A6B8;

struct SkinRow {
    std::string name;
    std::string low;
    void* conf = nullptr;
    bool named = false;
    int baseId = 0;
    int color = 0;
    int skinId = 0;
    int broadcast = 0;
    int assetGroup = 0;
    int icon = 0;
    int mythicArmor = 0;
    int mythicSight = 0;
    int deadReplay = 0;
    int killEffect = 0;
    int assetId = 0;
    int fireId = 0;
    int fireOrig = 0;
    int fx = 0;
    int invModel = 0;
    int modelRaw = 0;
    int kind = 0;
    int arm = 0;
    int suit = 0;
    int holo = 0;
    int pack = 0;
    int hasModel = 0;
    int modelF[12] = {};
    uint64_t avatar = 0;
};

static std::vector<void*> gSkinInv, gSkinExtra, gSkinFire, gSkinConf, gSkinAsset;
static std::vector<void*> gSkinArmor, gSkinSight, gSkinKill, gSkinRes;
static std::vector<void*> gSkinRole, gSkinModel;
static pthread_mutex_t gSkinMu = PTHREAD_MUTEX_INITIALIZER;
static std::vector<SkinRow> gSkinRows;
static std::atomic<int> gSkinState{0};
static int gSkinNameCursor = 0;
static int gSkinCharCursor = 0;
static char gSkinSearch[64] = "";
static std::vector<char> gSkinPicked;

struct SkinHookSlot {
    void (*orig)(void*);
    std::vector<void*>* vec;
};
static SkinHookSlot gSkinSlots[12];

template <int N>
static void SkinCtorHook(void* self) {
    if (self && gSkinSlots[N].vec && gSkinSlots[N].vec->size() < 50000) {
        if (pthread_mutex_trylock(&gSkinMu) == 0) {
            if (gSkinSlots[N].vec->size() < 50000)
                gSkinSlots[N].vec->push_back(self);
            pthread_mutex_unlock(&gSkinMu);
        }
    }
    if (gSkinSlots[N].orig) gSkinSlots[N].orig(self);
}

static bool SkinRdOk(void* p, uintptr_t off) {
    if (!p) return false;
    return Tools::IsPtrValid((void*)((uintptr_t)p + off));
}

static int SkinRdI(void* p, uintptr_t off) {
    if (!SkinRdOk(p, off)) return 0;
    return *(int*)((uintptr_t)p + off);
}

static uint64_t SkinRdU64(void* p, uintptr_t off) {
    if (!SkinRdOk(p, off) || !SkinRdOk(p, off + 4)) return 0;
    return *(uint64_t*)((uintptr_t)p + off);
}

static uint8_t SkinRdB(void* p, uintptr_t off) {
    if (!SkinRdOk(p, off)) return 0;
    return *(uint8_t*)((uintptr_t)p + off);
}

static void SkinWrI(void* p, uintptr_t off, int v) {
    if (!SkinRdOk(p, off)) return;
    *(int*)((uintptr_t)p + off) = v;
}

static void SkinWrB(void* p, uintptr_t off, uint8_t v) {
    if (!SkinRdOk(p, off)) return;
    *(uint8_t*)((uintptr_t)p + off) = v;
}

static void SkinWrU64(void* p, uintptr_t off, uint64_t v) {
    if (!SkinRdOk(p, off) || !SkinRdOk(p, off + 4)) return;
    *(uint64_t*)((uintptr_t)p + off) = v;
}

static int SkinLastInt(void* obj, uintptr_t off) {
    if (!SkinRdOk(obj, off)) return 0;
    auto arr = *(Array<int>**)((uintptr_t)obj + off);
    if (!Tools::IsPtrValid(arr)) return 0;
    int n = arr->getLength();
    if (n <= 0 || n > 64) return 0;
    return arr->m_Items[n - 1];
}

static std::string SkinConfName(void* obj, uintptr_t rva = kSkinRvaWeaponName) {
    if (!libunity_base || !rva || !Tools::IsPtrValid(obj)) return {};
    auto fn = (String* (*)(void*))(libunity_base + rva);
    String* s = fn(obj);
    if (!Tools::IsPtrValid(s)) return {};
    if (s->length <= 0 || s->length > 48) return {};
    const char* u = s->CString();
    if (!u || !u[0]) return {};
    return std::string(u);
}

static std::string SkinTake(const char* s, int cap) {
    std::string o;
    if (!s || cap <= 0) return o;
    for (int i = 0; i < cap && s[i]; ++i) {
        unsigned char c = (unsigned char)s[i];
        if (c < 32 || c == '#' || c == '%') continue;
        o.push_back((char)c);
    }
    return o;
}

static std::string SkinLower(const std::string& in) {
    std::string o = SkinTake(in.c_str(), 48);
    for (char& c : o) c = (char)std::tolower((unsigned char)c);
    return o;
}

static const char* SkinTag(int color) {
    if (color == 5) return "[M] ";
    if (color == 4) return "[L] ";
    if (color == 3) return "[E] ";
    return "[C] ";
}

static void SkinHookOne(int slot, uintptr_t rva, std::vector<void*>* vec, void* fn) {
    gSkinSlots[slot].vec = vec;
    gSkinSlots[slot].orig = nullptr;
    if (!libunity_base || !rva) return;
    DOSHOOK((void*)(libunity_base + rva), fn, (void**)&gSkinSlots[slot].orig);
}

static void SkinInstallHooks() {
    static bool once = false;
    if (once || !libunity_base) return;
    once = true;
    SkinHookOne(0, kSkinRvaItem2Inventory, &gSkinInv, (void*)SkinCtorHook<0>);
    SkinHookOne(1, kSkinRvaWeaponExtra, &gSkinExtra, (void*)SkinCtorHook<1>);
    SkinHookOne(2, kSkinRvaWeaponFire, &gSkinFire, (void*)SkinCtorHook<2>);
    SkinHookOne(3, kSkinRvaWeaponConf, &gSkinConf, (void*)SkinCtorHook<3>);
    SkinHookOne(4, kSkinRvaWeaponAsset, &gSkinAsset, (void*)SkinCtorHook<4>);
    SkinHookOne(5, kSkinRvaMythicArmor, &gSkinArmor, (void*)SkinCtorHook<5>);
    SkinHookOne(6, kSkinRvaMythicSight, &gSkinSight, (void*)SkinCtorHook<6>);
    SkinHookOne(7, kSkinRvaKillEffect, &gSkinKill, (void*)SkinCtorHook<7>);
    SkinHookOne(8, kSkinRvaItemResource, &gSkinRes, (void*)SkinCtorHook<8>);
    SkinHookOne(9, kSkinRvaRoleConf, &gSkinRole, (void*)SkinCtorHook<9>);
    SkinHookOne(10, kSkinRvaCharModel, &gSkinModel, (void*)SkinCtorHook<10>);
}

static const uintptr_t kCharModelOff[12] = {
    0x18, 0x1C, 0x20, 0x24, 0x28, 0x2C, 0x30, 0x34, 0x38, 0x3C, 0x40, 0x44
};

static int gCharHave = 0;

static void SkinPaintRole(uint32_t dstRole, uint64_t skinAvatar, const SkinRow& row,
                          const std::vector<void*>& roles,
                          const std::vector<void*>& models,
                          const std::vector<void*>& res) {
    if (!dstRole) return;
    uint64_t dstAvatar = 0;
    for (void* r : res) {
        if ((uint32_t)SkinRdI(r, 0x1C) != dstRole) continue;
        dstAvatar = SkinRdU64(r, 0x28);
        if (row.fx) SkinWrI(r, 0x18, row.fx);
        if (row.invModel) SkinWrI(r, 0x20, row.invModel);
        if (row.modelRaw) SkinWrI(r, 0x24, row.modelRaw);
        if (skinAvatar) SkinWrU64(r, 0x28, skinAvatar);
        break;
    }
    for (void* role : roles) {
        if ((uint32_t)SkinRdI(role, 0x34) != dstRole) continue;
        if (row.arm) SkinWrI(role, 0x14, row.arm);
        if (row.suit) SkinWrI(role, 0x18, row.suit);
        if (row.holo) SkinWrI(role, 0x1C, row.holo);
        if (row.color) SkinWrB(role, 0x22, (uint8_t)row.color);
        if (row.pack) SkinWrI(role, 0x44, row.pack);
        break;
    }
    if (!dstAvatar) return;
    void* dstMesh = nullptr;
    void* srcMesh = nullptr;
    for (void* m : models) {
        uint64_t id = SkinRdU64(m, 0x78);
        if (!dstMesh && id == dstAvatar) dstMesh = m;
        if (!srcMesh && skinAvatar && id == skinAvatar) srcMesh = m;
    }
    if (srcMesh && dstMesh && srcMesh != dstMesh) {
        for (int i = 0; i < 12; ++i) {
            int v = SkinRdI(srcMesh, kCharModelOff[i]);
            if (v) SkinWrI(dstMesh, kCharModelOff[i], v);
        }
    } else if (dstMesh && row.hasModel) {
        for (int i = 0; i < 12; ++i) {
            if (row.modelF[i]) SkinWrI(dstMesh, kCharModelOff[i], row.modelF[i]);
        }
    }
}

static void SkinApplyChar(uint32_t dstRole, const SkinRow& row) {
    uint32_t skinRole = (uint32_t)row.skinId;
    if (!dstRole || !skinRole || dstRole == skinRole) return;

    pthread_mutex_lock(&gSkinMu);
    std::vector<void*> roles = gSkinRole;
    std::vector<void*> models = gSkinModel;
    std::vector<void*> res = gSkinRes;
    pthread_mutex_unlock(&gSkinMu);

    uint64_t skinAvatar = row.avatar;
    if (!skinAvatar) {
        for (void* r : res) {
            if ((uint32_t)SkinRdI(r, 0x1C) != skinRole) continue;
            skinAvatar = SkinRdU64(r, 0x28);
            break;
        }
    }
    SkinPaintRole(dstRole, skinAvatar, row, roles, models, res);
}

static void SkinApplyRow(const SkinRow& row) {
    if (row.kind == 1) return;
    if (row.baseId == 0 && row.skinId == 0) return;
    pthread_mutex_lock(&gSkinMu);
    std::vector<void*> inv = gSkinInv;
    std::vector<void*> conf = gSkinConf;
    std::vector<void*> extra = gSkinExtra;
    std::vector<void*> asset = gSkinAsset;
    std::vector<void*> res = gSkinRes;
    pthread_mutex_unlock(&gSkinMu);

    const int alt = row.baseId ? row.baseId + 200 : 0;
    for (void* item : inv) {
        int id = SkinRdI(item, 0x20);
        if (id == 0) continue;
        if (id == row.baseId || id == alt) {
            if (row.assetGroup) SkinWrI(item, 0x24, row.assetGroup);
            if (row.icon) SkinWrI(item, 0x28, row.icon);
        }
    }
    for (void* c : conf) {
        int id = SkinRdI(c, 0x40);
        if (id == 0) continue;
        if (id == row.baseId || (alt && id == alt)) {
            SkinWrB(c, 0x22, (uint8_t)row.color);
            if (row.skinId) SkinWrI(c, 0x38, row.skinId);
            if (row.broadcast) SkinWrI(c, 0x3C, row.broadcast);
        }
    }
    for (void* e : extra) {
        int id = SkinRdI(e, 0x24);
        if (id == 0 || id != row.baseId) continue;
        if (row.name.find("[M]") != std::string::npos) {
            if (row.mythicArmor > 0 && row.mythicArmor < 256)
                SkinWrB(e, 0x10, (uint8_t)row.mythicArmor);
            if (row.mythicSight > 0 && row.mythicSight < 256)
                SkinWrB(e, 0x11, (uint8_t)row.mythicSight);
        }
        if (row.deadReplay) SkinWrI(e, 0x18, row.deadReplay);
        if (row.killEffect) SkinWrI(e, 0x1C, row.killEffect);
    }
    if (row.assetId && row.fireId > 0 && row.fireOrig > 0 &&
        (row.fireId / 10) == (row.fireOrig / 10)) {
        for (void* a : asset) {
            if (SkinRdI(a, 0x44) == row.assetId)
                SkinWrI(a, 0x40, row.fireId);
        }
    }
    if (row.baseId) {
        for (void* r : res) {
            if (SkinRdI(r, 0x1C) != row.baseId) continue;
            if (row.fx) SkinWrI(r, 0x18, row.fx);
            if (row.invModel) SkinWrI(r, 0x20, row.invModel);
            if (row.modelRaw) SkinWrI(r, 0x24, row.modelRaw);
        }
    }
}

static void SkinScanMain() {
    int last = -1;
    int stable = 0;
    for (int i = 0; i < 150; ++i) {
        pthread_mutex_lock(&gSkinMu);
        int n = (int)gSkinConf.size();
        int inv = (int)gSkinInv.size();
        int roles = (int)gSkinRole.size();
        pthread_mutex_unlock(&gSkinMu);
        if (n > 100 && n == last && inv > 0) {
            if (++stable >= 5 && (roles > 0 || stable >= 40)) break;
        } else {
            stable = 0;
        }
        last = n;
        usleep(200000);
    }

    pthread_mutex_lock(&gSkinMu);
    std::vector<void*> conf = gSkinConf;
    std::vector<void*> inv = gSkinInv;
    std::vector<void*> asset = gSkinAsset;
    std::vector<void*> fire = gSkinFire;
    std::vector<void*> extra = gSkinExtra;
    std::vector<void*> armor = gSkinArmor;
    std::vector<void*> sight = gSkinSight;
    std::vector<void*> kill = gSkinKill;
    std::vector<void*> res = gSkinRes;
    std::vector<void*> roles = gSkinRole;
    std::vector<void*> models = gSkinModel;
    pthread_mutex_unlock(&gSkinMu);

    if (conf.empty() || inv.empty()) {
        gSkinState.store(3, std::memory_order_release);
        return;
    }

    std::unordered_map<int, void*> invById;
    invById.reserve(inv.size());
    for (void* p : inv) {
        int id = SkinRdI(p, 0x20);
        if (id) invById.emplace(id, p);
    }
    std::unordered_map<int, void*> assetById;
    assetById.reserve(asset.size());
    for (void* p : asset) {
        int id = SkinRdI(p, 0x44);
        if (id) assetById.emplace(id, p);
    }
    std::unordered_map<int, void*> fireById;
    fireById.reserve(fire.size());
    for (void* p : fire) {
        int id = SkinRdI(p, 0x80);
        if (id) fireById.emplace(id, p);
    }
    std::unordered_map<int, void*> extraById;
    extraById.reserve(extra.size());
    for (void* p : extra) {
        int id = SkinRdI(p, 0x24);
        if (id) extraById.emplace(id, p);
    }
    std::unordered_map<int, void*> resById;
    resById.reserve(res.size());
    for (void* p : res) {
        int id = SkinRdI(p, 0x1C);
        if (id) resById.emplace(id, p);
    }

    std::vector<SkinRow> rows;
    rows.reserve(conf.size());

    for (void* c : conf) {
        int baseId = SkinRdI(c, 0x34);
        int confId = SkinRdI(c, 0x40);
        if (confId == 0) continue;
        auto iit = invById.find(confId);
        if (iit == invById.end()) continue;
        void* item = iit->second;
        int assetGroup = SkinRdI(item, 0x24);
        int icon = SkinRdI(item, 0x28);
        int color = SkinRdB(c, 0x22);

        char stub[32];
        snprintf(stub, sizeof(stub), "%sid %d", SkinTag(color), confId);

        SkinRow row;
        row.name = stub;
        row.low = SkinLower(row.name);
        row.conf = c;
        row.named = false;
        row.baseId = baseId;
        row.color = color;
        row.skinId = confId;
        row.broadcast = SkinRdI(c, 0x3C);
        row.assetGroup = assetGroup;
        row.icon = icon;

        bool mythic = color == 5;
        if (mythic) {
            auto ait = assetById.find(assetGroup);
            if (ait != assetById.end()) {
                int fireEffect = SkinRdI(ait->second, 0x40);
                auto fit = fireById.find(fireEffect);
                if (fit != fireById.end()) {
                    int smoke = SkinRdI(fit->second, 0x1C);
                    int chosen = fireEffect;
                    if (smoke == 0) {
                        for (int step = 1; step <= 10; ++step) {
                            auto nx = fireById.find(fireEffect + step);
                            if (nx != fireById.end() && SkinRdI(nx->second, 0x1C) != 0) {
                                chosen = fireEffect + step;
                                break;
                            }
                        }
                    }
                    row.fireId = chosen;
                    row.fireOrig = fireEffect;
                    row.assetId = SkinRdI(ait->second, 0x44);
                }
            }
            for (void* a : armor) {
                if (SkinRdI(a, 0x2C) == icon && SkinRdI(a, 0x30) == 5)
                    row.mythicArmor = SkinRdI(a, 0x14);
            }
            for (void* s : sight) {
                if (icon && icon == SkinLastInt(s, 0x38))
                    row.mythicSight = SkinRdI(s, 0x14);
            }
            for (void* k : kill) {
                if (icon && icon == SkinLastInt(k, 0x90))
                    row.killEffect = SkinLastInt(k, 0x10);
            }
        }
        auto eit = extraById.find(confId);
        if (eit != extraById.end()) {
            row.deadReplay = SkinRdI(eit->second, 0x18);
            if (!row.killEffect) row.killEffect = SkinRdI(eit->second, 0x1C);
        }
        auto rit = resById.find(confId);
        if (rit != resById.end()) {
            row.fx = SkinRdI(rit->second, 0x18);
            row.invModel = SkinRdI(rit->second, 0x20);
            row.modelRaw = SkinRdI(rit->second, 0x24);
        }
        rows.push_back(std::move(row));
    }

    std::unordered_map<uint64_t, int> modelAt;
    modelAt.reserve(models.size());
    struct CharMesh { uint64_t itemId; int f[12]; };
    std::vector<CharMesh> meshes;
    meshes.reserve(models.size());
    for (void* m : models) {
        uint64_t itemId = SkinRdU64(m, 0x78);
        if (!itemId) continue;
        CharMesh mesh;
        mesh.itemId = itemId;
        for (int i = 0; i < 12; ++i) mesh.f[i] = SkinRdI(m, kCharModelOff[i]);
        modelAt.emplace(itemId, (int)meshes.size());
        meshes.push_back(mesh);
    }
    for (void* role : roles) {
        int id = SkinRdI(role, 0x34);
        int baseId = SkinRdI(role, 0x10);
        if (id == 0) continue;
        int color = SkinRdB(role, 0x22);
        char stub[40];
        snprintf(stub, sizeof(stub), "%sid %d", SkinTag(color), id);
        SkinRow row;
        row.kind = 1;
        row.name = stub;
        row.low = SkinLower(row.name);
        row.conf = role;
        row.baseId = baseId;
        row.skinId = id;
        row.color = color;
        row.arm = SkinRdI(role, 0x14);
        row.suit = SkinRdI(role, 0x18);
        row.holo = SkinRdI(role, 0x1C);
        row.pack = SkinRdI(role, 0x44);
        auto rit = resById.find(id);
        if (rit != resById.end()) {
            row.fx = SkinRdI(rit->second, 0x18);
            row.invModel = SkinRdI(rit->second, 0x20);
            row.modelRaw = SkinRdI(rit->second, 0x24);
            uint64_t avatar = SkinRdU64(rit->second, 0x28);
            row.avatar = avatar;
            auto mit = modelAt.find(avatar);
            if (mit != modelAt.end()) {
                row.hasModel = 1;
                for (int i = 0; i < 12; ++i) row.modelF[i] = meshes[mit->second].f[i];
            }
        }
        rows.push_back(std::move(row));
    }

    gSkinNameCursor = 0;
    gSkinCharCursor = 0;
    gSkinRows = std::move(rows);
    gSkinState.store(2, std::memory_order_release);
}

static void SkinNameOne(SkinRow& row) {
    if (!row.conf || row.named) {
        row.named = true;
        return;
    }
    uintptr_t rva = row.kind == 1 ? kSkinRvaRoleName : kSkinRvaWeaponName;
    std::string raw = SkinTake(SkinConfName(row.conf, rva).c_str(), 40);
    if (!raw.empty()) {
        row.name = std::string(SkinTag(row.color)) + raw;
        row.low = SkinLower(row.name);
    }
    row.named = true;
    row.conf = nullptr;
}

static void SkinFillNames() {
    if (gSkinState.load(std::memory_order_acquire) != 2) return;
    int n = (int)gSkinRows.size();
    int budget = 16;
    while (budget > 0 && gSkinCharCursor < n) {
        while (gSkinCharCursor < n && gSkinRows[gSkinCharCursor].kind != 1)
            ++gSkinCharCursor;
        if (gSkinCharCursor >= n) break;
        SkinNameOne(gSkinRows[gSkinCharCursor++]);
        --budget;
    }
    while (budget > 0 && gSkinNameCursor < n) {
        SkinRow& row = gSkinRows[gSkinNameCursor++];
        if (row.kind == 1 || row.named) continue;
        SkinNameOne(row);
        --budget;
    }
}

static void SkinTick() {
    if (!checkboxStates["SKIN::LOAD"]) {
        if (gSkinState.load(std::memory_order_acquire) == 3)
            gSkinState.store(0, std::memory_order_release);
        return;
    }
    SkinInstallHooks();
    int st = gSkinState.load(std::memory_order_acquire);
    if (st == 0) {
        gSkinState.store(1, std::memory_order_release);
        std::thread(SkinScanMain).detach();
    }
}

static void SkinKeyAppend(const char* s) {
    if (!s || !s[0]) return;
    size_t n = strlen(gSkinSearch);
    size_t add = strlen(s);
    if (n + add >= sizeof(gSkinSearch)) return;
    memcpy(gSkinSearch + n, s, add + 1);
}

static void SkinKeyBack() {
    size_t n = strlen(gSkinSearch);
    if (n >= 3 && gSkinSearch[n - 1] == ']' && gSkinSearch[n - 3] == '[') {
        gSkinSearch[n - 3] = 0;
        return;
    }
    if (n) gSkinSearch[n - 1] = 0;
}

static bool SkinKeyBtn(const char* vis, int uid, float w, float h) {
    if (w < 8.f) w = 8.f;
    if (h < 8.f) h = 8.f;
    ImGui::PushID(uid);
    bool hit = ImGui::Button(vis, ImVec2(w, h));
    ImGui::PopID();
    return hit;
}

static void DrawSkinKeyboard() {
    static bool upper = false;
    const float gap = 4.f;
    const float keyH = 34.f;
    float width = ImGui::GetContentRegionAvail().x;
    if (width < 120.f) width = 120.f;
    float keyW = (width - gap * 9.f) / 10.f;
    if (keyW < 18.f) keyW = 18.f;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(gap, gap));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.f);

    const char* rows[4] = {
        "1234567890",
        upper ? "QWERTYUIOP" : "qwertyuiop",
        upper ? "ASDFGHJKL" : "asdfghjkl",
        upper ? "ZXCVBNM" : "zxcvbnm"
    };
    int uid = 1;
    for (int r = 0; r < 4; ++r) {
        int count = (int)strlen(rows[r]);
        float rowW = count * keyW + (count - 1) * gap;
        float indent = (width - rowW) * 0.5f;
        if (indent > 1.f && indent < width)
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + indent);
        for (int i = 0; i < count; ++i) {
            char lab[2] = { rows[r][i], 0 };
            if (SkinKeyBtn(lab, uid++, keyW, keyH))
                SkinKeyAppend(lab);
            if (i + 1 < count) ImGui::SameLine(0.f, gap);
        }
    }

    const char* tags[] = { "[M]", "[L]", "[E]", "[C]" };
    float tagW = (width - gap * 3.f) / 4.f;
    if (tagW < 8.f) tagW = 8.f;
    for (int i = 0; i < 4; ++i) {
        if (SkinKeyBtn(tags[i], uid++, tagW, keyH))
            SkinKeyAppend(tags[i]);
        if (i + 1 < 4) ImGui::SameLine(0.f, gap);
    }

    float actW = (width - gap * 2.f) / 3.f;
    if (actW < 8.f) actW = 8.f;
    if (SkinKeyBtn("Space", uid++, actW, keyH)) SkinKeyAppend(" ");
    ImGui::SameLine(0.f, gap);
    if (SkinKeyBtn("<<", uid++, actW, keyH)) SkinKeyBack();
    ImGui::SameLine(0.f, gap);
    if (SkinKeyBtn(upper ? "Aa" : "aa", uid++, actW, keyH)) upper = !upper;
    if (SkinKeyBtn("Clear", uid++, width, keyH)) gSkinSearch[0] = 0;

    ImGui::PopStyleVar(2);
}

static void SkinLabel(const SkinRow& row, char* out, int n) {
    snprintf(out, n, "%d  %s", row.skinId, row.name.c_str());
}

static bool SkinMatch(const SkinRow& row, const std::string& q) {
    if (q.empty()) return true;
    if (row.low.find(q) != std::string::npos) return true;
    char id[16];
    snprintf(id, sizeof(id), "%d", row.skinId);
    return strstr(id, q.c_str()) != nullptr;
}

static void DrawCharTab() {
    ImGui::Text(ICON_FA_USER "  Character");
    ImGui::Separator();
    ImGui::BeginGroupPanel("Character", ImVec2(-1, 0));
    g_widgets.checkbox("Load character list", &checkboxStates["SKIN::LOAD"]);
    int st = gSkinState.load(std::memory_order_acquire);
    static int step = 0;
    if (ImGui::Button("1 Equipped")) step = 0;
    ImGui::SameLine();
    if (ImGui::Button("2 Skin")) step = 1;
    ImGui::SameLine();
    ImGui::TextUnformatted(step == 0 ? "picking equipped" : "picking skin");
    if (gCharHave) {
        char have[48];
        snprintf(have, sizeof(have), "Equipped id %d", gCharHave);
        ImGui::TextUnformatted(have);
    } else {
        ImGui::TextUnformatted("Pick the character you have on.");
    }
    if (st == 1) ImGui::TextUnformatted("Reading character tables...");
    else if (st == 3) ImGui::TextUnformatted("No tables yet. Toggle again in the lobby.");
    ImGui::EndGroupPanel();

    if (st != 2 || gSkinRows.empty()) return;
    SkinFillNames();
    ImGui::Spacing();
    ImGui::BeginGroupPanel(step == 0 ? "Equipped id" : "Skin to replace", ImVec2(-1, 0));
    if (gSkinSearch[0]) ImGui::TextUnformatted(gSkinSearch);
    else ImGui::TextUnformatted("...");
    ImGui::EndGroupPanel();

    const float kbH = 34.f * 7.f + 40.f;
    float listH = ImGui::GetContentRegionAvail().y - kbH;
    if (listH < 90.f) listH = 90.f;
    ImGui::BeginChild("##charlist", ImVec2(0, listH), true);
    if (gSkinPicked.size() != gSkinRows.size())
        gSkinPicked.assign(gSkinRows.size(), 0);
    std::string q = SkinLower(gSkinSearch);
    int shown = 0;
    const int cap = q.empty() ? 30 : 40;
    for (size_t i = 0; i < gSkinRows.size(); ++i) {
        const SkinRow& row = gSkinRows[i];
        if (row.kind != 1) continue;
        if (!SkinMatch(row, q)) continue;
        if (shown >= cap) {
            ImGui::TextUnformatted(q.empty() ? "Search to see more." : "Keep typing to narrow.");
            break;
        }
        ++shown;
        char lab[96];
        SkinLabel(row, lab, sizeof(lab));
        ImGui::PushID((int)i + 1);
        bool on = step == 0 ? (gCharHave == row.skinId) : (gSkinPicked[i] != 0);
        if (g_widgets.checkbox(lab, &on)) {
            if (step == 0) {
                gCharHave = on ? row.skinId : 0;
            } else if (gCharHave) {
                gSkinPicked[i] = on ? 1 : 0;
                if (on) SkinApplyChar((uint32_t)gCharHave, row);
            }
        }
        ImGui::PopID();
    }
    if (shown == 0) ImGui::TextUnformatted("No match.");
    ImGui::EndChild();
    ImGui::Spacing();
    DrawSkinKeyboard();
}

static void DrawSkinTab() {
    ImGui::Text(ICON_FA_TSHIRT "  Skin");
    ImGui::Separator();
    ImGui::BeginGroupPanel("Weapon skins", ImVec2(-1, 0));
    g_widgets.checkbox("Load skin list", &checkboxStates["SKIN::LOAD"]);
    int st = gSkinState.load(std::memory_order_acquire);
    if (st == 1) ImGui::TextUnformatted("Reading skin tables...");
    else if (st == 3) ImGui::TextUnformatted("No tables yet. Toggle again in the lobby.");
    else if (st == 2) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%d skins", (int)gSkinRows.size());
        ImGui::TextUnformatted(buf);
    } else {
        ImGui::TextUnformatted("Turn on in the lobby, then pick a skin.");
    }
    ImGui::EndGroupPanel();

    if (st != 2 || gSkinRows.empty()) return;
    SkinFillNames();

    ImGui::Spacing();
    ImGui::BeginGroupPanel("Search", ImVec2(-1, 0));
    if (gSkinSearch[0]) ImGui::TextUnformatted(gSkinSearch);
    else ImGui::TextUnformatted("...");
    ImGui::EndGroupPanel();

    const float kbH = 34.f * 7.f + 40.f;
    float listH = ImGui::GetContentRegionAvail().y - kbH;
    if (listH < 90.f) listH = 90.f;
    ImGui::BeginChild("##skinlist", ImVec2(0, listH), true);
    if (gSkinPicked.size() != gSkinRows.size())
        gSkinPicked.assign(gSkinRows.size(), 0);
    std::string q = SkinLower(gSkinSearch);
    int shown = 0;
    const int cap = q.empty() ? 30 : 40;
    for (size_t i = 0; i < gSkinRows.size(); ++i) {
        const SkinRow& row = gSkinRows[i];
        if (row.kind == 1) continue;
        if (!SkinMatch(row, q)) continue;
        if (shown >= cap) {
            ImGui::TextUnformatted(q.empty() ? "Search to see more." : "Keep typing to narrow.");
            break;
        }
        ++shown;
        char lab[96];
        SkinLabel(row, lab, sizeof(lab));
        ImGui::PushID((int)i + 1);
        bool on = gSkinPicked[i] != 0;
        if (g_widgets.checkbox(lab, &on)) {
            gSkinPicked[i] = on ? 1 : 0;
            if (on) SkinApplyRow(row);
        }
        ImGui::PopID();
    }
    if (shown == 0) ImGui::TextUnformatted("No match.");
    ImGui::EndChild();
    ImGui::Spacing();
    DrawSkinKeyboard();
}
