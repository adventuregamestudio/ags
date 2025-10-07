//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// ScreenOverlay is a simple sprite container with no advanced functions.
// Contains an id of a sprite, which may be either owned by overlay, or shared
// to whole game similar to how other objects use sprites.
// May logically exist either on UI or room layer.
//
// TODO: historically overlay objects contained an actual bitmap in them.
// This was remade into having a dynamic sprite allocated exclusively for
// overlay. But sprites do not have any kind of a ref count of their own
// (unless exported into script as DynamicSprite), so we have to keep an
// overlay's flag, which tells that the sprite it references must be deleted
// on overlay's disposal. This should be improved at some point, by devising
// a better kind of a sprite's ownership mechanic.
//
//=============================================================================
#ifndef __AGS_EE_AC__SCREENOVERLAY_H
#define __AGS_EE_AC__SCREENOVERLAY_H

#include <functional>
#include <memory>
#include "ac/dynobj/scriptoverlay.h"
#include "core/types.h"
#include "util/geometry.h"

// Forward declaration
namespace AGS { namespace Common { class Bitmap; class Stream; } }
namespace AGS { namespace Engine { class IDriverDependantBitmap; }}
using namespace AGS; // FIXME later

// NOTE: overlay flags are serialized as int16
enum OverlayFlags
{
    kOver_AlphaChannel     = 0x0001,
    kOver_PositionAtRoomXY = 0x0002, // room-relative position, may be in ui
    kOver_RoomLayer        = 0x0004, // work in room layer (as opposed to UI)
    kOver_SpriteShared     = 0x0008, // reference shared sprite (as opposed to exclusive)
};

enum OverlaySvgVersion
{
    kOverSvgVersion_Initial = 0,
    kOverSvgVersion_35028   = 1, // offset x,y
    kOverSvgVersion_36008   = 2, // z, transparency
    kOverSvgVersion_36025   = 3, // merged options into flags
    kOverSvgVersion_36108   = 4, // don't save owned sprites (use dynamic sprites)
};

class ScreenOverlay
{
public:
    typedef std::function<void(ScreenOverlay &over)> PfnOverlayRemoved;

    ScreenOverlay() = default;
    ScreenOverlay(int id) : _id(id) {}
    ScreenOverlay(ScreenOverlay&&);
    ~ScreenOverlay();

    ScreenOverlay &operator =(ScreenOverlay&&);

    int  GetID() const { return _id; }
    int  GetX() const { return _x; }
    int  GetY() const { return _y; }
    int  GetOffsetX() const { return _offx; }
    int  GetOffsetY() const { return _offy; }
    // Get position of a overlay's sprite (may be displayed with an offset relative to overlay's pos)
    int  GetDrawX() const { return _x + _offx; }
    int  GetDrawY() const { return _y + _offy; }
    const Size &GetScaledSize() const { return _scaledSize; }
    int  GetScaledWidth() const { return _scaledSize.Width; }
    int  GetScaledHeight() const { return _scaledSize.Height; }
    bool HasAlphaChannel() const { return (_flags & kOver_AlphaChannel) != 0; }
    bool IsSpriteShared() const { return (_flags & kOver_SpriteShared) != 0; }
    bool IsRoomRelative() const { return (_flags & kOver_PositionAtRoomXY) != 0; }
    bool IsRoomLayer() const { return (_flags & kOver_RoomLayer) != 0; }
    int  GetTransparency() const { return _transparency; }
    int  GetZOrder() const { return _zorder; }
    // Gets actual overlay's image
    Common::Bitmap *GetImage() const;
    // Get this overlay's sprite id
    int  GetSpriteNum() const { return _sprnum; }
    // Get this overlay's graphic's dimensions (unscaled)
    Size GetGraphicSize() const;
    int  GetTimeout() const { return _timeout; }
    int  GetCharacterRef() const { return _bgSpeechForChar; }
    int  GetScriptHandle() const { return _scriptHandle; }

    void SetAlphaChannel(bool on) { on ? _flags |= kOver_AlphaChannel : _flags &= ~kOver_AlphaChannel; }
    void SetRoomRelative(bool on) { on ? _flags |= kOver_PositionAtRoomXY : _flags &= ~kOver_PositionAtRoomXY; }
    void SetRoomLayer(bool on)
    {
        on ? _flags |= (kOver_RoomLayer | kOver_PositionAtRoomXY) :
             _flags &= ~(kOver_RoomLayer | kOver_PositionAtRoomXY);
    }
    void SetPosition(int x, int y);
    void SetScaledSize(int w, int h);
    void SetTransparency(int trans);
    void SetZOrder(int zorder);
    // Assigns an exclusive image to this overlay; the image will be stored as a dynamic sprite
    // in a sprite cache, but owned by this overlay and therefore disposed at its disposal
    void SetImage(std::unique_ptr<Common::Bitmap> pic, bool has_alpha = false, int offx = 0, int offy = 0);
    // Assigns a shared sprite to this overlay
    void SetSpriteNum(int sprnum, int offx = 0, int offy = 0);
    // Assigns a role of background speech
    void SetAsBackgroundSpeech(int char_id, int timeout);
    // Creates a script object associated with this overlay;
    // optionally adds an internal reference to prevent script object's disposal
    ScriptOverlay *CreateScriptObject();
    // Resets script object handle (script object will remain in script memory)
    void DetachScriptObject();
    // Sets a callback to run whenever overlay's Dispose method is called
    void SetRemoveCallback(PfnOverlayRemoved remove_cb);

    // Disposes overlay's resources. Disposes script object (if one was created).
    // Calls remove callback, if one is set.
    // TODO: revise this, figure out if it's safe to call one from destructor,
    // might need to double check the ScreenOverlay's removal from storage
    void OnRemove();
    // Decrements timeout by one tick
    int  UpdateTimeout();

    // Tells if Overlay has graphically changed recently
    bool HasChanged() const { return _hasChanged; }
    // Manually marks GUI as graphically changed
    void MarkChanged() { _hasChanged = true; }
    // Clears changed flag
    void ClearChanged() { _hasChanged = false; }
    // Marks that the overlay's image has changed and the texture has to be updated
    void MarkImageChanged();

    void ReadFromSavegame(Common::Stream *in, bool &has_bitmap, int32_t cmp_ver);
    void WriteToSavegame(Common::Stream *out) const;

private:
    void ResetImage();
    // Disposes an associated script object, if one was created earlier;
    // releases internal reference if one was made
    // (script object may exist while there are user refs)
    void DisposeScriptObject();

    ScreenOverlay(const ScreenOverlay &) = default;
    ScreenOverlay &operator =(const ScreenOverlay&) = default;

    // Overlay's ID
    int _id = -1;
    int _flags = 0; // OverlayFlags
    // Note that x,y are overlay's properties, that define its position in script;
    // but real drawn position is x + offx, y + offy;
    int _x = 0;
    int _y = 0;
    // Border/padding offset for the tiled text windows
    int _offx = 0;
    int _offy = 0;
    int _sprnum = 0;
    // The size to stretch the texture to
    Size _scaledSize;
    int _zorder = INT_MIN;
    int _transparency = 0;
    // Timeout for automatic removal, 0 means disabled
    int _timeout = 0;
    // Index of a Character whose background speech this overlay represents;
    // TODO: redesign this, store the overlay's reference in character instead,
    // overlay should not have such data as its member.
    int _bgSpeechForChar = -1;
    int _scriptHandle = 0;
    PfnOverlayRemoved _removeCb;

    bool _hasChanged = false;
};

#endif // __AGS_EE_AC__SCREENOVERLAY_H
