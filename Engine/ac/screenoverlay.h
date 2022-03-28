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
//
//
//=============================================================================
#ifndef __AGS_EE_AC__SCREENOVERLAY_H
#define __AGS_EE_AC__SCREENOVERLAY_H

#include <stdint.h>
#include "core/types.h"

// Forward declaration
namespace AGS { namespace Common { class Bitmap; class Stream; } }
namespace AGS { namespace Engine { class IDriverDependantBitmap; }}
using namespace AGS; // FIXME later


struct ScreenOverlay {
    Engine::IDriverDependantBitmap *bmp = nullptr;
    Common::Bitmap *pic = nullptr;
    bool hasAlphaChannel = false;
    int type = 0, timeout = 0;
    // Note that x,y are overlay's properties, that define its position in script;
    // but real drawn position is x + offsetX, y + offsetY;
    int x = 0, y = 0;
    // Border/padding offset for the tiled text windows
    int offsetX = 0, offsetY = 0;
    int bgSpeechForChar = -1;
    int associatedOverlayHandle = 0;
    int zorder = INT_MIN;
    bool positionRelativeToScreen = false;
    bool hasSerializedBitmap = false;
    int transparency = 0;

    void ReadFromFile(Common::Stream *in, int32_t cmp_ver);
    void WriteToFile(Common::Stream *out) const;
};

#endif // __AGS_EE_AC__SCREENOVERLAY_H
