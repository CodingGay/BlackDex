cmake_minimum_required(VERSION 3.5)
project(Dobby)
enable_language(ASM)

include(cmake/Util.cmake)
include(cmake/Globals.cmake)
include(cmake/Macros.cmake)
include(cmake/XcodeGenerator.cmake)
include(cmake/AutoFiles.cmake)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

AutoFiles("." "CmakeSource" "\\.(cc|cpp|c|h)$")

# :< You Shall Not Pass!
if(0)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror")
endif()

# ===== Handle Option =====
option(DOBBY_GENERATE_SHARED "Build shared library" ON)

option(DOBBY_DEBUG "Enable debug logging" ON)

option(NearBranch "Enable Near Branch Trampoline" ON)

option(DynamicBinaryInstrument "Enable Dynamic Binary Instrument" ON)

option(FullFloatingPointRegisterPack "Save and pack all floating-point registers" OFF)

option(EnableObfuscation "Enable llvm obfuscation" OFF)

option(Plugin.SymbolResolver "Resolve symbol by [DobbySymbolResolver] " ON)

option(Plugin.GlobalOffsetTableHook "Global Offset Table Hook by [DobbyGlobalOffsetTableReplace] " ON)

option(Plugin.LinkerLoadCallback "Register image load callback " OFF)

# frida is better choice
option(Plugin.ApplicationEventMonitor "Auto monitor linker, file, etc." OFF)

option(Plugin.Android.BionicLinkerRestriction "Enable android bionic linker restriction" OFF)

# Use native assembly bridge to replace the runtime codegen
# if(CLOSURE_BRIDGE_TEMPLATE)
#   SET(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS}")
#   enable_language(ASM)
#   add_definitions(-DENABLE_CLOSURE_BRIDGE_TEMPLATE)
# endif()

# Enable debug will log more information
#if (CMAKE_BUILD_TYPE STREQUAL "Debug")
#  set(DOBBY_DEBUG ON)
#endif()
if(DOBBY_DEBUG)
  add_definitions(-DDOBBY_DEBUG)
  add_definitions(-DLOGGING_DEBUG)
  message(STATUS "[Dobby] Enable debug logging")
endif()

# Enable full floating point register pack
# for arm64, allow access q8 - q31
if(FullFloatingPointRegisterPack)
  add_definitions(-DFULL_FLOATING_POINT_REGISTER_PACK)
  message(STATUS "[Dobby] Save and pack all floating-point registers")
endif()

if(SYSTEM.Darwin)
  # -lstdc++
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -stdlib=libc++")
  if (NOT DOBBY_DEBUG)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-x -Wl,-S")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-exported_symbol,_log_internal_impl -Wl,-exported_symbol,_log_set_level")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-exported_symbol,_CodePatch")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-exported_symbol,_DobbyBuildVersion -Wl,-exported_symbol,_DobbyHook -Wl,-exported_symbol,_DobbyInstrument -Wl,-exported_symbol,_DobbyDestroy")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-exported_symbol,_DobbyGlobalOffsetTableReplace")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-exported_symbol,_DobbySymbolResolver")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-exported_symbol,_intercept_routing_common_bridge_handler")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-exported_symbol,_dobby_enable_near_branch_trampoline -Wl,-exported_symbol,_dobby_disable_near_branch_trampoline")
  endif()
elseif(SYSTEM.Android)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fomit-frame-pointer")
  if(NOT DOBBY_DEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections -Wl,--exclude-libs,ALL")
  endif()
elseif(SYSTEM.Linux)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
elseif(SYSTEM.Windows)
  if(NOT DOBBY_DEBUG)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /export:log_internal_impl")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /export:DobbyHook /export:DobbyInstrument /export:DobbySymbolResolver")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /export:dobby_enable_near_branch_trampoline /export:dobby_disable_near_branch_trampoline")
  endif()
endif()

if(COMPILER.Clang)
  if(NOT DOBBY_DEBUG)
    # set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-rtti -fvisibility=hidden -fvisibility-inlines-hidden")
  endif()
  if(PROCESSOR.ARM)
    set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -arch armv7 -x assembler-with-cpp")
  elseif(PROCESSOR.AARCH64)
    set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -arch arm64 -x assembler-with-cpp")
  endif()
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_CXX_FLAGS}")

message(STATUS "CMAKE_C_COMPILER: ${CMAKE_C_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
message(STATUS "CMAKE_C_FLAGS: ${CMAKE_C_FLAGS}")
message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message(STATUS "CMAKE_SHARED_LINKER_FLAGS: ${CMAKE_SHARED_LINKER_FLAGS}")

# arch prefix
if(PROCESSOR.ARM)
  set(ARCH1 ARM)
  set(arch1 arm)
  set(core_arch arm)
elseif(PROCESSOR.AARCH64)
  set(ARCH1 ARM64)
  set(arch1 arm64)
  set(core_arch arm64)
elseif(PROCESSOR.X86)
  set(ARCH1 X86)
  set(arch1 x86)
  set(core_arch ia32)
elseif(PROCESSOR.X86_64)
  set(ARCH1 X64)
  set(arch1 x64)
  set(core_arch x64)
else()
endif()

# system prefix
if(SYSTEM.Darwin OR SYSTEM.iOS OR SYSTEM.macOS)
  set(platform1 posix)
  set(platform2 Darwin)
elseif(SYSTEM.Linux OR SYSTEM.Android)
  set(platform1 posix)
  set(platform2 Linux)
elseif(SYSTEM.Windows)
  set(platform1 windows)
  set(platform2 Windows)
else()
endif()

if(CMAKE_GENERATOR STREQUAL Xcode)
endif()
include(cmake/dobby.xcode.source.cmake)

include_directories(
  .

  ./include

  ./source
  ./source/include
  ./source/UserMode

  ./external
  ./external/logging
  ./external/xnucxx
  ./external/misc-helper

  builtin-plugin
)

set(DOBBY_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(dobby.SOURCE_FILE_LIST ${dobby.SOURCE_FILE_LIST}
  # cpu
  source/core/arch/CpuFeature.cc
  source/core/arch/CpuRegister.cc

  # assembler
  source/core/modules/assembler/assembler.cc
  source/core/modules/assembler/assembler-${core_arch}.cc

  # codegen
  source/core/modules/codegen/codegen-${core_arch}.cc

  # memory kit
  source/MemoryAllocator/CodeBuffer/CodeBufferBase.cc
  source/MemoryAllocator/CodeBuffer/code-buffer-${arch1}.cc
  source/MemoryAllocator/AssemblyCodeBuilder.cc
  source/MemoryAllocator/MemoryArena.cc

  # instruction relocation
  source/InstructionRelocation/${arch1}/${ARCH1}InstructionRelocation.cc

  # intercept routing
  source/InterceptRouting/InterceptRouting.cpp

  # intercept routing trampoline
  source/TrampolineBridge/Trampoline/${arch1}/trampoline-${arch1}.cc

  # intercept routing plugin (buildin)
  source/InterceptRouting/Routing/FunctionInlineReplace/function-inline-replace.cc
  source/InterceptRouting/Routing/FunctionInlineReplace/FunctionInlineReplaceExport.cc

  # plugin register
  source/InterceptRouting/RoutingPlugin/RoutingPlugin.cc

  # platform util
  source/UserMode/PlatformUtil/${platform2}/ProcessRuntimeUtility.cc

  # user mode - platform interface
  source/UserMode/UnifiedInterface/platform-${platform1}.cc

  # user mode - executable memory
  source/UserMode/ExecMemory/code-patch-tool-${platform1}.cc
  source/UserMode/ExecMemory/clear-cache-tool-all.c

  # main
  source/dobby.cpp
  source/Interceptor.cpp
  )

if (PROCESSOR.X86_64 OR PROCESSOR.X86)
  set(NearBranch ON)
  if (PROCESSOR.X86_64)
    add_definitions(-DDETOURS_X64)
  endif()
endif()

if(SYSTEM.Darwin)
  include_directories(
    source/UserMode/ExecMemory/substrated/include
  )
  set(dobby.SOURCE_FILE_LIST ${dobby.SOURCE_FILE_LIST}
    source/UserMode/ExecMemory/code-patch-tool-darwin.cc
  )
endif()

if(SYSTEM.iOS)
  add_definitions(-DCODE_PATCH_WITH_SUBSTRATED)
  set(dobby.SOURCE_FILE_LIST ${dobby.SOURCE_FILE_LIST}
    source/UserMode/ExecMemory/substrated/mach_interface_support/substrated_client.c
  )
endif()


if(FunctionWrapper OR DynamicBinaryInstrument)
  set(dobby.SOURCE_FILE_LIST ${dobby.SOURCE_FILE_LIST}
    # closure trampoline bridge
    source/TrampolineBridge/ClosureTrampolineBridge/common-bridge-handler.cc
    source/TrampolineBridge/ClosureTrampolineBridge/${arch1}/helper-${arch1}.cc
    source/TrampolineBridge/ClosureTrampolineBridge/${arch1}/closure-bridge-${arch1}.cc
    source/TrampolineBridge/ClosureTrampolineBridge/${arch1}/${ARCH1}AssemblyClosureTrampoline.cc

    # user mode - multi thread support
    # source/UserMode/MultiThreadSupport/ThreadSupport.cpp
    # source/UserMode/Thread/PlatformThread.cc
    # source/UserMode/Thread/platform-thread-${platform1}.cc
    )
endif()

if(FunctionWrapper)
  message(FATAL_ERROR "[!] FunctionWrapper plugin is not supported")
endif()

if(DynamicBinaryInstrument)
  message(STATUS "[Dobby] Enable dynamic binary instrument(hook instruction with register context)")
  set(dobby.SOURCE_FILE_LIST ${dobby.SOURCE_FILE_LIST}
    source/InterceptRouting/Routing/DynamicBinaryInstrument/dynamic-binary-instrument.cc
    source/InterceptRouting/Routing/DynamicBinaryInstrument/DynamicBinaryInstrumentExport.cc
    source/InterceptRouting/Routing/DynamicBinaryInstrument/intercept_routing_handler.cc
    )
endif()

if(NearBranch)
  message(STATUS "[Dobby] Enable near branch trampoline(trampoline within single instruction)")
  set(dobby.SOURCE_FILE_LIST ${dobby.SOURCE_FILE_LIST}
    source/InterceptRouting/RoutingPlugin/NearBranchTrampoline/NeaBranchTrampoline.cc
    source/MemoryAllocator/NearMemoryArena.cc)
endif()

add_subdirectory(external/misc-helper)
get_target_property(misc_helper.SOURCE_FILE_LIST misc_helper SOURCES)

# add logging library
add_subdirectory(external/logging)
get_target_property(logging.SOURCE_FILE_LIST logging SOURCES)

# add xnucxx library
add_subdirectory(external/xnucxx)
get_target_property(xnucxx.SOURCE_FILE_LIST xnucxx SOURCES)

if(Plugin.GlobalOffsetTableHook AND SYSTEM.Darwin)
  message(STATUS "[Dobby] Enable global offset table hook")

  include_directories(builtin-plugin/GlobalOffsetTableHook)
  add_subdirectory(builtin-plugin/GlobalOffsetTableHook)
  get_target_property(global_offset_table_hook.SOURCE_FILE_LIST global_offset_table_hook SOURCES)
  set(dobby.plugin.SOURCE_FILE_LIST ${dobby.plugin.SOURCE_FILE_LIST}
    ${global_offset_table_hook.SOURCE_FILE_LIST}
    )
endif()

if(Plugin.SymbolResolver)
  message(STATUS "[Dobby] Enable symbol resolver")

  include_directories(builtin-plugin/SymbolResolver)
  add_subdirectory(builtin-plugin/SymbolResolver)
  get_target_property(symbol_resolver.SOURCE_FILE_LIST symbol_resolver SOURCES)
  set(dobby.plugin.SOURCE_FILE_LIST ${dobby.plugin.SOURCE_FILE_LIST}
    ${symbol_resolver.SOURCE_FILE_LIST}
    )
endif()

if(Plugin.Android.BionicLinkerRestriction)
  if(NOT SYSTEM.Android)
    message(FATAL_ERROR "[!] Plugin.Android.BionicLinkerRestriction only works on Android.")
  endif()
  message(STATUS "[Dobby] Enable Plugin.Android.BionicLinkerRestriction")
  set(dobby.plugin.SOURCE_FILE_LIST ${dobby.plugin.SOURCE_FILE_LIST}
    builtin-plugin/BionicLinkerRestriction/bionic_linker_restriction.cc
  )
endif()

if(Plugin.HideSystemCall)
  set(dobby.plugin.SOURCE_FILE_LIST ${dobby.plugin.SOURCE_FILE_LIST}
    )
endif()

if(Plugin.LinkerLoadCallback)
  set(dobby.plugin.SOURCE_FILE_LIST ${dobby.plugin.SOURCE_FILE_LIST}
    builtin-plugin/LinkerImageLoadCallback/linker_load_callback.cc
    )
endif()

set(dobby.HEADER_FILE_LIST
  include/dobby.h
  )

# add build version
string(TIMESTAMP TODAY "%Y%m%d")
set(VERSION_REVISION "-${TODAY}")
if (EXISTS "${CMAKE_SOURCE_DIR}/.git")
  execute_process(
    COMMAND git rev-parse --short --verify HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE VERSION_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if(VERSION_COMMIT_HASH)
    set(VERSION_REVISION "${VERSION_REVISION}-${VERSION_COMMIT_HASH}")
  endif()
endif()
set(DOBBY_BUILD_VERSION "Dobby${VERSION_REVISION}")
add_definitions(-D__DOBBY_BUILD_VERSION__="${DOBBY_BUILD_VERSION}")
message(STATUS "[Dobby] ${DOBBY_BUILD_VERSION}")

if(DOBBY_GENERATE_SHARED)
  message(STATUS "[Dobby] Generate shared library")
  set(DOBBY_LIBRARY_TYPE SHARED)
else()
  message(STATUS "[Dobby] Generate static library")
  set(DOBBY_LIBRARY_TYPE STATIC)
endif()
add_library(blackdex_d ${DOBBY_LIBRARY_TYPE} ${dobby.HEADER_FILE_LIST} ${dobby.SOURCE_FILE_LIST} ${logging.SOURCE_FILE_LIST} ${xnucxx.SOURCE_FILE_LIST} ${dobby.plugin.SOURCE_FILE_LIST})

target_include_directories(blackdex_d PUBLIC include)

if(EnableObfuscation)
set(linker_flags "${linker_flags} -Wl,-mllvm -Wl,-obfuscator-conf=all")
endif()
set_target_properties(blackdex_d
  PROPERTIES LINK_FLAGS "${linker_flags}"
  )
if(SYSTEM.Darwin)
  # set(CMAKE_BUILD_WITH_INSTALL_NAME_DIR TRUE)
  set(CMAKE_INSTALL_NAME_DIR "@rpath")
  set(CMAKE_SHARED_LIBRARY_RUNTIME_C_FLAG "-Wl,-rpath,")
  add_library(DobbyX ${DOBBY_LIBRARY_TYPE} ${dobby.HEADER_FILE_LIST} ${dobby.SOURCE_FILE_LIST} ${logging.SOURCE_FILE_LIST} ${xnucxx.SOURCE_FILE_LIST} ${dobby.plugin.SOURCE_FILE_LIST})

  set_target_properties(DobbyX
    PROPERTIES LINK_FLAGS "${linker_flags}"
  )

  # set framework property
  set_target_properties(DobbyX PROPERTIES
    FRAMEWORK TRUE
    FRAMEWORK_VERSION A
    MACOSX_FRAMEWORK_IDENTIFIER "com.dobby.dobby"
    # MACOSX_FRAMEWORK_INFO_PLIST Info.plist
    VERSION 1.0.0 # current version
    SOVERSION 1.0.0 # compatibility version
    PUBLIC_HEADER include/dobby.h
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Development"
  )
  # set_target_properties(Dobby PROPERTIES
  #  LINK_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}"
  #  )

  # message(STATUS "[Dobby] Enable Gollum.framework(iOS: 11.0 <= version, version <= 12.2, version == 12.4 )")
  # add_custom_command(TARGET Dobby
  #   POST_BUILD
  #   COMMAND mkdir -p $<TARGET_FILE_DIR:${dobby_output_name}>/Frameworks
  #   COMMAND cp -R ${CMAKE_SOURCE_DIR}/buildin-plugin/Gollum_2019.12.31.framework $<TARGET_FILE_DIR:${dobby_output_name}>/Frameworks/Gollum.framework
  #   )
endif()

if(SYSTEM.Android)
  target_link_libraries(blackdex_d log)
endif()

if(SYSTEM.Linux)
  target_link_libraries(blackdex_d dl)
endif()

if(SYSTEM.Darwin)
  target_link_libraries(DobbyX
    "-framework Foundation")
endif()

if(SYSTEM.Darwin)
  add_subdirectory(builtin-plugin/Dyld2HideLibrary)

  add_subdirectory(builtin-plugin/ObjcRuntimeHook)
  if(PROCESSOR.AARCH64)
    add_subdirectory(builtin-plugin/SupervisorCallMonitor)
  endif()
endif()

add_subdirectory(example)
