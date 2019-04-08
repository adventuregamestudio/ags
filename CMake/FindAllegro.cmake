if (TARGET Allegro::Allegro)
    return()
endif()

if (WIN32)
    add_library(allegro STATIC IMPORTED GLOBAL)
    set_property(TARGET allegro PROPERTY IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/Solutions/.lib/alleg-static.lib)
    target_compile_definitions(allegro INTERFACE ALLEGRO_STATICLINK)
    set_property(TARGET allegro PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/Windows/include)

    set_property(TARGET allegro PROPERTY INTERFACE_LINK_LIBRARIES 
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
    pkg_check_modules(ALLEGRO REQUIRED allegro)

    add_library(allegro INTERFACE IMPORTED GLOBAL)

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
    find_library(FW_AUDIO_TOOLBOX AudioToolbox)
    find_library(FW_CORE_AUDIO CoreAudio)
    find_library(FW_COCOA Cocoa)
    find_library(FW_CORE_VIDEO CoreVideo)
    find_library(FW_IO_KIT IOKit)
    find_library(FW_OPEN_GL OpenGL)

    add_library(allegro STATIC IMPORTED GLOBAL)
    set_property(TARGET allegro PROPERTY IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/OSX/lib/liballeg.a)
    target_compile_definitions(allegro INTERFACE ALLEGRO_STATICLINK)
    set_property(TARGET allegro PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/OSX/include)
    set_property(TARGET allegro PROPERTY INTERFACE_LINK_LIBRARIES 
        ${FW_AUDIO_TOOLBOX}
        ${FW_CORE_AUDIO}
        ${FW_COCOA}
        ${FW_CORE_VIDEO}
        ${FW_IO_KIT}
        ${FW_OPEN_GL}
        )
endif()


target_compile_definitions (allegro
    INTERFACE 
        ALLEGRO_NO_COMPATIBILITY 
        ALLEGRO_NO_FIX_ALIASES 
        ALLEGRO_NO_FIX_CLASS
)

add_library(Allegro::Allegro ALIAS allegro)
