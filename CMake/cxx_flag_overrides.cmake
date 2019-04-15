# For reference https://gitlab.kitware.com/cmake/community/wikis/FAQ#make-override-files
# Original values are available in cmake repo: Modules/Platform/Windows-MSVC.cmake
if(MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG_INIT          "/MTd /Zi /Ob0 /Od /RTC1")
    set(CMAKE_CXX_FLAGS_RELEASE_INIT        "/MT /O2 /Ob2 /DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT "/MT /Zi /O2 /Ob1 /DNDEBUG")
    set(CMAKE_CXX_FLAGS_MINSIZEREL_INIT     "/MT /O1 /Ob1 /DNDEBUG")
endif()