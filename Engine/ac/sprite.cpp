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

Bitmap *initialize_sprite(sprkey_t index, Bitmap *image, uint32_t &sprite_flags)
{
    return PrepareSpriteForUse(image, (sprite_flags & SPF_KEEPDEPTH) == 0);
}

void post_init_sprite(sprkey_t index)
{
    pl_run_plugin_hooks(AGSE_SPRITELOAD, index);
}
