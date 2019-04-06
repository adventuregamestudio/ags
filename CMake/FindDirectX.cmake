if (NOT WIN32)
  return()
endif()

set(FIND_LIBRARY_HINTS 
    ${DXSDK_DIR}
    ENV DXSDK_DIR
    "C:/Program Files (x86)/Microsoft DirectX SDK (August 2007)"
    "C:/Program Files/Microsoft DirectX SDK (August 2007)"
    "C:/apps_x86/Microsoft DirectX SDK (August 2007)"
    "C:/apps/Microsoft DirectX SDK (August 2007)"
    "$ENV{ProgramFiles}/Microsoft DirectX SDK (August 2007)"
)

find_library(DX_DDRAW_LIBRARY 
    NAMES ddraw ddraw.lib
    HINTS ${FIND_LIBRARY_HINTS}
    PATH_SUFFIXES Lib/x86
)

find_library(DX_DINPUT_LIBRARY 
    NAMES  dinput dinput.lib
    HINTS ${FIND_LIBRARY_HINTS}
    PATH_SUFFIXES Lib/x86
)

find_library(DX_DSOUND_LIBRARY 
    NAMES  dsound dsound.lib
    HINTS ${FIND_LIBRARY_HINTS}
    PATH_SUFFIXES Lib/x86
)

find_library(DX_DXGUID_LIBRARY 
    NAMES dxguid dxguid.lib
    HINTS ${FIND_LIBRARY_HINTS}
    PATH_SUFFIXES Lib/x86
)

find_library(DX_D3D9_LIBRARY 
    NAMES d3d9 d3d9.lib
    HINTS ${FIND_LIBRARY_HINTS}
    PATH_SUFFIXES Lib/x86
)

find_library(DX_D3DX9_LIBRARY 
    NAMES d3dx9 d3dx9.lib
    HINTS ${FIND_LIBRARY_HINTS}
    PATH_SUFFIXES Lib/x86
)

add_library(DirectX::DirectDraw UNKNOWN IMPORTED)
set_property(TARGET DirectX::DirectDraw PROPERTY IMPORTED_LOCATION ${DX_DDRAW_LIBRARY})

add_library(DirectX::DirectInput UNKNOWN IMPORTED)
set_property(TARGET DirectX::DirectInput PROPERTY IMPORTED_LOCATION ${DX_DINPUT_LIBRARY})

add_library(DirectX::DirectSound UNKNOWN IMPORTED)
set_property(TARGET DirectX::DirectSound PROPERTY IMPORTED_LOCATION ${DX_DSOUND_LIBRARY})

add_library(DirectX::DirectXGUID UNKNOWN IMPORTED)
set_property(TARGET DirectX::DirectXGUID PROPERTY IMPORTED_LOCATION ${DX_DXGUID_LIBRARY})

add_library(DirectX::Direct3D9 UNKNOWN IMPORTED)
set_property(TARGET DirectX::Direct3D9 PROPERTY IMPORTED_LOCATION ${DX_D3D9_LIBRARY})

add_library(DirectX::Direct3DX9 UNKNOWN IMPORTED)
set_property(TARGET DirectX::Direct3DX9 PROPERTY IMPORTED_LOCATION ${DX_D3DX9_LIBRARY})
