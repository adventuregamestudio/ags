//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__SPRITE_H
#define __AGS_EE_AC__SPRITE_H

void get_new_size_for_sprite (int ee, int ww, int hh, int &newwid, int &newhit);
// set any alpha-transparent pixels in the image to the appropriate
// RGB mask value so that the ->Blit calls work correctly
void set_rgb_mask_using_alpha_channel(Common::Bitmap *image);
// from is a 32-bit RGBA image, to is a 15/16/24-bit destination image
Common::Bitmap *remove_alpha_channel(Common::Bitmap *from);
void pre_save_sprite(Common::Bitmap *image);
void initialize_sprite (int ee);

#endif // __AGS_EE_AC__SPRITE_H
