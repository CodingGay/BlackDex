# Getting Started

## create native project and update CMakeLists.txt

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

## replace hook function

ref source `builtin-plugin/ApplicationEventMonitor/dynamic_loader_monitor.cc`

ref source `builtin-plugin/ApplicationEventMonitor/posix_file_descriptor_operation_monitor.cc`

## instrument function

ref source `builtin-plugin/ApplicationEventMonitor/memory_operation_instrument.cc`

## Android Linker Restriction

ref source `builtin-plugin/AndroidRestriction/android_restriction_demo.cc`

```c
# impl at SymbolResolver/elf/dobby_symbol_resolver.cc
void *__loader_dlopen = DobbySymbolResolver(NULL, "__loader_dlopen");
DobbyHook((void *)__loader_dlopen, (void *)fake_loader_dlopen, (void **)&orig_loader_dlopen);
```

```
# impl at AndroidRestriction/android_restriction.cc
linker_disable_namespace_restriction();
void *handle = NULL;
handle       = dlopen(lib, RTLD_LAZY);
vm           = dlsym(handle, "_ZN7android14AndroidRuntime7mJavaVME");
```