#!/bin/bash

# CEServer Stealth 编译脚本
# 需要 Android NDK

set -e

ARCH=${1:-arm64}
BUILD_DIR="build_$ARCH"

# 检查 NDK
if [ -z "$ANDROID_NDK" ]; then
    # 尝试常见路径
    if [ -d "$HOME/Android/Sdk/ndk" ]; then
        ANDROID_NDK=$(ls -d $HOME/Android/Sdk/ndk/*/ 2>/dev/null | head -1)
    elif [ -d "/opt/android-ndk" ]; then
        ANDROID_NDK="/opt/android-ndk"
    fi
fi

if [ -z "$ANDROID_NDK" ] || [ ! -d "$ANDROID_NDK" ]; then
    echo "Error: ANDROID_NDK not set or not found"
    echo "Please set: export ANDROID_NDK=/path/to/ndk"
    exit 1
fi

echo "Using NDK: $ANDROID_NDK"

# 设置工具链
case $ARCH in
    arm64|aarch64)
        TARGET=aarch64-linux-android
        API=21
        ;;
    arm|armv7)
        TARGET=armv7a-linux-androideabi
        API=21
        ;;
    x86_64)
        TARGET=x86_64-linux-android
        API=21
        ;;
    x86)
        TARGET=i686-linux-android
        API=21
        ;;
    *)
        echo "Unknown arch: $ARCH"
        echo "Supported: arm64, arm, x86_64, x86"
        exit 1
        ;;
esac

TOOLCHAIN="$ANDROID_NDK/toolchains/llvm/prebuilt/linux-x86_64"
if [ ! -d "$TOOLCHAIN" ]; then
    TOOLCHAIN="$ANDROID_NDK/toolchains/llvm/prebuilt/darwin-x86_64"
fi

CC="$TOOLCHAIN/bin/${TARGET}${API}-clang"
STRIP="$TOOLCHAIN/bin/llvm-strip"

if [ ! -f "$CC" ]; then
    echo "Error: Compiler not found: $CC"
    exit 1
fi

echo "Compiler: $CC"
echo "Target: $TARGET"
echo "API Level: $API"

# 创建构建目录
mkdir -p $BUILD_DIR

# 编译 stealth 核心
echo "Compiling stealth_core.c..."
$CC -c stealth_core.c -o $BUILD_DIR/stealth_core.o \
    -O2 -fPIC -Wall \
    -DANDROID -D__ANDROID_API__=$API

# 下载并编译 CEServer 源码
CESERVER_DIR="ceserver_src"
if [ ! -d "$CESERVER_DIR" ]; then
    echo "Downloading CEServer source..."
    git clone --depth 1 https://github.com/cheat-engine/cheat-engine.git ce_temp
    mkdir -p $CESERVER_DIR
    cp -r "ce_temp/Cheat Engine/ceserver/"* $CESERVER_DIR/
    rm -rf ce_temp
fi

# 编译 CEServer (简化版，实际需要更多文件)
echo "Compiling CEServer with stealth..."

# 主要源文件
CESERVER_SOURCES="
    $CESERVER_DIR/ceserver.c
    $CESERVER_DIR/api.c
    $CESERVER_DIR/porthelp.c
    $CESERVER_DIR/symbols.c
    $CESERVER_DIR/threads.c
"

# 检查源文件是否存在
SOURCES_EXIST=true
for src in $CESERVER_SOURCES; do
    if [ ! -f "$src" ]; then
        echo "Warning: $src not found"
        SOURCES_EXIST=false
    fi
done

if [ "$SOURCES_EXIST" = true ]; then
    $CC $CESERVER_SOURCES $BUILD_DIR/stealth_core.o \
        -o $BUILD_DIR/ceserver-stealth \
        -O2 -fPIC -Wall \
        -DANDROID -D__ANDROID_API__=$API \
        -I. -I$CESERVER_DIR \
        -lpthread -ldl
    
    # Strip 符号
    $STRIP $BUILD_DIR/ceserver-stealth
    
    echo "=== Build Complete ==="
    ls -la $BUILD_DIR/ceserver-stealth
    file $BUILD_DIR/ceserver-stealth
else
    echo "CEServer source incomplete, building stealth library only..."
    
    # 编译为静态库
    ar rcs $BUILD_DIR/libstealth.a $BUILD_DIR/stealth_core.o
    
    echo "=== Stealth Library Built ==="
    ls -la $BUILD_DIR/libstealth.a
fi

echo ""
echo "To use on Android:"
echo "  adb push $BUILD_DIR/ceserver-stealth /data/local/tmp/"
echo "  adb shell 'su -c /data/local/tmp/ceserver-stealth'"
