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
#include "ac/runtime_defines.h"
#include "ac/dynobj/scriptoverlay.h"
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
    kOverSvgVersion_40018   = 4000018, // shaders
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
    Point GetDrawPos() const { return Point(GetDrawX(), GetDrawY()); }
    const Size &GetScaledSize() const { return _scaledSize; }
    int  GetScaledWidth() const { return _scaledSize.Width; }
    int  GetScaledHeight() const { return _scaledSize.Height; }
    float GetRotation() const { return _rotation; }
    bool IsSpriteShared() const { return (_flags & kOver_SpriteShared) != 0; }
    bool IsAutoPosition() const { return (_flags & kOver_AutoPosition) != 0; }
    bool IsRoomLayer() const { return (_flags & kOver_RoomLayer) != 0; }
    int  GetTransparency() const { return _transparency; }
    Common::BlendMode GetBlendMode() const { return _blendMode; }
    Common::GraphicFlip GetFlip() const { return Common::GfxDef::GetFlipFromFlags(_spritetf); }
    bool HasTint() const { return (_flags & kOver_HasTint) != 0; }
    bool HasLightLevel() const { return (_flags & kOver_HasLightLevel) != 0; }
    int  GetTintLevel() const { return _tintLevel; }
    int  GetTintR() const { return _tintR; }
    int  GetTintG() const { return _tintG; }
    int  GetTintB() const { return _tintB; }
    int  GetTintLight() const { return _tintLight; }
    int  GetZOrder() const { return _zorder; }
    // Gets actual overlay's image
    Common::Bitmap *GetImage() const;
    // Get this overlay's sprite id
    int  GetSpriteNum() const { return _sprnum; }
    // Get this overlay's graphic's dimensions (unscaled)
    Size GetGraphicSize() const;
    // Gets shader id
    int  GetShaderID() const { return _shaderID; }
    // Gets script shader's managed handle
    int  GetShaderHandle() const { return _shaderHandle; }
    int  GetTimeout() const { return _timeout; }
    int  GetCharacterRef() const { return _speechForChar; }
    int  GetScriptHandle() const { return _scriptHandle; }
    void SetRoomLayer(bool on)
    {
        on ? _flags |= (kOver_RoomLayer | kOver_PositionAtRoomXY) :
             _flags &= ~(kOver_RoomLayer | kOver_PositionAtRoomXY);
    }
    // Assigns an automatic alignment to the given character's pos
    void SetAutoPosition(int for_character, int x = 0, int y = 0);
    // Assigns a fixed position in the current space (screen or room coordinates)
    void SetFixedPosition(int x, int y);
    void SetScaledSize(int w, int h);
    void SetRotation(float rotation);
    // Assigns a shader to overlay
    void SetZOrder(int zorder);
    // Assigns an exclusive image to this overlay; the image will be stored as a dynamic sprite
    // in a sprite cache, but owned by this overlay and therefore disposed at its disposal
    void SetImage(std::unique_ptr<Common::Bitmap> pic, int offx = 0, int offy = 0);
    // Assigns a shared sprite to this overlay
    void SetSpriteNum(int sprnum, int offx = 0, int offy = 0);
    // Assigns flip setting
    void SetFlip(Common::GraphicFlip flip);
    // Assigns overlay transparency (0 - 255)
    void SetTransparency(int trans);
    // Assigns blend mode
    void SetBlendMode(Common::BlendMode blend_mode);
    // Assigns tint settings
    void SetTint(int red, int green, int blue, int opacity, int luminance);
    // Assigns light level
    void SetLightLevel(int light_level);
    // Removes tint and light level
    void RemoveTint();
    // Assigns a shader to overlay
    void SetShader(int shader_id, int shader_handle);
    // Removes a shader reference
    void RemoveShader();
    // Assigns a role of character's speech
    void SetAsSpeech(int char_id, int timeout);
    // Creates a script object associated with this overlay.
    ScriptOverlay *CreateScriptObject();
    // Registers the provided script object, and attaches to this overlay
    // TODO: revise this, not a particularly secure design.
    void AssignScriptObject(ScriptOverlay *scover);
    // Resets script object handle (script object will remain in script memory)
    void DetachScriptObject();
    // Sets a callback to run whenever overlay's Dispose method is called
    void SetRemoveCallback(PfnOverlayRemoved remove_cb);

    // Disposes overlay's resources. Disposes script object (if one was created).
    // Calls remove callback, if one is set.
    // TODO: revise this, figure out if it's safe to call one from destructor,
    // might need to double check the ScreenOverlay's removal from storage
    void OnRemove();
    // Recalculates overlay's transform matrix and AABB
    void UpdateGraphicSpace();
    // Decrements timeout by one tick
    int  UpdateTimeout();

    // Returns Overlay's graphic space params
    inline const Common::GraphicSpace &GetGraphicSpace() const { return _gs; }

    // Tells if Overlay has graphically changed recently
    bool HasChanged() const { return _hasChanged; }
    // Manually marks overlay as graphically changed
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
    Common::SpriteTransformFlags _spritetf = Common::kSprTf_None;
    // The size to stretch the texture to
    Size _scaledSize;
    float _rotation = 0.f;
    int _zorder = INT_MIN;
    int _transparency = 0;
    uint8_t _tintR = 0u;
    uint8_t _tintG = 0u;
    uint8_t _tintB = 0u;
    uint8_t _tintLevel = 0u;
    int _tintLight = 0; // -100 to 100 (comply to objects and characters)
    Common::BlendMode _blendMode = Common::kBlend_Normal;
    // TODO: a RAII wrapper over managed handle, that auto releases the reference
    int _shaderID = 0;
    int _shaderHandle = 0;
    // Timeout for automatic removal, 0 means disabled
    int _timeout = 0;
    // Index of a Character whose speech (blocking or bg) this overlay represents;
    // TODO: redesign this, store the overlay's reference in character instead,
    // overlay should not have such data as its member.
    int _speechForChar = -1;
    int _scriptHandle = 0;
    PfnOverlayRemoved _removeCb;

    Common::GraphicSpace _gs;
    bool _hasChanged = false;
};


enum AnimatedOverlayFlags
{
    kAnimOver_PauseWithGame = 0x0001
};

enum AnimOverlaySvgVersion
{
    kAnimOverSvgVersion_40022 = 4000022, // initial
};

struct ViewFrame;

class AnimatedOverlay
{
public:
    AnimatedOverlay() = default;
    AnimatedOverlay(int over_id, uint32_t flags)
        : _overid(over_id), _flags(flags)
    {}

    int GetOverID() const { return _overid; }
    bool IsAnimating() const { return _anim.IsValid(); }
    bool GetPauseWithGame() const { return (_flags & kAnimOver_PauseWithGame) != 0; }
    int GetView() const { return _view; }
    int GetLoop() const { return _loop; }
    int GetFrame() const { return _frame; }
    const ViewFrame *GetViewFrame() const;

    void Begin(int view, int loop, int frame, const ViewAnimateParams &params);
    bool UpdateOnce();
    void Reset();

    void ReadFromSavegame(Common::Stream *in, int cmp_ver);
    void WriteToSavegame(Common::Stream *out) const;

private:
    int _overid = -1;
    uint32_t _flags = 0u; // AnimatedOverlayFlags
    // current animation status
    int _view = -1;
    uint16_t _loop = 0u;
    uint16_t _frame = 0u;
    ViewAnimateParams _anim;
    int _wait = 0;
};

#endif // __AGS_EE_AC__SCREENOVERLAY_H
