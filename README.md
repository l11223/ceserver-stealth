# CEServer Stealth - Android 隐蔽版

专为 Android root 设备设计的隐蔽版 Cheat Engine Server。

## 功能特性

### 反检测
- 进程名随机化 - 伪装成系统服务
- 端口随机化 - 避免默认端口检测
- 内存特征隐藏 - 清除字符串特征
- /proc 隐藏 - 隐藏进程信息

### 通信安全
- 流量加密 - XOR + 动态密钥
- 心跳伪装 - 模拟正常网络活动

## 编译

需要 Android NDK。

```bash
# 设置 NDK 路径
export ANDROID_NDK=/path/to/ndk

# 编译 ARM64
./build.sh arm64

# 编译 ARM32
./build.sh arm
```

## 使用

```bash
# 推送到手机
adb push ceserver-stealth /data/local/tmp/

# 运行 (root)
adb shell
su
cd /data/local/tmp
chmod +x ceserver-stealth
./ceserver-stealth
```

## 配置

启动参数：
- `-p <port>` - 指定端口 (默认随机)
- `-n <name>` - 指定进程名 (默认随机)
- `-d` - 后台运行
- `-v` - 详细输出
