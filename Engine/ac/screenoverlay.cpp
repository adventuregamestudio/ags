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
#include "ac/screenoverlay.h"
#include "ac/dynamicsprite.h"
#include "ac/gamesetupstruct.h"
#include "ac/spritecache.h"
#include "gfx/bitmap.h"
#include "util/stream.h"

using namespace AGS::Common;

extern GameSetupStruct game;
extern SpriteCache spriteset;

ScreenOverlay::ScreenOverlay(ScreenOverlay &&over)
{
    *this = std::move(over);
}

ScreenOverlay::~ScreenOverlay()
{
    if (_sprnum > 0 && !IsSpriteShared())
        free_dynamic_sprite(_sprnum, false);
}

// TODO: this may be avoided if we somehow make (dynamic) sprites reference counted when assigning ID too
ScreenOverlay &ScreenOverlay::operator =(ScreenOverlay &&over)
{
    *this = over;
    over._sprnum = 0;
    return *this;
}

void ScreenOverlay::ResetImage()
{
    if (_sprnum > 0 && !IsSpriteShared())
        free_dynamic_sprite(_sprnum, false);
    _sprnum = 0;
    _flags &= ~(kOver_SpriteShared);
    scaleWidth = scaleHeight = offsetX = offsetY = 0;
}

Bitmap *ScreenOverlay::GetImage() const
{
    return spriteset[_sprnum];
}

Size ScreenOverlay::GetGraphicSize() const
{
    return Size(game.SpriteInfos[_sprnum].Width, game.SpriteInfos[_sprnum].Height);
}

void ScreenOverlay::SetImage(std::unique_ptr<Common::Bitmap> pic, int offx, int offy)
{
    ResetImage();

    if (pic)
    {
        offsetX = offx;
        offsetY = offy;
        scaleWidth = pic->GetWidth();
        scaleHeight = pic->GetHeight();
        _sprnum = add_dynamic_sprite(std::move(pic), SPF_OBJECTOWNED);
    }
    MarkChanged();
}

void ScreenOverlay::SetSpriteNum(int sprnum, int offx, int offy)
{
    ResetImage();

    assert(sprnum >= 0 && static_cast<uint32_t>(sprnum) < game.SpriteInfos.size());
    if (sprnum < 0 || static_cast<uint32_t>(sprnum) >= game.SpriteInfos.size())
        return;

    _flags |= kOver_SpriteShared;
    _sprnum = sprnum;
    offsetX = offx;
    offsetY = offy;
    scaleWidth = game.SpriteInfos[sprnum].Width;
    scaleHeight = game.SpriteInfos[sprnum].Height;
    MarkChanged();
}

void ScreenOverlay::SetFlip(GraphicFlip flip)
{
    _spritetf = GfxDef::GetFlagsFromFlip(flip);
    MarkChanged();
}

void ScreenOverlay::SetTint(int red, int green, int blue, int opacity, int luminance)
{
    tint_r = red;
    tint_g = green;
    tint_b = blue;
    tint_level = opacity;
    tint_light = luminance;
    _flags &= ~kOver_HasLightLevel;
    _flags |= kOver_HasTint;
    MarkChanged();
}

void ScreenOverlay::SetLightLevel(int light_level)
{
    tint_level = tint_r = tint_g = tint_b = 0;
    tint_light = light_level;
    _flags &= ~kOver_HasTint;
    _flags |= kOver_HasLightLevel;
    MarkChanged();
}

void ScreenOverlay::RemoveTint()
{
    _flags &= ~(kOver_HasTint | kOver_HasLightLevel);
    MarkChanged();
}

void ScreenOverlay::SetShader(int shader_id, int shader_handle)
{
    _shaderID = shader_id;
    _shaderHandle = shader_handle;
    MarkChanged();
}

void ScreenOverlay::RemoveShader()
{
    _shaderID = 0;
    _shaderHandle = 0;
    MarkChanged();
}

void ScreenOverlay::ReadFromSavegame(Stream *in, bool &has_bitmap, int32_t cmp_ver)
{
    ResetImage();

    in->ReadInt32(); // ddb 32-bit pointer value (nasty legacy format)
    int pic = in->ReadInt32();
    type = in->ReadInt32();
    x = in->ReadInt32();
    y = in->ReadInt32();
    timeout = in->ReadInt32();
    speechForChar = in->ReadInt32();
    associatedOverlayHandle = in->ReadInt32();
    if (cmp_ver >= kOverSvgVersion_36025)
    {
        _flags = in->ReadInt16();
    }
    else
    {
        in->ReadBool(); // [DEPRECATED] has alpha
        in->ReadBool(); // [DEPRECATED] "is screen relative" (neg for blocking speech, never saved in practice)
    }

    if (cmp_ver >= kOverSvgVersion_35028)
    {
        offsetX = in->ReadInt32();
        offsetY = in->ReadInt32();
    }
    if (cmp_ver >= kOverSvgVersion_36008)
    {
        zorder = in->ReadInt32();
        transparency = in->ReadInt32();
        scaleWidth = in->ReadInt32();
        scaleHeight = in->ReadInt32();
    }

    // New saves always save overlay images as a part of the dynamicsprite set;
    // old saves could contain images saved along with overlays
    if ((cmp_ver >= kOverSvgVersion_36108) || (_flags & kOver_SpriteShared))
    {
        _sprnum = pic;
        has_bitmap = false;
    }
    else
    {
        _sprnum = 0;
        has_bitmap = pic != 0;
    }

    if (cmp_ver >= kOverSvgVersion_400)
    {
        blendMode = (BlendMode)in->ReadInt32();
        // Reserved for colour options
        in->ReadInt32(); // colour flags
        // tint rgb + s (4 uint8)
        tint_r = in->ReadInt8();
        tint_g = in->ReadInt8();
        tint_b = in->ReadInt8();
        tint_level = in->ReadInt8();
        tint_light = in->ReadInt32(); // tint light (or light level)
        // Reserved for transform options
        _spritetf = (SpriteTransformFlags)in->ReadInt32(); // sprite transform flags1
        in->ReadInt32(); // sprite transform flags2
        in->ReadInt32(); // transform scale x
        in->ReadInt32(); // transform scale y
        in->ReadInt32(); // transform skew x
        in->ReadInt32(); // transform skew y
        rotation = in->ReadFloat32(); // transform rotate
        in->ReadInt32(); // sprite pivot x
        in->ReadInt32(); // sprite pivot y
        in->ReadInt32(); // sprite anchor x
        in->ReadInt32(); // sprite anchor y
    }

    if (cmp_ver > kOverSvgVersion_40018)
    {
        _shaderID = in->ReadInt32();
        _shaderHandle = in->ReadInt32();
        in->ReadInt32(); // reserved
        in->ReadInt32();
    }

    // Convert magic x,y values from the older saves
    if (cmp_ver < kOverSvgVersion_40005)
    {
        const int OVR_AUTOPLACE = 30000;
        if (!IsRoomLayer() && (x == OVR_AUTOPLACE))
        {
            SetAutoPosition(y); // character index was stored in y
        }
    }
}

void ScreenOverlay::WriteToSavegame(Stream *out) const
{
    out->WriteInt32(0); // ddb 32-bit pointer value (nasty legacy format)
    out->WriteInt32(_sprnum); // sprite id
    out->WriteInt32(type);
    out->WriteInt32(x);
    out->WriteInt32(y);
    out->WriteInt32(timeout);
    out->WriteInt32(speechForChar);
    out->WriteInt32(associatedOverlayHandle);
    out->WriteInt16(_flags);
    // since cmp_ver = 1
    out->WriteInt32(offsetX);
    out->WriteInt32(offsetY);
    // since cmp_ver = 2
    out->WriteInt32(zorder);
    out->WriteInt32(transparency);
    out->WriteInt32(scaleWidth);
    out->WriteInt32(scaleHeight);
    // since cmp_ver = 10
    out->WriteInt32(blendMode);
    // Reserved for colour options
    out->WriteInt32(0); // colour flags
    // tint rgb + s (4 uint8)
    out->WriteInt8(tint_r);
    out->WriteInt8(tint_g);
    out->WriteInt8(tint_b);
    out->WriteInt8(tint_level);
    out->WriteInt32(tint_light); // tint light (or light level)
    // Reserved for transform options
    out->WriteInt32(_spritetf); // sprite transform flags1
    out->WriteInt32(0); // sprite transform flags2
    out->WriteInt32(0); // transform scale x
    out->WriteInt32(0); // transform scale y
    out->WriteInt32(0); // transform skew x
    out->WriteInt32(0); // transform skew y
    out->WriteFloat32(rotation); // transform rotate
    out->WriteInt32(0); // sprite pivot x
    out->WriteInt32(0); // sprite pivot y
    out->WriteInt32(0); // sprite anchor x
    out->WriteInt32(0); // sprite anchor y
    // kOverSvgVersion_40018
    out->WriteInt32(_shaderID);
    out->WriteInt32(_shaderHandle);
    out->WriteInt32(0); // reserved
    out->WriteInt32(0);
}
