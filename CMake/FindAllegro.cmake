if (TARGET Allegro::Allegro)
    return()
endif()

if (WIN32)
    add_library(Allegro::Allegro STATIC IMPORTED)
    set_property(TARGET Allegro::Allegro PROPERTY IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/Solutions/.lib/alleg-static.lib)
    set_property(TARGET Allegro::Allegro PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/Windows/include)

    set_property(TARGET Allegro::Allegro PROPERTY INTERFACE_LINK_LIBRARIES 
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
    pkg_check_modules(ALLEGRO REQUIRED IMPORTED_TARGET GLOBAL allegro)
    add_library(Allegro::Allegro ALIAS PkgConfig::ALLEGRO)
endif ()

if (MACOS)
    find_library(FW_AUDIO_TOOLBOX AudioToolbox)
    find_library(FW_CORE_AUDIO CoreAudio)
    find_library(FW_COCOA Cocoa)
    find_library(FW_CORE_VIDEO CoreVideo)
    find_library(FW_IO_KIT IOKit)
    find_library(FW_OPEN_GL OpenGL)

    add_library(Allegro::Allegro STATIC IMPORTED)
    set_property(TARGET Allegro::Allegro PROPERTY IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/OSX/lib/liballeg.a)
    set_property(TARGET Allegro::Allegro PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/OSX/include)
    set_property(TARGET Allegro::Allegro PROPERTY INTERFACE_LINK_LIBRARIES 
        ${FW_AUDIO_TOOLBOX}
        ${FW_CORE_AUDIO}
        ${FW_COCOA}
        ${FW_CORE_VIDEO}
        ${FW_IO_KIT}
        ${FW_OPEN_GL}
        )
endif()
