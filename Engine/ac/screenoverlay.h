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

// Forward declaration
namespace AGS { namespace Common { class Bitmap; class Stream; } }
namespace AGS { namespace Engine { class IDriverDependantBitmap; }}
using namespace AGS; // FIXME later

// Overlay class.
// TODO: currently overlay creates and stores its own bitmap, even if
// created using existing sprite. As a side-effect, changing that sprite
// (if it were a dynamic one) will not affect overlay (unlike other objects).
// For future perfomance optimization it may be desired to store sprite index
// instead; but that would mean that overlay will have to receive sprite
// changes. For backward compatibility there may be a game switch that
// forces it to make a copy.
struct ScreenOverlay {
    // Texture
    Engine::IDriverDependantBitmap *ddb = nullptr;
    // Original bitmap
    Common::Bitmap *pic = nullptr;
    bool hasAlphaChannel = false;
    int type = 0, timeout = 0;
    // Note that x,y are overlay's properties, that define its position in script;
    // but real drawn position is x + offsetX, y + offsetY;
    int x = 0, y = 0;
    // Border/padding offset for the tiled text windows
    int offsetX = 0, offsetY = 0;
    // Width and height to stretch the texture to
    int scaleWidth = 0, scaleHeight = 0;
    int bgSpeechForChar = -1;
    int associatedOverlayHandle = 0; // script obj handle
    int zorder = INT_MIN;
    bool positionRelativeToScreen = false;
    int transparency = 0;

    // Tells if Overlay has graphically changed recently
    bool HasChanged() const { return _hasChanged; }
    // Manually marks GUI as graphically changed
    void MarkChanged() { _hasChanged = true; }
    // Clears changed flag
    void ClearChanged() { _hasChanged = false; }

    void ReadFromFile(Common::Stream *in, bool &has_bitmap, int32_t cmp_ver);
    void WriteToFile(Common::Stream *out) const;

private:
    bool _hasChanged = false;
};

#endif // __AGS_EE_AC__SCREENOVERLAY_H
