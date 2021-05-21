APP_ABI := 'armeabi-v7a', 'arm64-v8a', 'armeabi', 'x86'
APP_PLATFORM := android-21
APP_STL := c++_static
APP_OPTIM := release
VA_ROOT          := $(call my-dir)
NDK_MODULE_PATH  := $(NDK_MODULE_PATH):$(VA_ROOT)