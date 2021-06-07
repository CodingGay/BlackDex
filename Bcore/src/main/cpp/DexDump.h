//
// Created by Milk on 2021/5/16.
//

#ifndef BLACKBOX_DEXDUMP_H
#define BLACKBOX_DEXDUMP_H
#include <jni.h>

#define HOOK_FUN(ret, func, ...) \
  ret (*orig_##func)(__VA_ARGS__); \
  ret new_##func(__VA_ARGS__)

class DexDump {
public:
    static void dumpDex(JNIEnv *env, jlong cookie, jstring dir, jboolean fix);
};


#endif //BLACKBOX_DEXDUMP_H
