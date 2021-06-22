include_directories(.)

set(SOURCE_FILE_LIST
  ${CMAKE_CURRENT_SOURCE_DIR}/cxxlogging.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/logging.c
)

add_library(logging STATIC
  ${SOURCE_FILE_LIST}
  ${SOURCE_HEADER_LIST}
)