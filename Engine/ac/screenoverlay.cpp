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
#include "ac/screenoverlay.h"
#include "ac/dynamicsprite.h"
#include "ac/gamesetupstruct.h"
#include "ac/spritecache.h"
#include "gfx/bitmap.h"
#include "util/stream.h"

using namespace AGS::Common;

extern GameSetupStruct game;

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
    _flags &= ~(kOver_SpriteShared | kOver_AlphaChannel);
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

void ScreenOverlay::SetImage(std::unique_ptr<Common::Bitmap> pic, bool has_alpha, int offx, int offy)
{
    ResetImage();

    if (pic)
    {
        _flags |= kOver_AlphaChannel * has_alpha;
        offsetX = offx;
        offsetY = offy;
        scaleWidth = pic->GetWidth();
        scaleHeight = pic->GetHeight();
        _sprnum = add_dynamic_sprite(std::move(pic), has_alpha);
    }
    MarkChanged();
}

void ScreenOverlay::SetSpriteNum(int sprnum, int offx, int offy)
{
    ResetImage();

    assert(sprnum >= 0 && sprnum < game.SpriteInfos.size());
    if (sprnum < 0 || sprnum >= game.SpriteInfos.size())
        return;

    _flags |= kOver_SpriteShared
           |  kOver_AlphaChannel * ((game.SpriteInfos[sprnum].Flags & SPF_ALPHACHANNEL) != 0);
    _sprnum = sprnum;
    offsetX = offx;
    offsetY = offy;
    scaleWidth = game.SpriteInfos[sprnum].Width;
    scaleHeight = game.SpriteInfos[sprnum].Height;
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
    bgSpeechForChar = in->ReadInt32();
    associatedOverlayHandle = in->ReadInt32();
    if (cmp_ver >= kOverSvgVersion_36025)
    {
        _flags = in->ReadInt16();
    }
    else
    {
        if (in->ReadBool()) // has alpha
            _flags |= kOver_AlphaChannel;
        if (!(in->ReadBool())) // screen relative position
            _flags |= kOver_PositionAtRoomXY;
    }

    if (cmp_ver < 0)
    {
        in->ReadInt16(); // alignment padding to int32 (finalize struct)
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
}

void ScreenOverlay::WriteToSavegame(Stream *out) const
{
    out->WriteInt32(0); // ddb 32-bit pointer value (nasty legacy format)
    out->WriteInt32(_sprnum); // sprite id
    out->WriteInt32(type);
    out->WriteInt32(x);
    out->WriteInt32(y);
    out->WriteInt32(timeout);
    out->WriteInt32(bgSpeechForChar);
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
}
