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

#include <memory>
#include "core/types.h"
#include "gfx/gfx_def.h"

// Forward declaration
namespace AGS { namespace Common { class Bitmap; class Stream; class GraphicSpace; } }
namespace AGS { namespace Engine { class IDriverDependantBitmap; }}
using namespace AGS; // FIXME later

// Overlay behavior flags
// NOTE: currently serialized as uint16
enum OverlayFlags
{
    // kOver_PositionAtRoomXY is now redundant, but is kept for safety and backwards compat;
    // must be set strictly along with kOver_RoomLayer
    kOver_PositionAtRoomXY = 0x0002, // room-relative position
    kOver_RoomLayer        = 0x0004, // work in room layer (as opposed to UI)
    kOver_SpriteShared     = 0x0008, // reference shared sprite (as opposed to exclusive)
    kOver_AutoPosition     = 0x0010, // autoposition over a linked Character (speech)
    kOver_HasTint          = 0x0020,
    kOver_HasLightLevel    = 0x0040,
};

enum OverlaySvgVersion
{
    kOverSvgVersion_Initial = 0,
    kOverSvgVersion_35028   = 1, // offset x,y
    kOverSvgVersion_36008   = 2, // z, transparency
    kOverSvgVersion_36025   = 3, // merged options into flags
    kOverSvgVersion_36108   = 4, // don't save owned sprites (use dynamic sprites)
    kOverSvgVersion_400     = 4000000, // blend mode, etc
    kOverSvgVersion_40005   = 4000005, // no magic values stored in x,y
};

// TODO: what if we actually register a real dynamic sprite for overlay?
struct ScreenOverlay
{
    // Overlay's "type" is a merged special overlay ID and internal container index
    int type = -1;
    int timeout = 0;
    // Note that x,y are overlay's properties, that define its position in script;
    // but real drawn position is x + offsetX, y + offsetY;
    int x = 0, y = 0;
    // Border/padding offset for the tiled text windows
    int offsetX = 0, offsetY = 0;
    // Width and height to stretch the texture to
    int scaleWidth = 0, scaleHeight = 0;
    // Character index, if this overlay serves as a speech (blocking or bg)
    int speechForChar = -1;
    int associatedOverlayHandle = 0; // script obj handle
    int zorder = INT_MIN;
    float rotation = 0.f;
    uint8_t tint_r = 0u, tint_g = 0u, tint_b = 0u, tint_level = 0u;
    int tint_light = 0; // -100 to 100 (comply to objects and characters)
    Common::BlendMode blendMode = Common::kBlend_Normal;
    int shader_id = 0;
    int transparency = 0;
    Common::GraphicSpace _gs;

    ScreenOverlay() = default;
    ScreenOverlay(ScreenOverlay&&);
    ~ScreenOverlay();

    ScreenOverlay &operator =(ScreenOverlay&&);

    // Returns Overlay's graphic space params
    inline const Common::GraphicSpace &GetGraphicSpace() const { return _gs; }
    bool IsSpriteShared() const { return (_flags & kOver_SpriteShared) != 0; }
    bool IsAutoPosition() const { return (_flags & kOver_AutoPosition) != 0; }
    bool IsRoomLayer() const { return (_flags & kOver_RoomLayer) != 0; }
    Common::GraphicFlip GetFlip() const { return Common::GfxDef::GetFlipFromFlags(_spritetf); }
    bool HasTint() const { return (_flags & kOver_HasTint) != 0; }
    bool HasLightLevel() const { return (_flags & kOver_HasLightLevel) != 0; }
    void SetAutoPosition(int for_character)
    {
        _flags |= kOver_AutoPosition;
        speechForChar = for_character;
    }
    void SetFixedPosition(int x_, int y_)
    {
        _flags &= ~kOver_AutoPosition;
        x = x_; y = y_;
        speechForChar = -1;
    }
    void SetRoomLayer(bool on)
    {
        on ? _flags |= (kOver_RoomLayer | kOver_PositionAtRoomXY) :
             _flags &= ~(kOver_RoomLayer | kOver_PositionAtRoomXY);
    }
    // Gets actual overlay's image, whether owned by overlay or by a sprite reference
    Common::Bitmap *GetImage() const;
    // Get this overlay's sprite id
    int  GetSpriteNum() const { return _sprnum; }
    Size GetGraphicSize() const;
    // Assigns an exclusive image to this overlay; the image will be stored as a dynamic sprite
    // in a sprite cache, but owned by this overlay and therefore disposed at its disposal
    void SetImage(std::unique_ptr<Common::Bitmap> pic, int offx = 0, int offy = 0);
    // Assigns a shared sprite to this overlay
    void SetSpriteNum(int sprnum, int offx = 0, int offy = 0);
    // Assigns flip setting
    void SetFlip(Common::GraphicFlip flip);
    // Assigns tint settings
    void SetTint(int red, int green, int blue, int opacity, int luminance);
    // Assigns light level
    void SetLightLevel(int light_level);
    // Removes tint and light level
    void RemoveTint();
    // Tells if Overlay has graphically changed recently
    bool HasChanged() const { return _hasChanged; }
    // Manually marks GUI as graphically changed
    void MarkChanged() { _hasChanged = true; }
    // Clears changed flag
    void ClearChanged() { _hasChanged = false; }

    void ReadFromSavegame(Common::Stream *in, bool &has_bitmap, int32_t cmp_ver);
    void WriteToSavegame(Common::Stream *out) const;

private:
    void ResetImage();

    ScreenOverlay(const ScreenOverlay &) = default;
    ScreenOverlay &operator =(const ScreenOverlay&) = default;

    int _flags = 0; // OverlayFlags
    int _sprnum = 0; // sprite id
    Common::SpriteTransformFlags _spritetf = Common::kSprTf_None;
    bool _hasChanged = false;
};

#endif // __AGS_EE_AC__SCREENOVERLAY_H
