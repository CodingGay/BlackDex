include_directories(.)

set(SOURCE_FILE_LIST
  # memory cache database
  ${CMAKE_CURRENT_SOURCE_DIR}/variable_cache.c

  # async logger
  ${CMAKE_CURRENT_SOURCE_DIR}/async_logger.cc
)

add_library(misc_helper STATIC
  ${SOURCE_FILE_LIST}
  ${SOURCE_HEADER_LIST}
)