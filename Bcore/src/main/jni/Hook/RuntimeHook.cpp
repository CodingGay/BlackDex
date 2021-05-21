//
// Created by Milk on 5/5/21.
//

#include "RuntimeHook.h"
#import "JniHook/JniHook.h"
#include "VmCore.h"

HOOK_JNI(jstring, nativeLoad, JNIEnv *env, jobject obj, jstring name, jobject class_loader) {
    const char *nameC = env->GetStringUTFChars(name, JNI_FALSE);
    ALOGD("nativeLoad: %s", nameC);
    jstring result = orig_nativeLoad(env, obj, name, class_loader);
    env->ReleaseStringUTFChars(name, nameC);
    return result;
}

HOOK_JNI(jstring, nativeLoad2, JNIEnv *env, jobject obj, jstring name, jobject class_loader,
         jobject caller) {
    const char *nameC = env->GetStringUTFChars(name, JNI_FALSE);
    ALOGD("nativeLoad: %s", nameC);
    jstring result = orig_nativeLoad2(env, obj, name, class_loader, caller);
    env->ReleaseStringUTFChars(name, nameC);
    return result;
}

void RuntimeHook::init(JNIEnv *env) {
    const char *className = "java/lang/Runtime";
    if (VmCore::getApiLevel() >= __ANDROID_API_Q__) {
        JniHook::HookJniFun(env, className, "nativeLoad",
                            "(Ljava/lang/String;Ljava/lang/ClassLoader;Ljava/lang/Class;)Ljava/lang/String;",
                            (void *) new_nativeLoad2,
                            (void **) (&orig_nativeLoad2), true);
    } else {
        JniHook::HookJniFun(env, className, "nativeLoad",
                            "(Ljava/lang/String;Ljava/lang/ClassLoader;)Ljava/lang/String;",
                            (void *) new_nativeLoad,
                            (void **) (&orig_nativeLoad), true);
    }
}
