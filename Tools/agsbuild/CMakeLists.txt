add_executable(agsbuild main.cpp agsbuild_stubs.cpp)

set_target_properties(agsbuild PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS NO
)

target_include_directories(agsbuild PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../libgamebuild
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Common
)

target_link_libraries(agsbuild PUBLIC libgamebuild)

if(AGS_DESKTOP)
    install(TARGETS agsbuild RUNTIME DESTINATION bin)
endif()
