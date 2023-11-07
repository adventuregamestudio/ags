//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "ac/common.h"
#include "ac/draw.h"
#include "ac/gamesetupstruct.h"
#include "ac/sprite.h"
#include "ac/system.h"
#include "platform/base/agsplatformdriver.h"
#include "plugin/agsplugin_evts.h"
#include "plugin/plugin_engine.h"
#include "gfx/bitmap.h"
#include "gfx/graphicsdriver.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern int our_eip, eip_guinum, eip_guiobj;
extern RGB palette[256];
extern IGraphicsDriver *gfxDriver;
extern AGSPlatformDriver *platform;

// CLNUP remove 15-16 bit support
// from is a 32-bit RGBA image, to is a 15/16/24-bit destination image
Bitmap *remove_alpha_channel(Bitmap *from)
{
    const int game_cd = game.GetColorDepth();
    Bitmap *to = BitmapHelper::CreateBitmap(from->GetWidth(), from->GetHeight(), game_cd);
    const int maskcol = to->GetMaskColor();
    int y,x;
    unsigned int c,b,g,r;

    if (game_cd == 24) // 32-to-24
    {
        for (y=0; y < from->GetHeight(); y++) {
            unsigned int*psrc = (unsigned int *)from->GetScanLine(y);
            unsigned char*pdest = (unsigned char*)to->GetScanLine(y);

            for (x=0; x < from->GetWidth(); x++) {
                c = psrc[x];
                // less than 50% opaque, remove the pixel
                if (((c >> 24) & 0x00ff) < 128)
                    c = maskcol;

                // copy the RGB values across
                memcpy(&pdest[x * 3], &c, 3);
            }
        }
    }
    else if (game_cd > 8) // 32 to 15 or 16
    {
        for (y=0; y < from->GetHeight(); y++) {
            unsigned int*psrc = (unsigned int *)from->GetScanLine(y);
            unsigned short*pdest = (unsigned short *)to->GetScanLine(y);

            for (x=0; x < from->GetWidth(); x++) {
                c = psrc[x];
                // less than 50% opaque, remove the pixel
                if (((c >> 24) & 0x00ff) < 128)
                    pdest[x] = maskcol;
                else {
                    // otherwise, copy it across
                    r = (c >> 16) & 0x00ff;
                    g = (c >> 8) & 0x00ff;
                    b = c & 0x00ff;
                    pdest[x] = makecol_depth(game_cd, r, g, b);
                }
            }
        }
    }
    else // 32 to 8-bit game
    { // TODO: consider similar to above approach if this becomes a wanted feature
        to->Blit(from);
    }
    return to;
}

Bitmap *initialize_sprite(sprkey_t index, Bitmap *image, uint32_t &sprite_flags)
{
    return PrepareSpriteForUse(image);
}

void post_init_sprite(sprkey_t index)
{
    pl_run_plugin_hooks(AGSE_SPRITELOAD, index);
}
