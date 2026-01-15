#ifndef STEALTH_CORE_H
#define STEALTH_CORE_H

#include <stdint.h>
#include <stdbool.h>

// 初始化隐蔽模块
int stealth_init(void);

// 进程名伪装
int stealth_set_process_name(const char* name);
const char* stealth_get_random_process_name(void);

// 端口随机化
uint16_t stealth_get_random_port(void);

// 通信加密
void stealth_encrypt(uint8_t* data, size_t len, uint32_t key);
void stealth_decrypt(uint8_t* data, size_t len, uint32_t key);
uint32_t stealth_generate_key(void);

// 反检测
bool stealth_check_debugger(void);
bool stealth_check_frida(void);
bool stealth_check_xposed(void);
int stealth_hide_from_proc(void);

// 内存清理
void stealth_clear_signatures(void);

// 反 ptrace
int stealth_anti_ptrace(void);

#endif // STEALTH_CORE_H
