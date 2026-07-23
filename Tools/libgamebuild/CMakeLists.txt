add_library(libgamebuild)

set_target_properties(libgamebuild PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS NO
    C_STANDARD 11
    C_EXTENSIONS NO
)

target_include_directories(libgamebuild PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../data
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Common
)

target_sources(libgamebuild PRIVATE
    build_system.cpp
    build_system.h
    script_api_versions.h
    game_data.h
    game_data_writer.cpp
    game_data_writer.h
    cli_utils.h
    project.cpp
    project.h
)

target_link_libraries(libgamebuild PUBLIC
    libtools
    AGS::Common
)

if(WIN32)
    target_link_libraries(libgamebuild PUBLIC shlwapi)
endif()

add_library(AGS::GameBuild ALIAS libgamebuild)
