//
// Created by Milk on 4/9/21.
//

#ifndef VIRTUALM_BASEHOOK_H
#define VIRTUALM_BASEHOOK_H

#include <jni.h>
#include <Log.h>

class BaseHook {
public:
    static void init(JNIEnv *env);
};


#endif //VIRTUALM_BASEHOOK_H
