#include <android/log.h>
#include <libgen.h>
#include <fcntl.h>
#include <inttypes.h>
#include <jni.h>
#include <unistd.h>
#include <sys/mman.h>
#include <time.h>
#include <vector>

#include "Tools.h"
#include "Includes/DOSOBF.h"

#if defined(__arm__)
#define process_vm_readv_syscall 376
#define process_vm_writev_syscall 377
#elif defined(__aarch64__)
#define process_vm_readv_syscall 270
#define process_vm_writev_syscall 271
#elif defined(__i386__)
#define process_vm_readv_syscall 347
#define process_vm_writev_syscall 348
#else
#define process_vm_readv_syscall 310
#define process_vm_writev_syscall 311
#endif

pid_t target_pid = -1;
#define INRANGE(x, a, b)        (x >= a && x <= b)
#define getBits(x)              (INRANGE(x,'0','9') ? (x - '0') : ((x&(~0x20)) - 'A' + 0xa))
#define getByte(x)              (getBits(x[0]) << 4 | getBits(x[1]))

ssize_t process_v(pid_t __pid, const struct iovec *__local_iov, unsigned long __local_iov_count, const struct iovec *__remote_iov, unsigned long __remote_iov_count, unsigned long __flags, bool iswrite) {
    return syscall((iswrite ? process_vm_writev_syscall : process_vm_readv_syscall), __pid, __local_iov, __local_iov_count, __remote_iov, __remote_iov_count, __flags);
}

bool pvm(void *address, void *buffer, size_t size, bool write = false) {
    struct iovec local[1];
    struct iovec remote[1];

    local[0].iov_base = buffer;
    local[0].iov_len = size;
    remote[0].iov_base = address;
    remote[0].iov_len = size;

    if (target_pid == -1) {
        target_pid = getpid();
    }

    ssize_t bytes = process_v(target_pid, local, 1, remote, 1, 0, write);
    return bytes == size;
}

void Tools::Hook(void *target, void *replace, void **backup) {
    unsigned long page_size = sysconf(_SC_PAGESIZE);
    unsigned long size = page_size * sizeof(uintptr_t);
    void *p = (void *) ((uintptr_t) target - ((uintptr_t) target % page_size) - page_size);
    if (mprotect(p, (size_t) size, PROT_EXEC | PROT_READ | PROT_WRITE) == 0) {
		DobbyHook(target, replace, backup);
    }
}

bool Tools::Read(void *addr, void *buffer, size_t length) {
	return memcpy(buffer, addr, length) != 0;
}

bool Tools::Write(void *addr, void *buffer, size_t length) {
	return memcpy(addr, buffer, length) != 0;
}

bool Tools::ReadAddr(void *addr, void *buffer, size_t length) {
	unsigned long page_size = sysconf(_SC_PAGESIZE);
	unsigned long size = page_size * sizeof(uintptr_t);
	return mprotect((void *) ((uintptr_t) addr - ((uintptr_t) addr % page_size) - page_size), (size_t) size, PROT_EXEC | PROT_READ | PROT_WRITE) == 0 && memcpy(buffer, addr, length) != 0;
}

bool Tools::WriteAddr(void *addr, void *buffer, size_t length) {
	unsigned long page_size = sysconf(_SC_PAGESIZE);
    unsigned long size = page_size * sizeof(uintptr_t);
    return mprotect((void *) ((uintptr_t) addr - ((uintptr_t) addr % page_size) - page_size), (size_t) size, PROT_EXEC | PROT_READ | PROT_WRITE) == 0 && memcpy(addr, buffer, length) != 0;
}

bool Tools::PVM_ReadAddr(void *addr, void *buffer, size_t length) {
    return pvm(addr, buffer, length, false);
}

bool Tools::PVM_WriteAddr(void *addr, void *buffer, size_t length) {
    return pvm(addr, buffer, length, true);
}

bool Tools::IsPtrValid(void *addr) {
    uintptr_t p = (uintptr_t)addr;
    if (p < 0x10000) return false;

    struct Range { uintptr_t a, b; };
    static std::vector<Range> ranges;
    static long long stamp = -10000;

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    long long now = (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
    if (ranges.empty() || now - stamp > 250) {
        FILE* f = fopen(DOS("/proc/self/maps"), DOS("r"));
        if (!f) {
            static int fd = -1;
            if (fd == -1) fd = open(DOS("/dev/random"), O_WRONLY);
            return write(fd, addr, 4) >= 0;
        }
        std::vector<Range> next;
        next.reserve(ranges.empty() ? 256 : ranges.size());
        char line[320];
        while (fgets(line, sizeof line, f)) {
            uintptr_t s = 0, e = 0;
            char perms[5] = {};
            if (sscanf(line, "%" SCNxPTR "-%" SCNxPTR " %4s", &s, &e, perms) == 3 && perms[0] == 'r' && e > s)
                next.push_back({s, e});
        }
        fclose(f);
        if (!next.empty()) {
            ranges.swap(next);
            stamp = now;
        }
    }
    if (ranges.empty()) return false;

    size_t lo = 0, hi = ranges.size();
    while (lo < hi) {
        size_t mid = (lo + hi) >> 1;
        if (ranges[mid].b <= p) lo = mid + 1;
        else hi = mid;
    }
    if (lo >= ranges.size()) return false;
    return p >= ranges[lo].a && (p + 4) <= ranges[lo].b;
}

uintptr_t Tools::GetBaseAddress(const char *name) {
    uintptr_t base = 0;
    char line[512];

    FILE *f = fopen(DOS("/proc/self/maps"), DOS("r"));

    if (!f) {
        return 0;
    }
    while (fgets(line, sizeof line, f)) {
        uintptr_t tmpBase;
        char tmpName[256];
        if (sscanf(line, "%" PRIXPTR "-%*" PRIXPTR "%*s %*s %*s %*s %s", &tmpBase, tmpName) > 0) {
            if (!strcmp(basename(tmpName), name)) {
                base = tmpBase;
                break;
            }
        }
    }

    fclose(f);
    return base;
}

uintptr_t Tools::GetRealOffsets(const char *libraryName, uintptr_t relativeAddr) {
	uintptr_t libBase = Tools::GetBaseAddress(libraryName);
	if (libBase == 0)
		return 0;
	return (reinterpret_cast<uintptr_t>(libBase + relativeAddr));
}


uintptr_t Tools::GetEndAddress(const char *name) {
    uintptr_t end = 0;
    char line[512];

    FILE *f = fopen(DOS("/proc/self/maps"), DOS("r"));

    if (!f) {
        return 0;
    }

    bool found = false;
    while (fgets(line, sizeof line, f)) {
        uintptr_t tmpEnd;
        char tmpName[256];
        if (sscanf(line, "%*" PRIXPTR "-%" PRIXPTR "%*s %*s %*s %*s %s", &tmpEnd, tmpName) > 0) {
            if (!strcmp(basename(tmpName), name)) {
                if (!found) {
                    found = true;
                }
            } else {
                if (found) {
                    end = tmpEnd;
                    break;
                }
            }
        }
    }

    fclose(f);
    return end;
}


uintptr_t Tools::FindPattern(const char *lib, const char *pattern) {
    auto start = GetBaseAddress(lib);
    if (!start)
        return 0;
    auto end = GetEndAddress(lib);
    if (!end)
        return 0;
    auto curPat = reinterpret_cast<const unsigned char *>(pattern);
    uintptr_t firstMatch = 0;
    for (uintptr_t pCur = start; pCur < end; ++pCur) {
        if (*(uint8_t *) curPat == (uint8_t) '\?' || *(uint8_t *) pCur == getByte(curPat)) {
            if (!firstMatch) {
                firstMatch = pCur;
            }
            curPat += (*(uint16_t *) curPat == (uint16_t) '\?\?' || *(uint8_t *) curPat != (uint8_t) '\?') ? 2 : 1;
            if (!*curPat) {
                return firstMatch;
            }
            curPat++;
            if (!*curPat) {
                return firstMatch;
            }
        } else if (firstMatch) {
            pCur = firstMatch;
            curPat = reinterpret_cast<const unsigned char *>(pattern);
            firstMatch = 0;
        }
    }
    return 0;
}

std::string Tools::RandomString(const int len) {
    static const char alphanumerics[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    srand((unsigned) time(0) * getpid());
    std::string tmp;
    tmp.reserve(len);
    for (int i = 0; i < len; ++i) {
        tmp += alphanumerics[rand() % (sizeof(alphanumerics) - 1)];
    }
    return tmp;
}

std::string Tools::GetPackageName(JNIEnv *env, jobject context) {
    jclass contextClass = env->FindClass("android/content/Context");
    jmethodID getPackageNameId = env->GetMethodID(contextClass, "getPackageName", "()Ljava/lang/String;");
    auto str = (jstring) env->CallObjectMethod(context, getPackageNameId);
    return env->GetStringUTFChars(str, 0);
}
