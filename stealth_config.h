#ifndef STEALTH_CONFIG_H
#define STEALTH_CONFIG_H

// ============================================
// CEServer Stealth 配置
// ============================================

// 进程伪装名称列表 (看起来像系统服务)
static const char* FAKE_PROCESS_NAMES[] = {
    "system_server",
    "surfaceflinger", 
    "servicemanager",
    "vold",
    "netd",
    "installd",
    "lmkd",
    "logd",
    "healthd",
    "storaged",
    "thermalserviced",
    "audioserver",
    "cameraserver",
    "mediaserver",
    "gpuservice",
    "hwservicemanager",
    "vendor.qti.hardware",
    "android.hardware.sensors",
    "android.hardware.graphics",
    "com.android.phone",
    NULL
};

// 默认端口范围 (随机选择)
#define STEALTH_PORT_MIN 49152
#define STEALTH_PORT_MAX 65535

// 通信加密密钥 (运行时随机生成)
#define ENCRYPTION_ENABLED 1

// 心跳间隔 (毫秒)
#define HEARTBEAT_INTERVAL 5000

// 隐藏 /proc/self/maps 中的特征
#define HIDE_PROC_MAPS 1

// 清除内存中的字符串特征
#define CLEAR_STRING_SIGNATURES 1

// 反调试检测
#define ANTI_DEBUG 1

// 反 ptrace
#define ANTI_PTRACE 1

#endif // STEALTH_CONFIG_H
