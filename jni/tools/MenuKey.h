#pragma once

#include <atomic>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <dlfcn.h>
#include <jni.h>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include "ImGui/imgui.h"

static char gMenuKeyInput[80] = {};
static char gMenuKeyStatus[160] = "Enter key";
static std::mutex gMenuKeyMu;
static std::atomic<bool> gMenuKeyBusy{false};
static bool gMenuKeyArmed = false;

static std::string MenuKeyAddress() {
    static const unsigned char packed[] = {
        207, 211, 211, 215, 212, 157, 136, 136, 213, 198, 208, 137, 192, 206, 211, 207, 210, 197, 210, 212, 194, 213, 196, 200, 201, 211, 194, 201, 211, 137, 196, 200, 202, 136, 230, 223, 196, 194, 203, 215, 200, 192, 206, 150, 158, 151, 136, 247, 198, 212, 212, 208, 200, 213, 195, 230, 201, 195, 235, 206, 196, 194, 201, 212, 194, 136, 202, 198, 206, 201, 136, 228, 200, 195, 202, 247, 198, 212, 212
    };
    std::string out(sizeof(packed), '\0');
    for (size_t i = 0; i < sizeof(packed); ++i) out[i] = (char)(packed[i] ^ 0xA7);
    return out;
}

static void MenuKeySetStatus(const std::string& text) {
    std::lock_guard<std::mutex> lock(gMenuKeyMu);
    snprintf(gMenuKeyStatus, sizeof(gMenuKeyStatus), "%s", text.c_str());
}

static std::string MenuKeyGetStatus() {
    std::lock_guard<std::mutex> lock(gMenuKeyMu);
    return gMenuKeyStatus;
}

static bool MenuKeySame(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i])) return false;
    }
    return !a.empty();
}

static bool MenuKeyDate(const std::string& text, int& year, int& month, int& day) {
    char sep = 0;
    if (sscanf(text.c_str(), "%d-%d-%d%c", &year, &month, &day, &sep) < 3 &&
        sscanf(text.c_str(), "%d/%d/%d%c", &year, &month, &day, &sep) < 3) {
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

static void MenuKeyStore(const std::string& key) {
    const char* paths[] = {
        "/sdcard/codm.menu.key",
        "/storage/emulated/0/codm.menu.key",
        "/data/local/tmp/codm.menu.key"
    };
    for (const char* path : paths) {
        FILE* file = fopen(path, "w");
        if (!file) continue;
        fputs(key.c_str(), file);
        fputc('\n', file);
        fclose(file);
    }
}

static void MenuKeyLoad(char* dest, size_t cap) {
    const char* paths[] = {
        "/sdcard/codm.menu.key",
        "/storage/emulated/0/codm.menu.key",
        "/data/local/tmp/codm.menu.key"
    };
    for (const char* path : paths) {
        FILE* file = fopen(path, "r");
        if (!file) continue;
        if (!fgets(dest, (int)cap, file)) {
            fclose(file);
            continue;
        }
        fclose(file);
        size_t n = strlen(dest);
        while (n > 0 && (dest[n - 1] == '\n' || dest[n - 1] == '\r' || dest[n - 1] == ' ')) dest[--n] = 0;
        if (n > 0) return;
    }
}

static std::string MenuKeyFetch(const std::string& address) {
    void* art = dlopen("libart.so", RTLD_NOW);
    if (!art) return {};
    auto getVms = (jint (*)(JavaVM**, jsize, jsize*))dlsym(art, "JNI_GetCreatedJavaVMs");
    JavaVM* vm = nullptr;
    jsize count = 0;
    if (!getVms || getVms(&vm, 1, &count) != JNI_OK || !vm || count < 1) return {};
    JNIEnv* env = nullptr;
    bool attached = false;
    jint state = vm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (state == JNI_EDETACHED) {
        if (vm->AttachCurrentThread(&env, nullptr) != 0) return {};
        attached = true;
    } else if (state != JNI_OK || !env) {
        return {};
    }

    std::string body;
    auto fail = [&]() {
        if (env->ExceptionCheck()) env->ExceptionClear();
        if (attached) vm->DetachCurrentThread();
    };
    jclass urlClass = env->FindClass("java/net/URL");
    if (!urlClass) { fail(); return {}; }
    jmethodID urlInit = env->GetMethodID(urlClass, "<init>", "(Ljava/lang/String;)V");
    jstring jAddress = env->NewStringUTF(address.c_str());
    jobject url = env->NewObject(urlClass, urlInit, jAddress);
    if (env->ExceptionCheck() || !url) { fail(); return {}; }
    jmethodID open = env->GetMethodID(urlClass, "openConnection", "()Ljava/net/URLConnection;");
    jobject conn = env->CallObjectMethod(url, open);
    if (env->ExceptionCheck() || !conn) { fail(); return {}; }
    jclass http = env->FindClass("java/net/HttpURLConnection");
    if (!http) { fail(); return {}; }
    env->CallVoidMethod(conn, env->GetMethodID(http, "setConnectTimeout", "(I)V"), 15000);
    env->CallVoidMethod(conn, env->GetMethodID(http, "setReadTimeout", "(I)V"), 20000);
    jstring agentKey = env->NewStringUTF("User-Agent");
    jstring agentValue = env->NewStringUTF("Accel");
    env->CallVoidMethod(conn, env->GetMethodID(http, "setRequestProperty", "(Ljava/lang/String;Ljava/lang/String;)V"), agentKey, agentValue);
    jobject stream = env->CallObjectMethod(conn, env->GetMethodID(http, "getInputStream", "()Ljava/io/InputStream;"));
    if (env->ExceptionCheck() || !stream) { fail(); return {}; }
    jclass input = env->FindClass("java/io/InputStream");
    jmethodID read = env->GetMethodID(input, "read", "([B)I");
    jbyteArray buffer = env->NewByteArray(4096);
    for (;;) {
        jint n = env->CallIntMethod(stream, read, buffer);
        if (env->ExceptionCheck() || n <= 0) break;
        jbyte* bytes = env->GetByteArrayElements(buffer, nullptr);
        body.append((const char*)bytes, (size_t)n);
        env->ReleaseByteArrayElements(buffer, bytes, JNI_ABORT);
    }
    if (env->ExceptionCheck()) env->ExceptionClear();
    env->CallVoidMethod(stream, env->GetMethodID(input, "close", "()V"));
    if (env->ExceptionCheck()) env->ExceptionClear();
    env->CallVoidMethod(conn, env->GetMethodID(http, "disconnect", "()V"));
    if (env->ExceptionCheck()) env->ExceptionClear();
    if (attached) vm->DetachCurrentThread();
    return body;
}

static std::vector<std::string> MenuKeyWords(const std::string& text) {
    std::vector<std::string> words;
    std::string word;
    for (char c : text) {
        if (c == ' ' || c == '\t' || c == '\r') {
            if (!word.empty()) words.push_back(word);
            word.clear();
        } else {
            word.push_back(c);
        }
    }
    if (!word.empty()) words.push_back(word);
    return words;
}

static void MenuKeyCheck(std::string typed) {
    while (!typed.empty() && (typed.back() == '\n' || typed.back() == '\r' || typed.back() == ' ')) typed.pop_back();
    if (typed.empty()) {
        MenuKeySetStatus("Enter key");
        gMenuKeyBusy.store(false);
        return;
    }
    std::string body = MenuKeyFetch(MenuKeyAddress());
    if (body.empty()) {
        MenuKeySetStatus("Key list unavailable");
        gMenuKeyBusy.store(false);
        return;
    }
    std::vector<std::string> typedWords = MenuKeyWords(typed);
    bool found = false;
    std::string name;
    std::string until;
    bool expired = false;
    size_t start = 0;
    while (start <= body.size()) {
        size_t end = body.find('\n', start);
        if (end == std::string::npos) end = body.size();
        std::string line = body.substr(start, end - start);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        auto words = MenuKeyWords(line);
        if (words.size() >= 3) {
            int year = 0, month = 0, day = 0;
            bool token = false;
            for (const std::string& word : typedWords) token = token || MenuKeySame(word, words[words.size() - 2]);
            if (token && MenuKeyDate(words.back(), year, month, day)) {
                found = true;
                name = words.front();
                char date[32];
                snprintf(date, sizeof(date), "%04d-%02d-%02d", year, month, day);
                until = date;
                expired = MenuKeyPast(year, month, day);
                break;
            }
        }
        if (end == body.size()) break;
        start = end + 1;
    }
    if (!found) {
        MenuKeySetStatus("Invalid key");
        gMenuKeyBusy.store(false);
        return;
    }
    std::string token;
    for (const std::string& word : typedWords) {
        if (word.find('-') != std::string::npos && word.size() >= 6) token = word;
    }
    if (token.empty()) token = typedWords.size() >= 2 ? typedWords[typedWords.size() - 2] : typed;
    MenuKeyStore(token);
    char line[160];
    if (expired) snprintf(line, sizeof(line), "%s expired %s", name.c_str(), until.c_str());
    else snprintf(line, sizeof(line), "%s valid until %s", name.c_str(), until.c_str());
    MenuKeySetStatus(line);
    gMenuKeyBusy.store(false);
}

static void MenuKeyStart(const std::string& typed) {
    if (gMenuKeyBusy.exchange(true)) return;
    MenuKeySetStatus("Checking...");
    std::thread([typed]() { MenuKeyCheck(typed); }).detach();
}

static void MenuKeyEnsure() {
    if (gMenuKeyArmed) return;
    gMenuKeyArmed = true;
    MenuKeyLoad(gMenuKeyInput, sizeof(gMenuKeyInput));
    if (gMenuKeyInput[0]) MenuKeyStart(gMenuKeyInput);
}

static void MenuKeyStatusLine() {
    MenuKeyEnsure();
    ImGui::TextUnformatted(MenuKeyGetStatus().c_str());
}

static void DrawMenuKeyBox() {
    MenuKeyEnsure();
    ImGui::BeginGroupPanel("Menu key", ImVec2(-1, 0));
    ImGui::InputText("##menukey", gMenuKeyInput, sizeof(gMenuKeyInput));
    if (ImGui::Button("Check") && !gMenuKeyBusy.load()) MenuKeyStart(gMenuKeyInput);
    ImGui::TextWrapped("%s", MenuKeyGetStatus().c_str());
    ImGui::EndGroupPanel();
}
