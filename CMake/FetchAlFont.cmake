FetchContent_Declare(
  alfont_content
  GIT_REPOSITORY https://github.com/adventuregamestudio/lib-alfont.git
  GIT_TAG        alfont-1.9.1-agspatch
  GIT_SHALLOW    yes
)

FetchContent_GetProperties(alfont_content)
if(NOT alfont_content_POPULATED)
  FetchContent_Populate(alfont_content)
  file(COPY Engine/libsrc/alfont-2.0.9/CMakeLists.txt DESTINATION ${alfont_content_SOURCE_DIR})
  add_subdirectory(${alfont_content_SOURCE_DIR} ${alfont_content_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
