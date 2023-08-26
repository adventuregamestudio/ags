FetchContent_Declare(
  zlibng_content
  URL https://github.com/zlib-ng/zlib-ng/archive/refs/tags/2.1.3.tar.gz
  URL_HASH MD5=f2ddef7b10e24cdcc4f17285575a6895
)

FetchContent_GetProperties(zlibng_content)
if(NOT zlibng_content_POPULATED)
  FetchContent_Populate(zlibng_content)

  set(ZLIB_ENABLE_TESTS  OFF CACHE BOOL "")
  set(ZLIBNG_ENABLE_TESTS  OFF CACHE BOOL "")
  set(WITH_GTEST  OFF CACHE BOOL "")
  set(ZLIB_COMPAT ON CACHE BOOL "")
  add_subdirectory(${zlibng_content_SOURCE_DIR} ${zlibng_content_BINARY_DIR} EXCLUDE_FROM_ALL)
   
  add_library(zlib-interface INTERFACE)
  target_link_libraries(zlib-interface INTERFACE zlibstatic)
  add_library(ZLIB::ZLIB ALIAS zlib-interface)

endif()
set(ZLIB_LIBRARIES ZLIB::ZLIB)
