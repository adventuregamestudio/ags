//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
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
    if (!spriteset.HasFreeSlots())
        return 0;

    std::unique_ptr<Stream> in(
        ResolveScriptPathAndOpen(filename, FileOpenMode::kFile_Open, StreamMode::kStream_Read));
    if (!in)
        return 0;

    String ext = Path::GetFileExtension(filename);
    std::unique_ptr<Bitmap> image(BitmapHelper::LoadBitmap(ext, in.get()));
    if (!image)
        return 0;

    return add_dynamic_sprite(std::unique_ptr<Bitmap>(
        PrepareSpriteForUse(image.release(), false)));
}
