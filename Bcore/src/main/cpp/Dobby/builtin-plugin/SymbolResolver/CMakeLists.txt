set(SOURCE_FILE_LIST )

if(NOT DEFINED DOBBY_DIR)
  message(FATAL_ERROR "DOBBY_DIR must be set!")
endif()

if(SYSTEM.Darwin)
  set(SOURCE_FILE_LIST ${SOURCE_FILE_LIST}
    ${CMAKE_CURRENT_SOURCE_DIR}/macho/dyld_shared_cache_symbol_table_iterator.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/macho/dobby_symbol_resolver.cc

    ${DOBBY_DIR}/source/UserMode/PlatformUtil/Darwin/ProcessRuntimeUtility.cc
    )
endif()
if(SYSTEM.Linux OR SYSTEM.Android)
  set(SOURCE_FILE_LIST ${SOURCE_FILE_LIST}
    ${CMAKE_CURRENT_SOURCE_DIR}/elf/dobby_symbol_resolver.cc

    ${DOBBY_DIR}/source/UserMode/PlatformUtil/Linux/ProcessRuntimeUtility.cc
    )
endif()
if(SYSTEM.Windows)
  set(SOURCE_FILE_LIST ${SOURCE_FILE_LIST}
  ${DOBBY_DIR}/source/UserMode/PlatformUtil/Windows/ProcessRuntimeUtility.cc
  )
endif()

add_library(symbol_resolver STATIC
  ${SOURCE_FILE_LIST}
  )

include_directories(
  .
)

