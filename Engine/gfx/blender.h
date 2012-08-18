/*
AGS specific color blending routines for transparency and tinting
effects

Adventure Game Studio source code Copyright 1999-2011 Chris Jones.
All rights reserved.

The AGS Editor Source Code is provided under the Artistic License 2.0
http://www.opensource.org/licenses/artistic-license-2.0.php

You MAY NOT compile your own builds of the engine without making it EXPLICITLY
CLEAR that the code has been altered from the Standard Version.

*/

#ifndef __AC_BLENDER_H
#define __AC_BLENDER_H

unsigned long _myblender_color15(unsigned long x, unsigned long y, unsigned long n);
unsigned long _myblender_color16(unsigned long x, unsigned long y, unsigned long n);
unsigned long _myblender_color32(unsigned long x, unsigned long y, unsigned long n);
unsigned long _myblender_color15_light(unsigned long x, unsigned long y, unsigned long n);
unsigned long _myblender_color16_light(unsigned long x, unsigned long y, unsigned long n);
unsigned long _myblender_color32_light(unsigned long x, unsigned long y, unsigned long n);
void set_my_trans_blender(int r, int g, int b, int a);
void set_additive_alpha_blender();
void set_opaque_alpha_blender();

#endif // __AC_BLENDER_H