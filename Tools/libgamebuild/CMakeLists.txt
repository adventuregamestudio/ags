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
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Compiler
)

target_sources(libgamebuild PRIVATE
    build_system.cpp
    build_system.h
    compiler_bridge.cpp
    compiler_bridge.h
    default_fonts.h
    game_data.h
    game_data_writer.cpp
    game_data_writer.h
    io_utils.h
    logger.h
    path_utils.h
    project.cpp
    project.h
    sprite_loader.cpp
    sprite_loader.h
)

target_link_libraries(libgamebuild PUBLIC
    libtools
    AGS::Common
    AGS::Compiler
)

target_compile_definitions(libgamebuild PRIVATE
    SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

if(WIN32)
    target_link_libraries(libgamebuild PUBLIC shlwapi)
endif()

add_library(AGS::GameBuild ALIAS libgamebuild)
