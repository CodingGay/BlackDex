//
// Created by Milk on 2021/6/6.
//

#include <sys/types.h>
#include "ProcessHook.h"
#import "jniHook/JniHook.h"
#import "utils/Log.h"
#import "xhook/xhook.h"

HOOK_JNI(void, sendSignal, JNIEnv *env, jobject obj, jint pid, jint signal) {
    ALOGE("hooked sendSignal");
}

HOOK_JNI(void, sendSignalQuiet, JNIEnv *env, jobject obj, jint pid, jint signal) {
    ALOGE("hooked sendSignalQuiet");
}

HOOK_JNI(jint, killProcessGroup, JNIEnv *env, jobject obj, jint uid, jint pid) {
    ALOGE("hooked killProcessGroup");
    return 0;
}

void ProcessHook::init(JNIEnv *env) {
    const char *className = "android/os/Process";
    JniHook::HookJniFun(env, className, "sendSignal", "(II)V",
                        (void *) new_sendSignal,
                        (void **) (&orig_sendSignal), true);

    JniHook::HookJniFun(env, className, "sendSignalQuiet", "(II)V",
                        (void *) new_sendSignalQuiet,
                        (void **) (&orig_sendSignalQuiet), true);

    JniHook::HookJniFun(env, className, "killProcessGroup", "(II)I",
                        (void *) new_killProcessGroup,
                        (void **) (&orig_killProcessGroup), true);
}
