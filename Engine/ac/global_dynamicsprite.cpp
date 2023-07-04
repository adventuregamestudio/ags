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
#include "ac/asset_helper.h"
#include "ac/draw.h"
#include "ac/dynamicsprite.h"
#include "ac/path_helper.h"
#include "ac/spritecache.h"
#include "gfx/graphicsdriver.h"
#include "gfx/bitmap.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern SpriteCache spriteset;
extern IGraphicsDriver *gfxDriver;

int LoadImageFile(const char *filename)
{
    String res_path;
    std::unique_ptr<Stream> in(
        ResolveScriptPathAndOpen(filename, FileOpenMode::kFile_Open, FileWorkMode::kFile_Read, res_path));
    if (!in)
        return 0;

    String ext = Path::GetFileExtension(filename).Lower(); // FIXME: don't require lower!
    Bitmap *loadedFile = BitmapHelper::LoadBitmap(ext, in.get());
    if (!loadedFile)
        return 0;

    int gotSlot = spriteset.GetFreeIndex();
    if (gotSlot <= 0)
        return 0;

    add_dynamic_sprite(gotSlot, PrepareSpriteForUse(loadedFile, false));

    return gotSlot;
}
