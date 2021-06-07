# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.
include_directories( dex )
include_directories( base )
include_directories( ./ )

aux_source_directory(./ SRC7)
aux_source_directory(xhook SRC6)
aux_source_directory(ziparchive SRC1)
aux_source_directory(dex SRC2)
aux_source_directory(android-base SRC3)
aux_source_directory(utils SRC4)
aux_source_directory(hook SRC5)
aux_source_directory(jnihook SRC6)
add_compile_options(-w)
add_library( # Sets the name of the library.
             blackdex

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             VmCore.cpp
        ${SRC3}
        ${SRC1}
        ${SRC2}
        ${SRC4}
        ${SRC5}
        ${SRC6}
        ${SRC7}
        )

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
                        blackdex

                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib}
                       z
        )
