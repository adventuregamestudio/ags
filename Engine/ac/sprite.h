//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#ifndef __AGS_EE_AC__SPRITE_H
#define __AGS_EE_AC__SPRITE_H

#include "ac/spritecache.h"
#include "gfx/bitmap.h"

Size get_new_size_for_sprite(const Size &size, const uint32_t sprite_flags);
// Initializes a loaded sprite for use in the game, adjusts the sprite flags.
// Returns a resulting bitmap, which may be a new or old bitmap; or null on failure.
// Original bitmap **gets deleted** if a new bitmap had to be created,
// or if failed to properly initialize one.
Common::Bitmap *initialize_sprite(Common::sprkey_t index, Common::Bitmap *image, uint32_t &sprite_flags);
void post_init_sprite(Common::sprkey_t index);

#endif // __AGS_EE_AC__SPRITE_H
