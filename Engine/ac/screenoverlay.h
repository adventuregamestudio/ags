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
// ScreenOverlay is a simple sprite container with no advanced functions.
//
//=============================================================================
#ifndef __AGS_EE_AC__SCREENOVERLAY_H
#define __AGS_EE_AC__SCREENOVERLAY_H

#include "core/types.h"
#include "gfx/gfx_def.h"

// Forward declaration
namespace AGS { namespace Common { class Bitmap; class Stream; } }
namespace AGS { namespace Engine { class IDriverDependantBitmap; }}
using namespace AGS; // FIXME later


struct ScreenOverlay {
    // TODO: what if we actually register a real dynamic sprite for overlay?
    Common::Bitmap *pic = nullptr;
    bool hasAlphaChannel = false;
    Engine::IDriverDependantBitmap *bmp = nullptr;
    int type = 0, x = 0, y = 0, timeout = 0;
    int bgSpeechForChar = 0;
    int associatedOverlayHandle = 0;
    int zorder = INT_MIN;
    bool positionRelativeToScreen = false;
    int _offsetX = 0, _offsetY = 0;
    Common::BlendMode blendMode = Common::kBlend_Normal;
    int transparency = 0;

    void ReadFromFile(Common::Stream *in, bool &has_bitmap, int32_t cmp_ver);
    void WriteToFile(Common::Stream *out) const;
};

#endif // __AGS_EE_AC__SCREENOVERLAY_H
