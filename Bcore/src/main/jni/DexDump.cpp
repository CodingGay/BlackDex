//
// Created by Milk on 2021/5/16.
//

#include "DexDump.h"
#include "Utils/HexDump.h"
#include "Log.h"
#include "VmCore.h"
#include "Utils/PointerCheck.h"

#include <cstdio>
#include <fcntl.h>
#include <cstring>
#include <cstdlib>
#include "unistd.h"

using namespace std;
static int beginOffset = -2;
static int sizeOffset = -2;


void init(JNIEnv *env) {
    jlongArray emptyCookie = VmCore::loadEmptyDex(env);
    jsize arrSize = env->GetArrayLength(emptyCookie);
    if (env->ExceptionCheck() == JNI_TRUE) {
        return;
    }
    jlong* long_data = env->GetLongArrayElements(emptyCookie, nullptr);

    for (int i = 0; i < arrSize; ++i) {
        jlong cookie = long_data[i];
        if (cookie == 0) {
            continue;
        }
        auto dex = reinterpret_cast<char *>(cookie);
        for (int ii = 0; ii < 10; ++ii) {
            auto value = *(size_t *) (dex + ii * sizeof(size_t));
            if (value == 1872) {
                sizeOffset = ii;
                beginOffset = sizeOffset - 1;
                // auto dexBegin = *(size_t *) (dex + beginOffset * sizeof(size_t));
                // HexDump(reinterpret_cast<char *>(dexBegin), 10, 0);
                env->ReleaseLongArrayElements(emptyCookie, long_data, 0);
                return;
            }
        }
    }
    env->ReleaseLongArrayElements(emptyCookie, long_data, 0);
    beginOffset = -1;
    sizeOffset = -1;
}

void DexDump::dumpDex(JNIEnv *env, jlong cookie, jstring dir) {
    if (beginOffset == -2 || sizeOffset == -2) {
        init(env);
    }
    if (beginOffset == -1 || sizeOffset == -1) {
        ALOGD("dumpDex not support!");
        return;
    }
    char magic[8] = {0x64, 0x65, 0x78, 0x0a, 0x30, 0x33, 0x35, 0x00};
    auto base = reinterpret_cast<char *>(cookie);
    auto begin = *(size_t *) (base + beginOffset * sizeof(size_t));
    auto size = *(size_t *) (base + sizeOffset * sizeof(size_t));
    if (!PointerCheck::check(reinterpret_cast<void *>(begin))) {
        return;
    }

    auto dirC = env->GetStringUTFChars(dir, 0);

    auto buffer = malloc(size);
    memcpy(buffer, reinterpret_cast<const void *>(begin), size);
    // fix magic
    memcpy(buffer, magic, sizeof(magic));

    char path[1024];
    sprintf(path, "%s/dex_%ld.dex", dirC, size);
    auto fd = open(path, O_CREAT | O_WRONLY, 0600);
    ssize_t w = write(fd, buffer, size);
    fsync(fd);
    if (w > 0) {
        ALOGD("dump dex ======> %s", path);
    } else {
        remove(path);
    }
    close(fd);
    free(buffer);
    env->ReleaseStringUTFChars(dir, dirC);
}
