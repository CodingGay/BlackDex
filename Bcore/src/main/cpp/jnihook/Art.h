//
// Created by Milk on 3/7/21.
//
#include <stdio.h>

#ifndef ARTHOOK_ART_METHOD_H
#define ARTHOOK_ART_METHOD_H

#define __ANDROID_API_R__ 30
#define __ANDROID_API_Q__ 29
#define __ANDROID_API_P__ 28

static constexpr uint32_t kAccPublic =       0x0001;  // class, field, method, ic
static constexpr uint32_t kAccPrivate =      0x0002;  // field, method, ic
static constexpr uint32_t kAccProtected =    0x0004;  // field, method, ic
static constexpr uint32_t kAccStatic =       0x0008;  // field, method, ic
static constexpr uint32_t kAccFinal =        0x0010;  // class, field, method, ic
static constexpr uint32_t kAccSynchronized = 0x0020;  // method (only allowed on natives)
static constexpr uint32_t kAccSuper =        0x0020;  // class (not used in dex)
static constexpr uint32_t kAccVolatile =     0x0040;  // field
static constexpr uint32_t kAccBridge =       0x0040;  // method (1.5)
static constexpr uint32_t kAccTransient =    0x0080;  // field
static constexpr uint32_t kAccVarargs =      0x0080;  // method (1.5)
static constexpr uint32_t kAccNative =       0x0100;  // method
static constexpr uint32_t kAccInterface =    0x0200;  // class, ic
static constexpr uint32_t kAccAbstract =     0x0400;  // class, method, ic
static constexpr uint32_t kAccStrict =       0x0800;  // method
static constexpr uint32_t kAccSynthetic =    0x1000;  // class, field, method, ic
static constexpr uint32_t kAccAnnotation =   0x2000;  // class, ic (1.5)
static constexpr uint32_t kAccEnum =         0x4000;  // class, field, ic (1.5)

static constexpr uint32_t kAccPublicApi =             0x10000000;  // field, method
static constexpr uint32_t kAccCorePlatformApi =       0x20000000;  // field, method

// Native method flags are set when linking the methods based on the presence of the
// @dalvik.annotation.optimization.{Fast,Critical}Native annotations with build visibility.
// Reuse the values of kAccSkipAccessChecks and kAccMiranda which are not used for native methods.
static constexpr uint32_t kAccFastNative =            0x00080000;  // method (runtime; native only)
static constexpr uint32_t kAccCriticalNative =        0x00200000;  // method (runtime; native only)
#endif //ARTHOOK_ART_METHOD_H
