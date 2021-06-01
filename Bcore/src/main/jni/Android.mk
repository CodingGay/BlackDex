LOCAL_PATH := $(call my-dir)
MAIN_LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
    LOCAL_MODULE := vm64
    LOCAL_CFLAGS := -DCORE_SO_NAME=\"libvm64.so\"
else
    LOCAL_MODULE := vm
    LOCAL_CFLAGS := -DCORE_SO_NAME=\"libvm.so\"
endif

LOCAL_CFLAGS += -Wno-error=format-security -fpermissive -O2
LOCAL_CFLAGS += -DLOG_TAG=\"VM++\"
LOCAL_CFLAGS += -fno-rtti -fno-exceptions
LOCAL_CPPFLAGS += -std=c++11

LOCAL_C_INCLUDES += $(MAIN_LOCAL_PATH)

LOCAL_SRC_FILES := JniHook/JniHook.cpp \
                   Utils/fake_dlfcn.cpp \
                   Utils/HexDump.cpp \
                   Utils/PointerCheck.cpp \
                   IO.cpp \
                   Hook/BaseHook.cpp \
                   Hook/RuntimeHook.cpp \
                   Hook/VMClassLoaderHook.cpp \
                   Hook/UnixFileSystemHook.cpp \
                   Hook/BinderHook.cpp \
                   DexDump.cpp \
                   VmCore.cpp \

LOCAL_LDLIBS := -llog -latomic

include $(BUILD_SHARED_LIBRARY)