if(SYSTEM.Darwin)
set(SOURCE_FILE_LIST
  ${CMAKE_CURRENT_SOURCE_DIR}/global_offset_table_hook.cc
  )
endif()

add_library(global_offset_table_hook STATIC
  ${SOURCE_FILE_LIST}
  )

include_directories(
  .
)
