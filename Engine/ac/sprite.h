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
#ifndef __AGS_EE_AC__SPRITE_H
#define __AGS_EE_AC__SPRITE_H

// Converts from 32-bit RGBA image, to a 15/16/24-bit destination image,
// replacing more than half-translucent alpha pixels with transparency mask pixels.
Common::Bitmap *remove_alpha_channel(Common::Bitmap *from);
void pre_save_sprite(Common::Bitmap *image);
void initialize_sprite (int ee);

#endif // __AGS_EE_AC__SPRITE_H
