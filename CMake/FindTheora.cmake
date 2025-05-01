# find_package uses this "FindTheora.cmake" in CMAKE_MODULE_PATH of the ags project
# It uses PkgConfig on Linux and it should probably be redone to be simpler

if (TARGET Theora::Theora)
    return()
endif()

if (WIN32)
    add_library(Theora::Theora STATIC IMPORTED)
    set_property(TARGET Theora::Theora PROPERTY IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/Solutions/.lib/libtheora_static.lib)
    set_property(TARGET Theora::Theora PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/Windows/include)
endif()

if (LINUX)
    pkg_check_modules(THEORA REQUIRED IMPORTED_TARGET GLOBAL theora)
    add_library(Theora::Theora ALIAS PkgConfig::THEORA)
endif()

if (MACOS)
    add_library(Theora::Theora STATIC IMPORTED)
    set_property(TARGET Theora::Theora PROPERTY IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/OSX/lib/libtheoradec.a)
    set_property(TARGET Theora::Theora PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/OSX/include)
    set_property(TARGET Theora::Theora PROPERTY INTERFACE_LINK_LIBRARIES Ogg::Ogg)
endif()
