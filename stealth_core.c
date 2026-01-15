#include "stealth_core.h"
#include "stealth_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/prctl.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <pthread.h>
#include <dlfcn.h>
#include <errno.h>

// ============================================
// 进程名伪装
// ============================================

static char original_process_name[256] = {0};

const char* stealth_get_random_process_name(void) {
    int count = 0;
    while (FAKE_PROCESS_NAMES[count] != NULL) count++;
    
    srand(time(NULL) ^ getpid());
    return FAKE_PROCESS_NAMES[rand() % count];
}

int stealth_set_process_name(const char* name) {
    // 保存原始名称
    if (original_process_name[0] == 0) {
        prctl(PR_GET_NAME, original_process_name);
    }
    
    // 设置新进程名
    if (prctl(PR_SET_NAME, name) != 0) {
        return -1;
    }
    
    // 修改 /proc/self/comm
    int fd = open("/proc/self/comm", O_WRONLY);
    if (fd >= 0) {
        write(fd, name, strlen(name));
        close(fd);
    }
    
    // 修改 argv[0] (需要在 main 中处理)
    
    return 0;
}

// ============================================
// 端口随机化
// ============================================

uint16_t stealth_get_random_port(void) {
    srand(time(NULL) ^ getpid() ^ (uintptr_t)&stealth_get_random_port);
    return STEALTH_PORT_MIN + (rand() % (STEALTH_PORT_MAX - STEALTH_PORT_MIN));
}

// ============================================
// 通信加密 (简单 XOR，可以升级为 ChaCha20)
// ============================================

uint32_t stealth_generate_key(void) {
    uint32_t key = 0;
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd >= 0) {
        read(fd, &key, sizeof(key));
        close(fd);
    } else {
        key = (uint32_t)(time(NULL) ^ getpid() ^ (uintptr_t)&key);
    }
    return key;
}

void stealth_encrypt(uint8_t* data, size_t len, uint32_t key) {
    uint8_t* key_bytes = (uint8_t*)&key;
    for (size_t i = 0; i < len; i++) {
        data[i] ^= key_bytes[i % 4];
        data[i] = (data[i] << 3) | (data[i] >> 5); // 简单位旋转
    }
}

void stealth_decrypt(uint8_t* data, size_t len, uint32_t key) {
    uint8_t* key_bytes = (uint8_t*)&key;
    for (size_t i = 0; i < len; i++) {
        data[i] = (data[i] >> 3) | (data[i] << 5); // 反向位旋转
        data[i] ^= key_bytes[i % 4];
    }
}

// ============================================
// 反调试检测
// ============================================

bool stealth_check_debugger(void) {
    // 检查 /proc/self/status 中的 TracerPid
    FILE* f = fopen("/proc/self/status", "r");
    if (f) {
        char line[256];
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "TracerPid:", 10) == 0) {
                int tracer_pid = atoi(line + 10);
                fclose(f);
                return tracer_pid != 0;
            }
        }
        fclose(f);
    }
    return false;
}

bool stealth_check_frida(void) {
    // 检查 Frida 特征
    
    // 1. 检查 frida-server 进程
    DIR* dir = opendir("/proc");
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_DIR) {
                char path[512];
                snprintf(path, sizeof(path), "/proc/%s/cmdline", entry->d_name);
                FILE* f = fopen(path, "r");
                if (f) {
                    char cmdline[256] = {0};
                    fread(cmdline, 1, sizeof(cmdline) - 1, f);
                    fclose(f);
                    if (strstr(cmdline, "frida") || strstr(cmdline, "gum-js-loop")) {
                        closedir(dir);
                        return true;
                    }
                }
            }
        }
        closedir(dir);
    }
    
    // 2. 检查 frida 端口 (27042)
    FILE* f = fopen("/proc/net/tcp", "r");
    if (f) {
        char line[256];
        while (fgets(line, sizeof(line), f)) {
            if (strstr(line, "69B2") || strstr(line, "69B3")) { // 27042, 27043 hex
                fclose(f);
                return true;
            }
        }
        fclose(f);
    }
    
    // 3. 检查 frida 相关库
    void* handle = dlopen("libfrida-gadget.so", RTLD_NOW);
    if (handle) {
        dlclose(handle);
        return true;
    }
    
    return false;
}

bool stealth_check_xposed(void) {
    // 检查 Xposed 框架
    
    // 1. 检查系统属性
    char value[256] = {0};
    FILE* f = popen("getprop ro.build.version.sdk", "r");
    if (f) {
        fgets(value, sizeof(value), f);
        pclose(f);
    }
    
    // 2. 检查 Xposed 相关文件
    const char* xposed_files[] = {
        "/system/framework/XposedBridge.jar",
        "/system/lib/libxposed_art.so",
        "/system/lib64/libxposed_art.so",
        "/data/data/de.robv.android.xposed.installer",
        "/data/data/io.va.exposed",
        NULL
    };
    
    for (int i = 0; xposed_files[i] != NULL; i++) {
        if (access(xposed_files[i], F_OK) == 0) {
            return true;
        }
    }
    
    return false;
}

// ============================================
// 反 ptrace
// ============================================

int stealth_anti_ptrace(void) {
#if ANTI_PTRACE
    // 自己 ptrace 自己，防止被其他进程 attach
    if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) == -1) {
        // 已经被 ptrace 了
        return -1;
    }
#endif
    return 0;
}

// ============================================
// 隐藏 /proc 信息
// ============================================

int stealth_hide_from_proc(void) {
#if HIDE_PROC_MAPS
    // 这需要 root 权限和内核支持
    // 简单方案：修改 /proc/self/maps 的可读性（需要 SELinux 配合）
    
    // 更高级的方案需要内核模块或 eBPF
#endif
    return 0;
}

// ============================================
// 清除内存特征
// ============================================

void stealth_clear_signatures(void) {
#if CLEAR_STRING_SIGNATURES
    // 清除一些明显的字符串特征
    // 这个函数应该在初始化后调用
    
    // 注意：这只是示例，实际需要更复杂的实现
#endif
}

// ============================================
// 初始化
// ============================================

int stealth_init(void) {
    // 1. 反 ptrace
    if (stealth_anti_ptrace() != 0) {
        fprintf(stderr, "[!] Anti-ptrace failed, may be debugged\n");
    }
    
    // 2. 检测调试器
    if (stealth_check_debugger()) {
        fprintf(stderr, "[!] Debugger detected!\n");
        return -1;
    }
    
    // 3. 检测 Frida
    if (stealth_check_frida()) {
        fprintf(stderr, "[!] Frida detected!\n");
        return -1;
    }
    
    // 4. 设置随机进程名
    const char* fake_name = stealth_get_random_process_name();
    stealth_set_process_name(fake_name);
    
    // 5. 清除特征
    stealth_clear_signatures();
    
    // 6. 隐藏 /proc
    stealth_hide_from_proc();
    
    return 0;
}
