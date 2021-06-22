
include_directories(.)

if(KERNELMODE)
   add_definitions(-DKERNELMODE)
endif()

set(xnucxx.SOURCE_FILE_LIST
  ${CMAKE_CURRENT_SOURCE_DIR}/LiteMemOpt.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/LiteObject.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/LiteIterator.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/LiteCollection.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/LiteMutableBuffer.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/LiteMutableArray.cc
)

add_library(xnucxx STATIC
  ${xnucxx.SOURCE_FILE_LIST}
)