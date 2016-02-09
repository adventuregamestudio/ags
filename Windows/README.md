# Adventure Game Studio - Windows

## Build Requirements

-   Visual Studio 2008 SP1 - it is required to be this exact version.
-   DirectX SDK April 2007 ([Download](https://www.microsoft.com/en-us/download/details.aspx?id=23776))
-   OpenGL API and Extension Header Files ([Download](https://www.opengl.org/registry/#headers))
-   Python 2.7 with PyWin32 extension ([Download](http://www.activestate.com/activepython/downloads))
-   InnoSetup 5.5 ([Download](http://www.jrsoftware.org/isdl.php))

At a minimum, Visual Studio, DirectX SDK and OpenGL header files are required. Python is only necessary for building
the user manual and installer.

## Environment Variables

Some environment variables must be set in the system to help the Visual Studio projects find certain header files.

-   DIRECTX_SDK_INCLUDE_DIR e.g. "C:\Program Files (x86)\Microsoft DirectX SDK (April 2007)\Include"
-   OPENGL_INCLUDE_DIR e.g. "C:\opt\opengl-20150720\include"

## Instructions

[Instructions are available in the AGS wiki.](http://www.adventuregamestudio.co.uk/wiki/Compiling_AGS)
