//
// Created by Milk on 4/9/21.
//

#include "VmCore.h"
#include "utils/Log.h"
#include "IO.h"
#include <jni.h>
#include <jniHook/JniHook.h>
#include <jniHook/ArtM.h>
#include <hook/ProcessHook.h>
#include <hook/VMClassLoaderHook.h>
#include <hook/UnixFileSystemHook.h>
#include "DexDump.h"
#include "Utils/HexDump.h"
#import "xhook/xhook.h"

struct {
    JavaVM *vm;
    jclass VMCoreClass;
    jmethodID getCallingUidId;
    jmethodID redirectPathString;
    jmethodID redirectPathFile;
    jmethodID loadEmptyDex;
    jmethodID loadEmptyDexL;
    jmethodID findMethod;
    int api_level;
} VMEnv;

bool is_str_utf8(const char* str)
{
    unsigned int nBytes = 0;//UFT8可用1-6个字节编码,ASCII用一个字节
    unsigned char chr = *str;
    bool bAllAscii = true;
    for (unsigned int i = 0; str[i] != '\0'; ++i){
        chr = *(str + i);
        //判断是否ASCII编码,如果不是,说明有可能是UTF8,ASCII用7位编码,最高位标记为0,0xxxxxxx
        if (nBytes == 0 && (chr & 0x80) != 0){
            bAllAscii = false;
        }
        if (nBytes == 0) {
            //如果不是ASCII码,应该是多字节符,计算字节数
            if (chr >= 0x80) {
                if (chr >= 0xFC && chr <= 0xFD){
                    nBytes = 6;
                }
                else if (chr >= 0xF8){
                    nBytes = 5;
                }
                else if (chr >= 0xF0){
                    nBytes = 4;
                }
                else if (chr >= 0xE0){
                    nBytes = 3;
                }
                else if (chr >= 0xC0){
                    nBytes = 2;
                }
                else{
                    return false;
                }
                nBytes--;
            }
        }
        else{
            //多字节符的非首字节,应为 10xxxxxx
            if ((chr & 0xC0) != 0x80){
                return false;
            }
            //减到为零为止
            nBytes--;
        }
    }
    //违返UTF8编码规则
    if (nBytes != 0) {
        return false;
    }
    if (bAllAscii){ //如果全部都是ASCII, 也是UTF8
        return true;
    }
    return true;
}

JNIEnv *getEnv() {
    JNIEnv *env;
    VMEnv.vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);
    return env;
}

JNIEnv *ensureEnvCreated() {
    JNIEnv *env = getEnv();
    if (env == NULL) {
        VMEnv.vm->AttachCurrentThread(&env, NULL);
    }
    return env;
}

int VmCore::getCallingUid(JNIEnv *env, int orig) {
    env = ensureEnvCreated();
    return env->CallStaticIntMethod(VMEnv.VMCoreClass, VMEnv.getCallingUidId, orig);
}

jobject VmCore::findMethod(JNIEnv *env, const char *className, const char *methodName,
                           const char *signature) {
    try {
        if (!is_str_utf8(className) || !is_str_utf8(methodName) || !is_str_utf8(methodName)) {
            return nullptr;
        }
        auto clazz = (jclass) env->CallStaticObjectMethod(VMEnv.VMCoreClass, VMEnv.findMethod,
                                                          env->NewStringUTF(className),
                                                          env->NewStringUTF(methodName),
                                                          env->NewStringUTF(signature));
        env->ExceptionClear();
        return clazz;
    } catch (const char* &e) {
        ALOGE("catch findMethod %s", e);
        return nullptr;
    }
}

jstring VmCore::redirectPathString(JNIEnv *env, jstring path) {
    env = ensureEnvCreated();
    return (jstring) env->CallStaticObjectMethod(VMEnv.VMCoreClass, VMEnv.redirectPathString, path);
}

jobject VmCore::redirectPathFile(JNIEnv *env, jobject path) {
    env = ensureEnvCreated();
    return env->CallStaticObjectMethod(VMEnv.VMCoreClass, VMEnv.redirectPathFile, path);
}

jlongArray VmCore::loadEmptyDex(JNIEnv *env) {
    env = ensureEnvCreated();
    return (jlongArray) env->CallStaticObjectMethod(VMEnv.VMCoreClass, VMEnv.loadEmptyDex);
}

int VmCore::getApiLevel() {
    return VMEnv.api_level;
}

JavaVM *VmCore::getJavaVM() {
    return VMEnv.vm;
}

void nativeHook(JNIEnv *env) {
    BaseHook::init(env);
    UnixFileSystemHook::init(env);
//    VMClassLoaderHook::init(env);
    ProcessHook::init(env);
}

void hideXposed(JNIEnv *env, jclass clazz) {
    ALOGD("set hideXposed");
    VMClassLoaderHook::hideXposed();
}

void init(JNIEnv *env, jobject clazz, jint api_level) {
    ALOGD("VmCore init.");
    VMEnv.api_level = api_level;
    VMEnv.VMCoreClass = (jclass) env->NewGlobalRef(env->FindClass(VMCORE_CLASS));
    VMEnv.getCallingUidId = env->GetStaticMethodID(VMEnv.VMCoreClass, "getCallingUid", "(I)I");
    VMEnv.redirectPathString = env->GetStaticMethodID(VMEnv.VMCoreClass, "redirectPath",
                                                      "(Ljava/lang/String;)Ljava/lang/String;");
    VMEnv.redirectPathFile = env->GetStaticMethodID(VMEnv.VMCoreClass, "redirectPath",
                                                    "(Ljava/io/File;)Ljava/io/File;");
    VMEnv.loadEmptyDex = env->GetStaticMethodID(VMEnv.VMCoreClass, "loadEmptyDex",
                                                "()[J");
    VMEnv.findMethod = env->GetStaticMethodID(VMEnv.VMCoreClass, "findMethod",
                                              "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Ljava/lang/Object;");

    ArtM::InitArtMethod(env, api_level);
    JniHook::InitJniHook(env, api_level);
}

void addIORule(JNIEnv *env, jclass clazz, jstring target_path,
               jstring relocate_path) {
    IO::addRule(env->GetStringUTFChars(target_path, JNI_FALSE),
                env->GetStringUTFChars(relocate_path, JNI_FALSE));
}

void enableIO(JNIEnv *env, jclass clazz) {
    IO::init(env);
    nativeHook(env);
}

void hookDumpDex(JNIEnv *env, jobject clazz, jstring dir) {
    DexDump::hookDumpDex(env, dir);
}

void cookieDumpDex(JNIEnv *env, jclass clazz, jlong cookie, jstring dir, jboolean fixCodeItem) {
    DexDump::cookieDumpDex(env, cookie, dir, fixCodeItem);
}

static JNINativeMethod gMethods[] = {
        {"hideXposed",      "()V",                                     (void *) hideXposed},
        {"addIORule",       "(Ljava/lang/String;Ljava/lang/String;)V", (void *) addIORule},
        {"enableIO",        "()V",                                     (void *) enableIO},
        {"init",            "(I)V",                                    (void *) init},
        {"hookDumpDex",     "(Ljava/lang/String;)V",                   (void *) hookDumpDex},
        {"cookieDumpDex",   "(JLjava/lang/String;Z)V",                 (void *) cookieDumpDex},
};

int registerNativeMethods(JNIEnv *env, const char *className,
                          JNINativeMethod *gMethods, int numMethods) {
    jclass clazz;
    clazz = env->FindClass(className);
    if (clazz == nullptr) {
        return JNI_FALSE;
    }
    if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
        return JNI_FALSE;
    }
    return JNI_TRUE;
}

int registerNatives(JNIEnv *env) {
    if (!registerNativeMethods(env, VMCORE_CLASS, gMethods,
                               sizeof(gMethods) / sizeof(gMethods[0])))
        return JNI_FALSE;
    return JNI_TRUE;
}

void registerMethod(JNIEnv *jenv) {
    registerNatives(jenv);
}

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    VMEnv.vm = vm;
    if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_EVERSION;
    }
    registerMethod(env);
    return JNI_VERSION_1_6;
}