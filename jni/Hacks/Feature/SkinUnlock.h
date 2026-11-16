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

struct SkinRow {
    std::string name;
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
};

static std::vector<void*> gSkinInv, gSkinExtra, gSkinFire, gSkinConf, gSkinAsset;
static std::vector<void*> gSkinArmor, gSkinSight, gSkinKill, gSkinRes;
static pthread_mutex_t gSkinMu = PTHREAD_MUTEX_INITIALIZER;
static std::vector<SkinRow> gSkinRows;
static std::atomic<int> gSkinState{0};
static char gSkinSearch[64] = "";

struct SkinHookSlot {
    void (*orig)(void*);
    std::vector<void*>* vec;
};
static SkinHookSlot gSkinSlots[9];

template <int N>
static void SkinCtorHook(void* self) {
    if (self && gSkinSlots[N].vec && gSkinSlots[N].vec->size() < 50000) {
        pthread_mutex_lock(&gSkinMu);
        gSkinSlots[N].vec->push_back(self);
        pthread_mutex_unlock(&gSkinMu);
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

static int SkinLastInt(void* obj, uintptr_t off) {
    if (!SkinRdOk(obj, off)) return 0;
    auto arr = *(Array<int>**)((uintptr_t)obj + off);
    if (!Tools::IsPtrValid(arr)) return 0;
    int n = arr->getLength();
    if (n <= 0 || n > 64) return 0;
    return arr->m_Items[n - 1];
}

static std::string SkinConfName(void* obj) {
    if (!libunity_base || !Tools::IsPtrValid(obj)) return {};
    auto fn = (String* (*)(void*))(libunity_base + kSkinRvaWeaponName);
    String* s = fn(obj);
    if (!Tools::IsPtrValid(s)) return {};
    if (s->length <= 0 || s->length > 48) return {};
    const char* u = s->CString();
    if (!u || !u[0]) return {};
    return std::string(u);
}

static std::string SkinLower(const std::string& in) {
    std::string o = in;
    for (char& c : o) c = (char)std::tolower((unsigned char)c);
    return o;
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
}

static void SkinApplyRow(const SkinRow& row) {
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
        pthread_mutex_unlock(&gSkinMu);
        if (n > 100 && n == last && inv > 0) {
            if (++stable >= 5) break;
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
    std::unordered_map<std::string, int> seen;
    seen.reserve(conf.size());

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

        std::string raw = SkinConfName(c);
        if (raw.empty()) raw = "id " + std::to_string(confId);
        const char* tag = "[C] ";
        if (color == 5) tag = "[M] ";
        else if (color == 4) tag = "[L] ";
        else if (color == 3) tag = "[E] ";
        std::string name = std::string(tag) + raw;
        int n = seen[name]++;
        if (n > 0) name += " +" + std::to_string(n);

        SkinRow row;
        row.name = name;
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

    gSkinRows = std::move(rows);
    gSkinState.store(2, std::memory_order_release);
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

    ImGui::Spacing();
    ImGui::InputText("Search##skin", gSkinSearch, sizeof(gSkinSearch));
    ImGui::BeginChild("##skinlist", ImVec2(0, 0), true);
    std::string q = SkinLower(gSkinSearch);
    int shown = 0;
    const int cap = q.empty() ? 40 : 80;
    for (size_t i = 0; i < gSkinRows.size(); ++i) {
        if (!q.empty() && SkinLower(gSkinRows[i].name).find(q) == std::string::npos)
            continue;
        if (shown >= cap) {
            ImGui::TextUnformatted(q.empty() ? "Search to see more." : "Keep typing to narrow.");
            break;
        }
        ++shown;
        std::string lab = gSkinRows[i].name + "##sk" + std::to_string(i);
        bool on = false;
        if (g_widgets.checkbox(lab.c_str(), &on) && on)
            SkinApplyRow(gSkinRows[i]);
    }
    if (shown == 0) ImGui::TextUnformatted("No match.");
    ImGui::EndChild();
}
