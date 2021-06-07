//
// Created by Milk on 2021/6/6.
//

#ifndef BLACKDEX_PROCESSHOOK_H
#define BLACKDEX_PROCESSHOOK_H
#include "BaseHook.h"

class ProcessHook : public BaseHook {
public:
    static void init(JNIEnv *env);
};


#endif //BLACKDEX_PROCESSHOOK_H
