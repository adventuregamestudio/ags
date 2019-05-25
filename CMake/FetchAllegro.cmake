FetchContent_Declare(
  allegro_content
  GIT_REPOSITORY https://github.com/adventuregamestudio/lib-allegro.git
  GIT_TAG        allegro-4.4.3.1-agspatch
  GIT_SHALLOW    yes
)

FetchContent_GetProperties(allegro_content)
if(NOT allegro_content_POPULATED)
  FetchContent_Populate(allegro_content)
  file(COPY Common/libsrc/allegro4/CMakeLists.txt DESTINATION ${allegro_content_SOURCE_DIR})
  add_subdirectory(${allegro_content_SOURCE_DIR} ${allegro_content_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
