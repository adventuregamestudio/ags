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
#include "ac/screenoverlay.h"
#include "ac/spritecache.h"
#include "gfx/bitmap.h"
#include "util/stream.h"

using namespace AGS::Common;

Bitmap *ScreenOverlay::GetImage() const
{
    return IsSpriteReference() ?
        spriteset[_sprnum] :
        _pic.get();
}

void ScreenOverlay::SetImage(std::unique_ptr<Common::Bitmap> pic)
{
    _flags &= ~kOver_SpriteReference;
    _pic = std::move(pic);
    _sprnum = -1;
}

void ScreenOverlay::SetSpriteNum(int sprnum)
{
    _flags |= kOver_SpriteReference;
    _pic.reset();
    _sprnum = sprnum;
}

void ScreenOverlay::ReadFromFile(Stream *in, bool &has_bitmap, int32_t cmp_ver)
{
    _pic.reset();
    ddb = nullptr;
    in->ReadInt32(); // ddb 32-bit pointer value (nasty legacy format)
    int pic = in->ReadInt32();
    type = in->ReadInt32();
    x = in->ReadInt32();
    y = in->ReadInt32();
    timeout = in->ReadInt32();
    bgSpeechForChar = in->ReadInt32();
    associatedOverlayHandle = in->ReadInt32();
    if (cmp_ver >= 3)
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

    if (cmp_ver >= 1)
    {
        offsetX = in->ReadInt32();
        offsetY = in->ReadInt32();
    }
    if (cmp_ver >= 2)
    {
        zorder = in->ReadInt32();
        transparency = in->ReadInt32();
        scaleWidth = in->ReadInt32();
        scaleHeight = in->ReadInt32();
    }

    if (_flags & kOver_SpriteReference)
    {
        _sprnum = pic;
        has_bitmap = false;
    }
    else
    {
        _sprnum = -1;
        has_bitmap = pic != 0;
    }
}

void ScreenOverlay::WriteToFile(Stream *out) const
{
    out->WriteInt32(0); // ddb 32-bit pointer value (nasty legacy format)
    if (_flags & kOver_SpriteReference)
        out->WriteInt32(_sprnum); // sprite reference
    else
        out->WriteInt32(_pic ? 1 : 0); // has bitmap
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
