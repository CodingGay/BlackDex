//
// Created by Milk on 5/5/21.
//

#ifndef BLACKBOX_RUNTIMEHOOK_H
#define BLACKBOX_RUNTIMEHOOK_H


#include "BaseHook.h"
#include <jni.h>

class RuntimeHook : public BaseHook {
public:
    static void init(JNIEnv *env);
};


#endif //BLACKBOX_RUNTIMEHOOK_H
