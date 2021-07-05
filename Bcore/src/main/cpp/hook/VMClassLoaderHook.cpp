//
// Created by Milk on 2021/5/5.
//

//
// Created by Milk on 5/5/21.
//

#include <cstring>
#include "VMClassLoaderHook.h"
#include <JniHook.h>

static bool hideXposedClass = false;

HOOK_JNI(jobject, findLoadedClass, JNIEnv *env, jobject obj, jobject class_loader, jstring name) {
    const char * nameC = env->GetStringUTFChars(name, JNI_FALSE);
//     ALOGD("findLoadedClass: %s", nameC);
    if (hideXposedClass) {
        if (strstr(nameC, "de/robv/android/xposed/") ||
            strstr(nameC, "me/weishu/epic") ||
            strstr(nameC, "me/weishu/exposed") ||
            strstr(nameC, "de.robv.android") ||
            strstr(nameC, "me.weishu.epic") ||
            strstr(nameC, "me.weishu.exposed")) {
            return nullptr;
        }
    }
    jobject result = orig_findLoadedClass(env, obj, class_loader, name);
    env->ReleaseStringUTFChars(name, nameC);
    return result;
}

void VMClassLoaderHook::init(JNIEnv *env) {
    const char *className = "java/lang/VMClassLoader";
    JniHook::HookJniFun(env, className, "findLoadedClass", "(Ljava/lang/ClassLoader;Ljava/lang/String;)Ljava/lang/Class;",
                        (void *) new_findLoadedClass,
                        (void **) (&orig_findLoadedClass), true);
}

void VMClassLoaderHook::hideXposed() {
    hideXposedClass = true;
}
