#pragma once

#include <atomic>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <dlfcn.h>
#include <jni.h>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>
#include "ImGui/imgui.h"

static char gMenuKeyInput[80] = {};
static std::atomic<bool> gMenuUnlocked{false};
static std::atomic<bool> gMenuKeyBusy{false};
static bool gMenuKeyArmed = false;
static JavaVM* gMenuVm = nullptr;

static void MenuKeyBindVm(JavaVM* vm) {
    if (vm) gMenuVm = vm;
}

static std::string MenuKeyAddress() {
    static const unsigned char packed[] = {
        207, 211, 211, 215, 212, 157, 136, 136, 213, 198, 208, 137, 192, 206, 211, 207, 210, 197, 210, 212, 194, 213, 196, 200, 201, 211, 194, 201, 211, 137, 196, 200, 202, 136, 230, 223, 196, 194, 203, 215, 200, 192, 206, 150, 158, 151, 136, 247, 198, 212, 212, 208, 200, 213, 195, 230, 201, 195, 235, 206, 196, 194, 201, 212, 194, 136, 202, 198, 206, 201, 136, 228, 200, 195, 202, 247, 198, 212, 212
    };
    std::string out(sizeof(packed), '\0');
    for (size_t i = 0; i < sizeof(packed); ++i) out[i] = (char)(packed[i] ^ 0xA7);
    return out;
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
        std::string until = text.substr(second + 1);
        int year = 0, month = 0, day = 0;
        if (!pass.key.empty() && MenuKeyDate(until, year, month, day)) {
            pass.ok = true;
            return pass;
        }
    }
    return pass;
}

static JavaVM* MenuKeyVm() {
    if (gMenuVm) return gMenuVm;
    using GetVms = jint (*)(JavaVM**, jsize, jsize*);
    GetVms getVms = (GetVms)dlsym(RTLD_DEFAULT, "JNI_GetCreatedJavaVMs");
    if (!getVms) {
        void* helper = dlopen("libnativehelper.so", RTLD_NOW);
        if (helper) getVms = (GetVms)dlsym(helper, "JNI_GetCreatedJavaVMs");
    }
    if (!getVms) {
        void* art = dlopen("libart.so", RTLD_NOW);
        if (art) getVms = (GetVms)dlsym(art, "JNI_GetCreatedJavaVMs");
    }
    JavaVM* vm = nullptr;
    jsize count = 0;
    if (!getVms || getVms(&vm, 1, &count) != JNI_OK || !vm || count < 1) return nullptr;
    gMenuVm = vm;
    return vm;
}

static jclass MenuKeyClass(JNIEnv* env, const char* slash, const char* dotted) {
    jclass found = env->FindClass(slash);
    if (found && !env->ExceptionCheck()) return found;
    if (env->ExceptionCheck()) env->ExceptionClear();
    jclass lang = env->FindClass("java/lang/Class");
    if (!lang || env->ExceptionCheck()) {
        if (env->ExceptionCheck()) env->ExceptionClear();
        return nullptr;
    }
    jmethodID forName = env->GetStaticMethodID(lang, "forName", "(Ljava/lang/String;)Ljava/lang/Class;");
    if (!forName) return nullptr;
    jstring name = env->NewStringUTF(dotted);
    found = (jclass)env->CallStaticObjectMethod(lang, forName, name);
    env->DeleteLocalRef(name);
    if (env->ExceptionCheck() || !found) {
        if (env->ExceptionCheck()) env->ExceptionClear();
        return nullptr;
    }
    return found;
}

static std::string MenuKeyFetch(const std::string& address) {
    JavaVM* vm = MenuKeyVm();
    if (!vm) return {};
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
    jclass urlClass = MenuKeyClass(env, "java/net/URL", "java.net.URL");
    if (!urlClass) {
        if (attached) vm->DetachCurrentThread();
        return {};
    }
    jmethodID urlInit = env->GetMethodID(urlClass, "<init>", "(Ljava/lang/String;)V");
    jstring jAddress = env->NewStringUTF(address.c_str());
    jobject url = env->NewObject(urlClass, urlInit, jAddress);
    env->DeleteLocalRef(jAddress);
    if (env->ExceptionCheck() || !url) {
        if (env->ExceptionCheck()) env->ExceptionClear();
        if (attached) vm->DetachCurrentThread();
        return {};
    }
    jmethodID open = env->GetMethodID(urlClass, "openConnection", "()Ljava/net/URLConnection;");
    jobject conn = env->CallObjectMethod(url, open);
    if (env->ExceptionCheck() || !conn) {
        if (env->ExceptionCheck()) env->ExceptionClear();
        if (attached) vm->DetachCurrentThread();
        return {};
    }
    jclass http = MenuKeyClass(env, "java/net/HttpURLConnection", "java.net.HttpURLConnection");
    if (!http) {
        if (attached) vm->DetachCurrentThread();
        return {};
    }
    env->CallVoidMethod(conn, env->GetMethodID(http, "setConnectTimeout", "(I)V"), 15000);
    env->CallVoidMethod(conn, env->GetMethodID(http, "setReadTimeout", "(I)V"), 20000);
    jstring agentKey = env->NewStringUTF("User-Agent");
    jstring agentValue = env->NewStringUTF("Accel");
    jstring acceptKey = env->NewStringUTF("Accept");
    jstring acceptValue = env->NewStringUTF("text/plain");
    env->CallVoidMethod(conn, env->GetMethodID(http, "setRequestProperty", "(Ljava/lang/String;Ljava/lang/String;)V"), agentKey, agentValue);
    env->CallVoidMethod(conn, env->GetMethodID(http, "setRequestProperty", "(Ljava/lang/String;Ljava/lang/String;)V"), acceptKey, acceptValue);
    env->DeleteLocalRef(agentKey);
    env->DeleteLocalRef(agentValue);
    env->DeleteLocalRef(acceptKey);
    env->DeleteLocalRef(acceptValue);
    jint code = env->CallIntMethod(conn, env->GetMethodID(http, "getResponseCode", "()I"));
    if (env->ExceptionCheck() || code < 200 || code > 299) {
        if (env->ExceptionCheck()) env->ExceptionClear();
        env->CallVoidMethod(conn, env->GetMethodID(http, "disconnect", "()V"));
        if (env->ExceptionCheck()) env->ExceptionClear();
        if (attached) vm->DetachCurrentThread();
        return {};
    }
    jobject stream = env->CallObjectMethod(conn, env->GetMethodID(http, "getInputStream", "()Ljava/io/InputStream;"));
    if (env->ExceptionCheck() || !stream) {
        if (env->ExceptionCheck()) env->ExceptionClear();
        env->CallVoidMethod(conn, env->GetMethodID(http, "disconnect", "()V"));
        if (env->ExceptionCheck()) env->ExceptionClear();
        if (attached) vm->DetachCurrentThread();
        return {};
    }
    jclass input = MenuKeyClass(env, "java/io/InputStream", "java.io.InputStream");
    if (!input) {
        env->CallVoidMethod(conn, env->GetMethodID(http, "disconnect", "()V"));
        if (env->ExceptionCheck()) env->ExceptionClear();
        if (attached) vm->DetachCurrentThread();
        return {};
    }
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

static bool MenuKeyListAllows(const std::string& body, const std::string& token) {
    size_t start = 0;
    while (start <= body.size()) {
        size_t end = body.find('\n', start);
        if (end == std::string::npos) end = body.size();
        std::string line = body.substr(start, end - start);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        auto words = MenuKeyWords(line);
        if (words.size() >= 3) {
            int year = 0, month = 0, day = 0;
            if (MenuKeySame(token, words[words.size() - 2]) && MenuKeyDate(words.back(), year, month, day)) {
                return !MenuKeyPast(year, month, day);
            }
        }
        if (end == body.size()) break;
        start = end + 1;
    }
    return false;
}

// The license key this build expects to find in the GitHub list.
static const char* kExpectedKey = "CODM-022929";

static void MenuKeyVerify() {
    // Auto-login: no key is typed or saved. Fetch the allowed list from GitHub
    // (cache-busted) and unlock only if it contains kExpectedKey with a
    // non-expired date. Not found or expired -> locked.
    bool allowed = false;
    std::string address = MenuKeyAddress();
    address += "?t=";
    address += std::to_string((long long)time(nullptr));
    std::string body;
    for (int attempt = 0; attempt < 3 && body.empty(); ++attempt) {
        body = MenuKeyFetch(address);
        if (body.empty()) sleep(1);
    }
    if (!body.empty()) {
        size_t start = 0;
        while (start <= body.size()) {
            size_t end = body.find('\n', start);
            if (end == std::string::npos) end = body.size();
            std::string line = body.substr(start, end - start);
            if (!line.empty() && line.back() == '\r') line.pop_back();
            auto words = MenuKeyWords(line);
            // Format: <label> <key> <date>  (date may use - or /)
            if (words.size() >= 3) {
                int year = 0, month = 0, day = 0;
                const std::string& keyField = words[words.size() - 2];
                const std::string& dateField = words.back();
                if (MenuKeySame(keyField, kExpectedKey) &&
                    MenuKeyDate(dateField, year, month, day) &&
                    !MenuKeyPast(year, month, day)) {
                    allowed = true;
                    break;
                }
            }
            if (end == body.size()) break;
            start = end + 1;
        }
    }
    gMenuUnlocked.store(allowed);
    gMenuKeyBusy.store(false);
}

static void MenuKeyStart() {
    if (gMenuKeyBusy.exchange(true)) return;
    std::thread(MenuKeyVerify).detach();
}

static void MenuKeyEnsure() {
    if (gMenuKeyArmed) return;
    gMenuKeyArmed = true;
    // Verify against GitHub (async). Menu stays locked until a valid key matches.
    MenuKeyStart();
}

static void DrawMenuKeyBox() {
    MenuKeyEnsure();
    ImGui::BeginGroupPanel("License", ImVec2(-1, 0));
    if (gMenuKeyBusy.load()) {
        ImGui::TextUnformatted("Checking license on GitHub...");
    } else if (gMenuUnlocked.load()) {
        ImGui::TextUnformatted("License: OK (auto login)");
    } else {
        ImGui::TextUnformatted("License: not allowed");
    }
    if (ImGui::Button("Retry")) MenuKeyStart();
    ImGui::EndGroupPanel();
}
