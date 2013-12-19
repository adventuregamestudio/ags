This is the Humble Bundle port of AGS to OS X for Gemini Rue

This will work on Mac OS X 10.6+ (tested through 10.9).  It does so via a modified allegro 4 library which sets up an offscreen buffer that the game renders to. And then blits that to texture memory in the OpenGL graphics hardware and finally renders that texture to screen.  Currently aspect ratios are not taken into account (and probably should be).

