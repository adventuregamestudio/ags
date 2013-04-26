//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
//
// AGS specific color blending routines for transparency and tinting effects
//
//=============================================================================

#ifndef __AC_BLENDER_H
#define __AC_BLENDER_H

enum GameGuiAlphaRenderingStyle
{
    kGuiAlphaRender_Classic = 0,
    kGuiAlphaRender_AdditiveOpacitySrcCopy,
    kGuiAlphaRender_MultiplyTranslucenceSrcBlend,
};

void init_blenders(GameGuiAlphaRenderingStyle gui_alpha_style);

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
