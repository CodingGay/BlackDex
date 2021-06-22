# Build 

## Cmake build options

```
option(DOBBY_GENERATE_SHARED "Build shared library" ON)

option(DOBBY_DEBUG "Enable debug logging" ON)

option(NearBranch "Enable Near Branch Trampoline" ON)

option(DynamicBinaryInstrument "Enable Dynamic Binary Instrument" ON)

option(FullFloatingPointRegisterPack "Save and pack all floating-point registers" OFF)

option(Plugin.SymbolResolver "Resolve symbol by [DobbySymbolResolver] " ON)

option(Plugin.GlobalOffsetTableHook "Global Offset Table Hook by [DobbyGlobalOffsetTableReplace] " ON)

option(Plugin.LinkerLoadCallback "Register image load callback " OFF)
```

## Build script

refer: [build-workspace/auto-build.sh](build-workspace/auto-build.sh)

## Build for host

```
cd Dobby && mkdir build_for_host && cd build_for_host

cmake ..

make -j4
```

## Build for iOS / macOS

#### Manual build for macOS X64 host

```
cd Dobby && mkdir build_for_macos_x64 && cd build_for_macos_x64

cmake .. \
-DCMAKE_BUILD_TYPE=Release \

make -j4
```

#### Manual build for iOS [ARM/ARM64]

```
cd Dobby && mkdir build_for_ios_arm64 && cd build_for_ios_arm64

cmake .. -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_SYSTEM_PROCESSOR=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=9.3

make -j4
```

if you want generate Xcode Project, just replace with `cmake -G Xcode`.

## Build for Android

#### Manual build for Android ARM64

```
export ANDROID_NDK=/Users/jmpews/Library/Android/sdk/ndk-bundle

cd Dobby && mkdir build_for_android_arm64 && cd build_for_android_arm64

cmake .. \
-DCMAKE_BUILD_TYPE=Release \
-DCMAKE_SYSTEM_NAME=Android -DCMAKE_ANDROID_ARCH_ABI="arm64-v8a" -DCMAKE_ANDROID_NDK=$ANDROID_NDK -DCMAKE_SYSTEM_VERSION=21 -DCMAKE_ANDROID_NDK_TOOLCHAIN_VERSION=clang

make -j4
```

#### Manual build for Android ARM

```
export ANDROID_NDK=/Users/jmpews/Library/Android/sdk/ndk-bundle

cd Dobby && mkdir build_for_android_arm && cd build_for_android_arm

-DCMAKE_BUILD_TYPE=Release \
-DCMAKE_SYSTEM_NAME=Android -DCMAKE_ANDROID_ARCH_ABI="armeabi-v7a" -DCMAKE_ANDROID_NDK=$ANDROID_NDK -DCMAKE_SYSTEM_VERSION=16 -DCMAKE_ANDROID_NDK_TOOLCHAIN_VERSION=clang 

make -j4
```

#### Android Studio CMake

```
if(NOT TARGET dobby)
set(DOBBY_DIR /Users/jmpews/Workspace/Project.wrk/Dobby)
macro(SET_OPTION option value)
  set(${option} ${value} CACHE INTERNAL "" FORCE)
endmacro()
SET_OPTION(DOBBY_DEBUG OFF)
SET_OPTION(DOBBY_GENERATE_SHARED OFF)
add_subdirectory(${DOBBY_DIR} dobby)
get_property(DOBBY_INCLUDE_DIRECTORIES
  TARGET dobby
  PROPERTY INCLUDE_DIRECTORIES)
include_directories(
  .
  ${DOBBY_INCLUDE_DIRECTORIES}
  $<TARGET_PROPERTY:dobby,INCLUDE_DIRECTORIES>
)
endif()

add_library(native-lib SHARED
  ${DOBBY_DIR}/example/android_common_api.cc

  native-lib.cpp)
```