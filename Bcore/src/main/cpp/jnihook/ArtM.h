//
// Created by Milk on 2021/6/5.
//

#ifndef BLACKDEX_ARTM_H
#define BLACKDEX_ARTM_H

#include <jni.h>

class ArtM {
public:
    static void InitArtMethod(JNIEnv *env, int api_level);

    static uint32_t GetArtMethodSize();

    static uint32_t GetArtMethodFlagsOffset();

    static uint32_t GetArtMethodDexCodeItemOffset(void *art_method);

    static uint32_t GetArtMethodNativeOffset();

    static void *GetArtMethod(JNIEnv *env, jclass clazz, jmethodID methodId);

    static void *GetArtMethod(JNIEnv *env, jobject method);

    static uint32_t GetAccessFlags(const char *art_method);

    static bool SetAccessFlags(char *art_method, uint32_t flags);

    static bool AddAccessFlag(char *art_method, uint32_t flag);

    static bool ClearAccessFlag(char *art_method, uint32_t flag);

    static bool HasAccessFlag(char *art_method, uint32_t flag);

    static bool ClearFastNativeFlag(char *art_method);
};
#endif //BLACKDEX_ARTM_H
