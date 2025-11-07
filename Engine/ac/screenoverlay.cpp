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
#include "ac/game.h"
#include "ac/gamesetupstruct.h"
#include "ac/spritecache.h"
#include "ac/dynobj/dynobj_manager.h"
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
    _flags &= ~(kOver_SpriteShared | kOver_AlphaChannel);
    _scaledSize = {};
    _offx = _offy = 0;
}

Bitmap *ScreenOverlay::GetImage() const
{
    return spriteset[_sprnum];
}

Size ScreenOverlay::GetGraphicSize() const
{
    return Size(game.SpriteInfos[_sprnum].Width, game.SpriteInfos[_sprnum].Height);
}

void ScreenOverlay::SetPosition(int x, int y)
{
    _x = x;
    _y = y;
}

void ScreenOverlay::SetScaledSize(int w, int h)
{
    if ((w == _scaledSize.Width) && (h == _scaledSize.Height))
        return;
    _scaledSize = Size(w, h);
    MarkChanged();
}

void ScreenOverlay::SetTransparency(int trans)
{
    _transparency = trans;
}

void ScreenOverlay::SetZOrder(int zorder)
{
    _zorder = zorder;
}

void ScreenOverlay::SetImage(std::unique_ptr<Common::Bitmap> pic, bool has_alpha, int offx, int offy)
{
    ResetImage();

    if (pic)
    {
        _flags |= kOver_AlphaChannel * has_alpha;
        _offx = offx;
        _offy = offy;
        _scaledSize = Size(pic->GetWidth(), pic->GetHeight());
        _sprnum = add_dynamic_sprite(std::move(pic), has_alpha, SPF_OBJECTOWNED);
    }
    MarkChanged();
}

void ScreenOverlay::MarkImageChanged()
{
    game_sprite_updated(_sprnum);
}

void ScreenOverlay::SetSpriteNum(int sprnum, int offx, int offy)
{
    ResetImage();

    assert(sprnum >= 0 && static_cast<size_t>(sprnum) < game.SpriteInfos.size());
    if (sprnum < 0 || static_cast<size_t>(sprnum) >= game.SpriteInfos.size())
        return;

    _flags |= kOver_SpriteShared
           |  kOver_AlphaChannel * ((game.SpriteInfos[sprnum].Flags & SPF_ALPHACHANNEL) != 0);
    _sprnum = sprnum;
    _offx = offx;
    _offy = offy;
    _scaledSize = Size(game.SpriteInfos[sprnum].Width, game.SpriteInfos[sprnum].Height);
    MarkChanged();
}

void ScreenOverlay::SetAsBackgroundSpeech(int char_id, int timeout)
{
    _bgSpeechForChar = char_id;
    _timeout = timeout;
}

ScriptOverlay *ScreenOverlay::CreateScriptObject()
{
    ScriptOverlay *scover = new ScriptOverlay();
    scover->overlayId = _id;
    _scriptHandle = ccRegisterManagedObject(scover, scover);
    return scover;
}

void ScreenOverlay::DetachScriptObject()
{
    _scriptHandle = 0;
}

void ScreenOverlay::DisposeScriptObject()
{
    if (_scriptHandle <= 0)
        return; // invalid handle

    ScriptOverlay *scover = (ScriptOverlay *)ccGetObjectAddressFromHandle(_scriptHandle);
    if (scover)
    {
        // Invalidate script object: this is required in case the object will
        // remain in script mem after disconnecting overlay from it
        scover->overlayId = -1;
        ccAttemptDisposeObject(_scriptHandle);
        _scriptHandle = 0;
    }
}

void ScreenOverlay::SetRemoveCallback(PfnOverlayRemoved remove_cb)
{
    _removeCb = remove_cb;
}

void ScreenOverlay::OnRemove()
{
    SetImage(nullptr);
    DisposeScriptObject();
    if (_removeCb)
        _removeCb(*this);
}

int ScreenOverlay::UpdateTimeout()
{
    if (_timeout > 0)
        _timeout--;
    return _timeout;
}

void ScreenOverlay::ReadFromSavegame(Stream *in, bool &has_bitmap, int32_t cmp_ver)
{
    ResetImage();

    in->ReadInt32(); // ddb 32-bit pointer value (nasty legacy format)
    int pic = in->ReadInt32();
    _id = in->ReadInt32();
    _x = in->ReadInt32();
    _y = in->ReadInt32();
    _timeout = in->ReadInt32();
    _bgSpeechForChar = in->ReadInt32();
    _scriptHandle = in->ReadInt32();
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
        _offx = in->ReadInt32();
        _offy = in->ReadInt32();
    }
    if (cmp_ver >= kOverSvgVersion_36008)
    {
        _zorder = in->ReadInt32();
        _transparency = in->ReadInt32();
        _scaledSize.Width = in->ReadInt32();
        _scaledSize.Height = in->ReadInt32();
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

    if (cmp_ver < kOverSvgVersion_36303)
    {
        _flags |= kOver_Visible;
    }
}

void ScreenOverlay::WriteToSavegame(Stream *out) const
{
    out->WriteInt32(0); // ddb 32-bit pointer value (nasty legacy format)
    out->WriteInt32(_sprnum); // sprite id
    out->WriteInt32(_id);
    out->WriteInt32(_x);
    out->WriteInt32(_y);
    out->WriteInt32(_timeout);
    out->WriteInt32(_bgSpeechForChar);
    out->WriteInt32(_scriptHandle);
    out->WriteInt16(_flags);
    // since cmp_ver = 1
    out->WriteInt32(_offx);
    out->WriteInt32(_offy);
    // since cmp_ver = 2
    out->WriteInt32(_zorder);
    out->WriteInt32(_transparency);
    out->WriteInt32(_scaledSize.Width);
    out->WriteInt32(_scaledSize.Height);
}
