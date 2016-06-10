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

#include "ac/global_dynamicsprite.h"
#include "util/wgt2allg.h" // Allegro RGB, PALETTE
#include "ac/draw.h"
#include "ac/dynamicsprite.h"
#include "ac/file.h"
#include "ac/spritecache.h"
#include "ac/runtime_defines.h" //MAX_PATH
#include "gfx/graphicsdriver.h"
#include "gfx/bitmap.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern SpriteCache spriteset;
extern IGraphicsDriver *gfxDriver;

int LoadImageFile(const char *filename) {

    char loadFromPath[MAX_PATH];
    get_current_dir_path(loadFromPath, filename);

	Bitmap *loadedFile = BitmapHelper::LoadFromFile(loadFromPath);

    if (loadedFile == NULL)
        return 0;

    int gotSlot = spriteset.findFreeSlot();
    if (gotSlot <= 0)
        return 0;

    add_dynamic_sprite(gotSlot, ReplaceBitmapWithSupportedFormat(loadedFile));

    return gotSlot;
}
