//
// Created by Milk on 4/9/21.
//

#ifndef VIRTUALM_UNIXFILESYSTEMHOOK_H
#define VIRTUALM_UNIXFILESYSTEMHOOK_H


#include "BaseHook.h"

class UnixFileSystemHook : public BaseHook {
public:
    static void init(JNIEnv *env);
};


#endif //VIRTUALM_UNIXFILESYSTEMHOOK_H
