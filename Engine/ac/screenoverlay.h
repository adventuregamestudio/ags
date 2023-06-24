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
// May contain owned bitmap or reference persistent sprite's id, similar to how
// other game objects do that.
// May logically exist either on UI or room layer.
//
//=============================================================================
#ifndef __AGS_EE_AC__SCREENOVERLAY_H
#define __AGS_EE_AC__SCREENOVERLAY_H

#include <memory>
#include "core/types.h"

// Forward declaration
namespace AGS { namespace Common { class Bitmap; class Stream; } }
namespace AGS { namespace Engine { class IDriverDependantBitmap; }}
using namespace AGS; // FIXME later

enum OverlayFlags
{
    kOver_AlphaChannel     = 0x0001,
    kOver_PositionAtRoomXY = 0x0002, // room-relative position, may be in ui
    kOver_RoomLayer        = 0x0004, // work in room layer (as opposed to UI)
    kOver_SpriteReference  = 0x0008, // reference persistent sprite
};

struct ScreenOverlay
{
    // Texture
    Engine::IDriverDependantBitmap *ddb = nullptr;
    int type = -1, timeout = 0;
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
    int transparency = 0;

    bool HasAlphaChannel() const { return (_flags & kOver_AlphaChannel) != 0; }
    bool IsSpriteReference() const { return (_flags & kOver_SpriteReference) != 0; }
    bool IsRoomRelative() const { return (_flags & kOver_PositionAtRoomXY) != 0; }
    bool IsRoomLayer() const { return (_flags & kOver_RoomLayer) != 0; }
    void SetAlphaChannel(bool on) { on ? _flags |= kOver_AlphaChannel : _flags &= ~kOver_AlphaChannel; }
    void SetRoomRelative(bool on) { on ? _flags |= kOver_PositionAtRoomXY : _flags &= ~kOver_PositionAtRoomXY; }
    void SetRoomLayer(bool on)
    {
        on ? _flags |= (kOver_RoomLayer | kOver_PositionAtRoomXY) :
             _flags &= ~(kOver_RoomLayer | kOver_PositionAtRoomXY);
    }
    // Gets actual overlay's image, whether owned by overlay or by a sprite reference
    Common::Bitmap *GetImage() const;
    // Get sprite reference id, or -1 if none set
    int GetSpriteNum() const { return _sprnum; }
    void SetImage(std::unique_ptr<Common::Bitmap> pic, int offx = 0, int offy = 0);
    void SetSpriteNum(int sprnum, int offx = 0, int offy = 0);
    // Tells if Overlay has graphically changed recently
    bool HasChanged() const { return _hasChanged; }
    // Manually marks GUI as graphically changed
    void MarkChanged() { _hasChanged = true; }
    // Clears changed flag
    void ClearChanged() { _hasChanged = false; }

    void ReadFromFile(Common::Stream *in, bool &has_bitmap, int32_t cmp_ver);
    void WriteToFile(Common::Stream *out) const;

private:
    int _flags = 0; // OverlayFlags
    std::unique_ptr<Common::Bitmap> _pic; // owned bitmap
    int _sprnum = -1; // sprite reference

    bool _hasChanged = false;
};

#endif // __AGS_EE_AC__SCREENOVERLAY_H
