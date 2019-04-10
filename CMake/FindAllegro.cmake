if (TARGET Allegro::Allegro)
    return()
endif()

if (WIN32)
    add_library(allegro STATIC IMPORTED GLOBAL)
    set_property(TARGET allegro PROPERTY IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/Solutions/.lib/alleg-static.lib)
    target_compile_definitions(allegro INTERFACE ALLEGRO_STATICLINK)
    target_include_directories(allegro INTERFACE {PROJECT_SOURCE_DIR}/Windows/include)

    target_link_libraries(allegro INTERFACE
		winmm
		DirectX::DirectDraw
        DirectX::DirectInput
        DirectX::DirectSound
        DirectX::DirectXGUID
        DirectX::Direct3D9
        DirectX::Direct3DX9
    )
endif()

if (LINUX)
    add_library(allegro INTERFACE IMPORTED GLOBAL)

    pkg_check_modules(ALLEGRO REQUIRED allegro)
    if(ALLEGRO_INCLUDE_DIRS)
        set_property(TARGET allegro PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${ALLEGRO_INCLUDE_DIRS}")
    endif()
    if(ALLEGRO_LINK_LIBRARIES)
        set_property(TARGET allegro PROPERTY INTERFACE_LINK_LIBRARIES "${ALLEGRO_LINK_LIBRARIES}")
    endif()
    if(ALLEGRO_LDFLAGS_OTHER)
        set_property(TARGET allegro PROPERTY INTERFACE_LINK_OPTIONS "${ALLEGRO_LDFLAGS_OTHER}")
    endif()
    if(ALLEGRO_CFLAGS_OTHER)
        set_property(TARGET allegro PROPERTY INTERFACE_COMPILE_OPTIONS "${ALLEGRO_CFLAGS_OTHER}")
    endif()
endif ()

if (MACOS)
    add_library(allegro STATIC IMPORTED GLOBAL)
    set_property(TARGET allegro PROPERTY IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/OSX/lib/liballeg.a)
    target_compile_definitions(allegro INTERFACE ALLEGRO_STATICLINK)
    target_include_directories(allegro INTERFACE ${PROJECT_SOURCE_DIR}/OSX/include)

    find_library(AUDIO_TOOLBOX_FRAMEWORK AudioToolbox)
    mark_as_advanced(AUDIO_TOOLBOX_FRAMEWORK)
    target_link_libraries(allegro INTERFACE ${AUDIO_TOOLBOX_FRAMEWORK})

    find_library(COCOA_FRAMEWORK Cocoa)
    mark_as_advanced(COCOA_FRAMEWORK)
    target_link_libraries(allegro INTERFACE ${COCOA_FRAMEWORK})

    find_library(CORE_AUDIO_FRAMEWORK CoreAudio)
    mark_as_advanced(CORE_AUDIO_FRAMEWORK)
    target_link_libraries(allegro INTERFACE ${CORE_AUDIO_FRAMEWORK})

    find_library(CORE_VIDEO_FRAMEWORK CoreVideo)
    mark_as_advanced(CORE_VIDEO_FRAMEWORK)
    target_link_libraries(allegro INTERFACE ${CORE_VIDEO_FRAMEWORK})

    find_library(IO_KIT_FRAMEWORK IOKit)
    mark_as_advanced(IO_KIT_FRAMEWORK)
    target_link_libraries(allegro INTERFACE ${IO_KIT_FRAMEWORK})

    find_library(OPEN_GL_FRAMEWORK OpenGL)
    mark_as_advanced(OPEN_GL_FRAMEWORK)
    target_link_libraries(allegro INTERFACE ${OPEN_GL_FRAMEWORK})
endif()


target_compile_definitions (allegro
    INTERFACE 
        ALLEGRO_NO_COMPATIBILITY 
        ALLEGRO_NO_FIX_ALIASES 
        ALLEGRO_NO_FIX_CLASS
)

add_library(Allegro::Allegro ALIAS allegro)
