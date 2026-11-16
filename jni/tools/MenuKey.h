#pragma once

#include <atomic>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <mutex>
#include <string>
#include <vector>
#include "ImGui/imgui.h"

static char gMenuKeyInput[80] = {};
static char gMenuKeyStatus[160] = "Enter the key from the loader";
static std::mutex gMenuKeyMu;
static std::atomic<bool> gMenuUnlocked{false};
static bool gMenuKeyArmed = false;

static void MenuKeySetStatus(const std::string& text) {
    std::lock_guard<std::mutex> lock(gMenuKeyMu);
    snprintf(gMenuKeyStatus, sizeof(gMenuKeyStatus), "%s", text.c_str());
}

static std::string MenuKeyGetStatus() {
    std::lock_guard<std::mutex> lock(gMenuKeyMu);
    return gMenuKeyStatus;
}

static bool MenuKeyUnlocked() {
    return gMenuUnlocked.load();
}

static bool MenuKeySame(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i])) return false;
    }
    return !a.empty();
}

static bool MenuKeyDate(const std::string& text, int& year, int& month, int& day) {
    if (sscanf(text.c_str(), "%d-%d-%d", &year, &month, &day) != 3 &&
        sscanf(text.c_str(), "%d/%d/%d", &year, &month, &day) != 3) {
        return false;
    }
    return year > 2000 && month >= 1 && month <= 12 && day >= 1 && day <= 31;
}

static bool MenuKeyPast(int year, int month, int day) {
    time_t now = time(nullptr);
    tm local = *localtime(&now);
    int cy = local.tm_year + 1900;
    int cm = local.tm_mon + 1;
    int cd = local.tm_mday;
    if (year != cy) return year < cy;
    if (month != cm) return month < cm;
    return day < cd;
}

struct MenuKeyPass {
    std::string key;
    std::string name;
    std::string until;
    bool ok = false;
};

static MenuKeyPass MenuKeyReadPass() {
    const char* paths[] = {
        "/sdcard/Android/data/com.garena.game.codm/files/codm.menu.key",
        "/storage/emulated/0/Android/data/com.garena.game.codm/files/codm.menu.key",
        "/sdcard/codm.menu.key",
        "/storage/emulated/0/codm.menu.key",
        "/data/local/tmp/codm.menu.key"
    };
    MenuKeyPass pass;
    for (const char* path : paths) {
        FILE* file = fopen(path, "r");
        if (!file) continue;
        char line[180] = {};
        if (!fgets(line, sizeof(line), file)) {
            fclose(file);
            continue;
        }
        fclose(file);
        std::string text = line;
        while (!text.empty() && (text.back() == '\n' || text.back() == '\r' || text.back() == ' ')) text.pop_back();
        size_t first = text.find('|');
        size_t second = first == std::string::npos ? std::string::npos : text.find('|', first + 1);
        if (first == std::string::npos || second == std::string::npos) continue;
        pass.key = text.substr(0, first);
        pass.name = text.substr(first + 1, second - first - 1);
        pass.until = text.substr(second + 1);
        int year = 0, month = 0, day = 0;
        if (!pass.key.empty() && MenuKeyDate(pass.until, year, month, day)) {
            char date[32];
            snprintf(date, sizeof(date), "%04d-%02d-%02d", year, month, day);
            pass.until = date;
            pass.ok = true;
            return pass;
        }
    }
    return pass;
}

static std::string MenuKeyToken(const std::string& typed) {
    std::string word;
    std::string token;
    for (size_t i = 0; i <= typed.size(); ++i) {
        char c = i < typed.size() ? typed[i] : ' ';
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            if (word.find('-') != std::string::npos && word.size() >= 6) token = word;
            word.clear();
        } else {
            word.push_back(c);
        }
    }
    if (!token.empty()) return token;
    size_t start = 0;
    while (start < typed.size() && typed[start] == ' ') start++;
    size_t end = typed.size();
    while (end > start && typed[end - 1] == ' ') end--;
    return typed.substr(start, end - start);
}

static void MenuKeyApply(const std::string& typed) {
    MenuKeyPass pass = MenuKeyReadPass();
    std::string token = MenuKeyToken(typed);
    if (!pass.ok || token.empty() || !MenuKeySame(token, pass.key)) {
        gMenuUnlocked.store(false);
        MenuKeySetStatus(pass.ok ? "Invalid key" : "Enter the key from the loader");
        return;
    }
    int year = 0, month = 0, day = 0;
    MenuKeyDate(pass.until, year, month, day);
    char line[160];
    if (MenuKeyPast(year, month, day)) {
        gMenuUnlocked.store(false);
        snprintf(line, sizeof(line), "%s expired %s", pass.name.c_str(), pass.until.c_str());
    } else {
        gMenuUnlocked.store(true);
        snprintf(line, sizeof(line), "%s valid until %s", pass.name.c_str(), pass.until.c_str());
    }
    MenuKeySetStatus(line);
}

static void MenuKeyEnsure() {
    if (gMenuKeyArmed) return;
    gMenuKeyArmed = true;
    MenuKeyPass pass = MenuKeyReadPass();
    if (pass.ok) snprintf(gMenuKeyInput, sizeof(gMenuKeyInput), "%s", pass.key.c_str());
    MenuKeyApply(gMenuKeyInput);
}

static void MenuKeyStatusLine() {
    MenuKeyEnsure();
    ImGui::TextUnformatted(MenuKeyGetStatus().c_str());
}

static void DrawMenuKeyBox() {
    MenuKeyEnsure();
    ImGui::BeginGroupPanel("Menu key", ImVec2(-1, 0));
    ImGui::InputText("##menukey", gMenuKeyInput, sizeof(gMenuKeyInput));
    if (ImGui::Button("Check")) MenuKeyApply(gMenuKeyInput);
    ImGui::TextWrapped("%s", MenuKeyGetStatus().c_str());
    ImGui::EndGroupPanel();
}
