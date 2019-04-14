FetchContent_Declare(
  ogg_content
  GIT_REPOSITORY https://github.com/xiph/ogg.git
  GIT_TAG        f7dadaaf75634289f7ead64ed1802b627d761ee3
  GIT_SHALLOW    yes
)

FetchContent_GetProperties(ogg_content)
if(NOT ogg_content_POPULATED)
  FetchContent_Populate(ogg_content)
  file(COPY Engine/libsrc/ogg/CMakeLists.txt DESTINATION ${ogg_content_SOURCE_DIR})
  add_subdirectory(${ogg_content_SOURCE_DIR} ${ogg_content_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
