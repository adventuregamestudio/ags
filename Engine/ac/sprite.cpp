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
#include "ac/common.h"
#include "ac/draw.h"
#include "ac/gamesetupstruct.h"
#include "ac/sprite.h"
#include "ac/system.h"
#include "platform/base/agsplatformdriver.h"
#include "plugin/plugin_engine.h"
#include "gfx/bitmap.h"
#include "gfx/graphicsdriver.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern int eip_guinum, eip_guiobj;
extern RGB palette[256];
extern IGraphicsDriver *gfxDriver;
extern AGSPlatformDriver *platform;

Size get_new_size_for_sprite(const Size &size, const uint32_t sprite_flags)
{
    if (!game.AllowRelativeRes() || ((sprite_flags & SPF_VAR_RESOLUTION) == 0))
        return size;
    Size newsz = size;
    ctx_data_to_game_size(newsz.Width, newsz.Height, ((sprite_flags & SPF_HIRES) != 0));
    return newsz;
}

Bitmap *initialize_sprite(sprkey_t index, Bitmap *image, uint32_t &sprite_flags)
{
    int oldeip = get_our_eip();
    set_our_eip(4300);

    // If SPF_HADALPHACHANNEL is set that means that we have stripped alpha
    // channel from this sprite last time it was loaded. Add SPF_ALPHACHANNEL
    // so that we can remove it properly again (see PrepareSpriteForUse).
    if (sprite_flags & SPF_HADALPHACHANNEL)
    {
        sprite_flags |= SPF_ALPHACHANNEL;
    }
        
    // stretch sprites to correct resolution
    Size newsz = get_new_size_for_sprite(image->GetSize(), sprite_flags);

    eip_guinum = index;
    eip_guiobj = newsz.Width;
        
    Bitmap *use_bmp = image;
    if (newsz != image->GetSize())
    {
        use_bmp = new Bitmap(newsz.Width, newsz.Height, image->GetColorDepth());
        use_bmp->StretchBlt(image, RectWH(0, 0, use_bmp->GetWidth(), use_bmp->GetHeight()));
        delete image;
    }

    const bool has_alpha = (sprite_flags & SPF_ALPHACHANNEL) != 0;
    use_bmp = PrepareSpriteForUse(use_bmp, has_alpha);
    // For non-32 bit games, strip SPF_ALPHACHANNEL flag, but add SPF_HADALPHACHANNEL
    // in order to record the fact that the asset on disk has alpha channel.
    if (has_alpha && (game.GetColorDepth() < 32))
    {
        sprite_flags &= ~SPF_ALPHACHANNEL;
        sprite_flags |= SPF_HADALPHACHANNEL;
    }

    set_our_eip(oldeip);
    return use_bmp;
}

void post_init_sprite(sprkey_t index)
{
    pl_run_plugin_hooks(kPluginEvt_SpriteLoad, index);
}
