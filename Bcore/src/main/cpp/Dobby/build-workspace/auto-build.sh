#!/bin/sh

# if error, exit
set -

CURRENT_DIR=$(dirname "$0")
SOURCE_DIR=${CURRENT_DIR}/..

compress_dir_array=""

summary_output_dir_name=auto-build-output

rm -rf ${summary_output_dir_name}

# Darwin ================================================================

darwin_library_name=libdobby.a
darwin_fat_library_name=libdobby.a

# build macos x86_64
output_dir_name=auto-build-workspace/darwin-x86_64-build
echo "prepare build ${output_dir_name}"

mkdir -p ${CURRENT_DIR}/${output_dir_name}
cmake -S ${SOURCE_DIR} -B ${output_dir_name} -DCMAKE_BUILD_TYPE=Release \
  -DDOBBY_GENERATE_SHARED=OFF -DDOBBY_DEBUG=OFF
cmake --build ${output_dir_name} --parallel 4 --target dobby

mkdir -p ${summary_output_dir_name}/darwin/x86_64
cp -r ${output_dir_name}/${darwin_library_name} ${summary_output_dir_name}/darwin/x86_64

# build iphone arm64
output_dir_name=auto-build-workspace/darwin-arm64-build
compress_dir_array="$compress_dir_array $output_dir_name"
echo "prepare build ${output_dir_name}"

mkdir -p ${CURRENT_DIR}/${output_dir_name}
cmake -S ${SOURCE_DIR} -B ${output_dir_name} -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_SYSTEM_PROCESSOR=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=9.3 \
  -DDOBBY_GENERATE_SHARED=OFF -DDOBBY_DEBUG=OFF
cmake --build ${output_dir_name} --parallel 4 --target dobby

mkdir -p ${summary_output_dir_name}/darwin/arm64
cp -r ${output_dir_name}/${darwin_library_name} ${summary_output_dir_name}/darwin/arm64

# build iphone arm64e
output_dir_name=auto-build-workspace/darwin-arm64e-build
compress_dir_array="$compress_dir_array $output_dir_name"
echo "prepare build ${output_dir_name}"

mkdir -p ${CURRENT_DIR}/${output_dir_name}
cmake -S ${SOURCE_DIR} -B ${output_dir_name} -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=cmake/ios.toolchain.cmake \
  -DPLATFORM=OS64 -DARCHS="arm64e" -DCMAKE_SYSTEM_PROCESSOR=arm64e \
  -DENABLE_BITCODE=0 -DENABLE_ARC=0 -DENABLE_VISIBILITY=1 -DDEPLOYMENT_TARGET=9.3 \
  -DDOBBY_GENERATE_SHARED=OFF -DDOBBY_DEBUG=OFF
cmake --build ${output_dir_name} --parallel 4 --target dobby

mkdir -p ${summary_output_dir_name}/darwin/arm64e
cp -r ${output_dir_name}/${darwin_library_name} ${summary_output_dir_name}/darwin/arm64e

# build darwin universal
output_dir_name=auto-build-workspace/darwin-universal-build
echo "prepare build ${output_dir_name}"

mkdir -p ${CURRENT_DIR}/${output_dir_name}
cp -r ${summary_output_dir_name}/darwin/arm64/${darwin_library_name} ${output_dir_name}

# create universal fat lib
lipo -create \
  ${summary_output_dir_name}/darwin/arm64/${darwin_fat_library_name} \
  ${summary_output_dir_name}/darwin/arm64e/${darwin_fat_library_name} \
  ${summary_output_dir_name}/darwin/x86_64/${darwin_fat_library_name} \
  -output ${output_dir_name}/${darwin_fat_library_name}

mkdir -p ${summary_output_dir_name}/darwin/universal
cp -r ${output_dir_name}/${darwin_library_name} ${summary_output_dir_name}/darwin/universal

# Android ================================================================

android_library_name=libdobby.a

# build android aarch64
output_dir_name=auto-build-workspace/android-arm64-build
compress_dir_array="$compress_dir_array $output_dir_name"
echo "prepare build ${output_dir_name}"

cmake -S ${SOURCE_DIR} -B ${output_dir_name} -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_SYSTEM_NAME=Android -DCMAKE_ANDROID_ARCH_ABI="arm64-v8a" -DCMAKE_ANDROID_NDK=$ANDROID_NDK_DIR -DCMAKE_SYSTEM_VERSION=21 -DCMAKE_ANDROID_NDK_TOOLCHAIN_VERSION=clang \
  -DDOBBY_GENERATE_SHARED=OFF -DDOBBY_DEBUG=OFF -DPlugin.Android.BionicLinkerRestriction=ON
cmake --build ${output_dir_name} --parallel 4 --target dobby

mkdir -p ${summary_output_dir_name}/android/arm64
mv ${output_dir_name}/${android_library_name} ${summary_output_dir_name}/android/arm64/${android_library_name}
mv ${output_dir_name}/${android_library_name} "prefab/modules/dobby/libs/android.arm64-v8a/${android_library_name}"

# build android armv7
output_dir_name=auto-build-workspace/android-armv7-build
compress_dir_array="$compress_dir_array $output_dir_name"
echo "prepare build ${output_dir_name}"

cmake -S ${SOURCE_DIR} -B ${output_dir_name} -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_SYSTEM_NAME=Android -DCMAKE_ANDROID_ARCH_ABI="armeabi-v7a" -DCMAKE_ANDROID_NDK=$ANDROID_NDK_DIR -DCMAKE_SYSTEM_VERSION=16 -DCMAKE_ANDROID_NDK_TOOLCHAIN_VERSION=clang \
  -DDOBBY_GENERATE_SHARED=OFF -DDOBBY_DEBUG=OFF -DPlugin.Android.BionicLinkerRestriction=ON
cmake --build ${output_dir_name} --parallel 4 --target dobby

mkdir -p ${summary_output_dir_name}/android/armv7
mv ${output_dir_name}/${android_library_name} ${summary_output_dir_name}/android/armv7/${android_library_name}
mv ${output_dir_name}/${android_library_name} "prefab/modules/dobby/libs/android.armeabi-v7a/${android_library_name}"

# build android x86
output_dir_name=auto-build-workspace/android-x86-build
compress_dir_array="$compress_dir_array $output_dir_name"
echo "prepare build ${output_dir_name}"

cmake -S ${SOURCE_DIR} -B ${output_dir_name} -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_SYSTEM_NAME=Android -DCMAKE_ANDROID_ARCH_ABI="x86" -DCMAKE_ANDROID_NDK=$ANDROID_NDK_DIR -DCMAKE_ANDROID_NDK_TOOLCHAIN_VERSION=clang \
  -DDOBBY_GENERATE_SHARED=OFF -DDOBBY_DEBUG=OFF -DPlugin.Android.BionicLinkerRestriction=ON
cmake --build ${output_dir_name} --parallel 4 --target dobby

mkdir -p ${summary_output_dir_name}/android/x86
mv ${output_dir_name}/${android_library_name} ${summary_output_dir_name}/android/x86/${android_library_name}
mv ${output_dir_name}/${android_library_name} "prefab/modules/dobby/libs/android.x86/${android_library_name}"

# build android x86_64
output_dir_name=auto-build-workspace/android-x86_64-build
compress_dir_array="$compress_dir_array $output_dir_name"
echo "prepare build ${output_dir_name}"

cmake -S ${SOURCE_DIR} -B ${output_dir_name} -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_SYSTEM_NAME=Android -DCMAKE_ANDROID_ARCH_ABI="x86_64" -DCMAKE_ANDROID_NDK=$ANDROID_NDK_DIR -DCMAKE_ANDROID_NDK_TOOLCHAIN_VERSION=clang \
  -DDOBBY_GENERATE_SHARED=OFF -DDOBBY_DEBUG=OFF -DPlugin.Android.BionicLinkerRestriction=ON
cmake --build ${output_dir_name} --parallel 4 --target dobby

mkdir -p ${summary_output_dir_name}/android/x86_64
mv ${output_dir_name}/${android_library_name} ${summary_output_dir_name}/android/x86_64/${android_library_name}
#mv ${output_dir_name}/${android_library_name} "prefab/modules/dobby/libs/android.x86_64/${android_library_name}"

## zip android prefab
#mkdir -p prefab/modules/dobby/include
#cp "include/dobby.h" "prefab/modules/dobby/include/"
#cp "builtin-plugin/BionicLinkerRestriction/bionic_linker_restriction.h" "prefab/modules/dobby/include/"
#cp "builtin-plugin/SymbolResolver/dobby_symbol_resolver.h" "prefab/modules/dobby/include/"
#cp "prefab/AndroidManifest.xml" .
#zip -r ${summary_output_dir_name}/android_prefab.aar prefab AndroidManifest.xml -x prefab/AndroidManifest.xml

if [ $DOBBY_BUILD_OUTPUT_NAME ]; then
  tar czvf ${DOBBY_BUILD_OUTPUT_NAME} ${summary_output_dir_name}
fi
