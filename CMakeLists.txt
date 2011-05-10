project(ags)
cmake_minimum_required(VERSION 2.8)
set(COMMON "${CMAKE_CURRENT_SOURCE_DIR}/Common")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Engine)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Common)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Common/libinclude)

# We don't support the Editor for now
add_definitions(-DTHIS_IS_THE_ENGINE)
add_subdirectory(Engine)
