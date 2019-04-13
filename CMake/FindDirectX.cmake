set(FIND_LIBRARY_HINTS 
    ${DXSDK_DIR}
    ENV DXSDK_DIR
    "C:/Program Files (x86)/Microsoft DirectX SDK (August 2007)"
    "C:/Program Files/Microsoft DirectX SDK (August 2007)"
    "C:/apps_x86/Microsoft DirectX SDK (August 2007)"
    "C:/apps/Microsoft DirectX SDK (August 2007)"
    "$ENV{ProgramFiles}/Microsoft DirectX SDK (August 2007)"
)

add_library(ddraw STATIC IMPORTED GLOBAL)
find_library(DX_DDRAW_LIBRARY 
    NAMES ddraw ddraw.lib
    HINTS ${FIND_LIBRARY_HINTS}
    PATH_SUFFIXES Lib/x86
)
mark_as_advanced(DX_DDRAW_LIBRARY)
set_property(TARGET ddraw PROPERTY IMPORTED_LOCATION ${DX_DDRAW_LIBRARY})
add_library(DirectX::DirectDraw ALIAS ddraw)


add_library(dinput STATIC IMPORTED GLOBAL)
find_library(DX_DINPUT_LIBRARY 
    NAMES  dinput dinput.lib
    HINTS ${FIND_LIBRARY_HINTS}
    PATH_SUFFIXES Lib/x86
)
mark_as_advanced(DX_DINPUT_LIBRARY)
set_property(TARGET dinput PROPERTY IMPORTED_LOCATION ${DX_DINPUT_LIBRARY})
add_library(DirectX::DirectInput ALIAS dinput)


add_library(dsound STATIC IMPORTED GLOBAL)
find_library(DX_DSOUND_LIBRARY 
    NAMES  dsound dsound.lib
    HINTS ${FIND_LIBRARY_HINTS}
    PATH_SUFFIXES Lib/x86
)
mark_as_advanced(DX_DSOUND_LIBRARY)
set_property(TARGET dsound PROPERTY IMPORTED_LOCATION ${DX_DSOUND_LIBRARY})
add_library(DirectX::DirectSound ALIAS dsound)


add_library(dxguid STATIC IMPORTED GLOBAL)
find_library(DX_DXGUID_LIBRARY 
    NAMES dxguid dxguid.lib
    HINTS ${FIND_LIBRARY_HINTS}
    PATH_SUFFIXES Lib/x86
)
mark_as_advanced(DX_DXGUID_LIBRARY)
set_property(TARGET dxguid PROPERTY IMPORTED_LOCATION ${DX_DXGUID_LIBRARY})
add_library(DirectX::DirectXGUID ALIAS dxguid)


add_library(d3d9 STATIC IMPORTED GLOBAL)
find_library(DX_D3D9_LIBRARY 
    NAMES d3d9 d3d9.lib
    HINTS ${FIND_LIBRARY_HINTS}
    PATH_SUFFIXES Lib/x86
)
mark_as_advanced(DX_D3D9_LIBRARY)
set_property(TARGET d3d9 PROPERTY IMPORTED_LOCATION ${DX_D3D9_LIBRARY})
add_library(DirectX::Direct3D9 ALIAS d3d9)


add_library(d3dx9 STATIC IMPORTED GLOBAL)
find_library(DX_D3DX9_LIBRARY 
    NAMES d3dx9 d3dx9.lib
    HINTS ${FIND_LIBRARY_HINTS}
    PATH_SUFFIXES Lib/x86
)
mark_as_advanced(DX_D3DX9_LIBRARY)
set_property(TARGET d3dx9 PROPERTY IMPORTED_LOCATION ${DX_D3DX9_LIBRARY})
add_library(DirectX::Direct3DX9 ALIAS d3dx9)
