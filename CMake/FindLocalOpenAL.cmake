# Grab system openAL or use embedded mojoAL

if(WIN32 OR MACOS OR FREEBSD)
    set(AGS_USE_MOJO_AL TRUE)
endif()

if(NOT AGS_USE_MOJO_AL)
    find_package(OpenAL)
endif()

if(AGS_USE_MOJO_AL OR NOT OPENAL_FOUND)
    add_subdirectory(libsrc/mojoAL EXCLUDE_FROM_ALL)

    add_library(openal-interface INTERFACE)
    target_link_libraries(openal-interface INTERFACE MojoAL::MojoAL)
    add_library(External::OpenAL ALIAS openal-interface)
    message("using MojoAL...")
else()
    add_library(openal-interface INTERFACE)
    target_link_libraries(openal-interface INTERFACE ${OPENAL_LIBRARY})
    target_include_directories(openal-interface INTERFACE ${OPENAL_INCLUDE_DIR})
    add_library(External::OpenAL ALIAS openal-interface)
    message("using system OpenAL...")
endif()