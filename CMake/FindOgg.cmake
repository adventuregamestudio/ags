# find_package uses this "FindOgg.cmake" in CMAKE_MODULE_PATH of the ags project
# It uses PkgConfig on Linux and it should probably be redone to be simpler

if (TARGET Ogg::Ogg)
    return()
endif()

if (WIN32)
    add_library(Ogg::Ogg STATIC IMPORTED)
    set_property(TARGET Ogg::Ogg PROPERTY IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/Solutions/.lib/libogg_static.lib)
    set_property(TARGET Ogg::Ogg PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/Windows/include)
endif()

if (LINUX)
    pkg_check_modules(OGG REQUIRED IMPORTED_TARGET GLOBAL ogg)
    add_library(Ogg::Ogg ALIAS PkgConfig::OGG)
endif()

if (MACOS)
    add_library(Ogg::Ogg STATIC IMPORTED)
    set_property(TARGET Ogg::Ogg PROPERTY IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/OSX/lib/libogg.a)
    set_property(TARGET Ogg::Ogg PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/OSX/include)
endif()
