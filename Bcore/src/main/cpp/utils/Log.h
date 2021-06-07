#include <android/log.h>

#define TAG "VmCore"

#if 1
#define log_print_error(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#define log_print_debug(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#else
#define log_print_error(...)
#endif

#define ALOGE(...) log_print_error(__VA_ARGS__)
#define ALOGD(...) log_print_debug(__VA_ARGS__)

#ifndef SPEED_LOG_H
#define SPEED_LOG_H 1

#endif
