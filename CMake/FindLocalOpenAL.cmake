# Grab system openAL or use embedded mojoAL

find_package(OpenAL)

if (OPENAL_FOUND)
    add_library(openal-interface INTERFACE)
    target_link_libraries(openal-interface INTERFACE ${OPENAL_LIBRARY})
    target_include_directories(openal-interface INTERFACE ${OPENAL_INCLUDE_DIR})
    add_library(External::OpenAL ALIAS openal-interface)
else()
    add_subdirectory(libsrc/mojoAL EXCLUDE_FROM_ALL)

    add_library(openal-interface INTERFACE)
    target_link_libraries(openal-interface INTERFACE MojoAL::MojoAL)
    add_library(External::OpenAL ALIAS openal-interface)
endif()
