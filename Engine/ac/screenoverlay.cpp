//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
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
#include "ac/object.h"
#include "ac/spritecache.h"
#include "ac/viewframe.h"
#include "ac/dynobj/dynobj_manager.h"
#include "gfx/bitmap.h"
#include "util/stream.h"
#include "util/string_utils.h"

using namespace AGS::Common;

extern GameSetupStruct game;
extern SpriteCache spriteset;
extern std::vector<ViewStruct> views;

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

Bitmap *ScreenOverlay::GetImage() const
{
    return spriteset[_sprnum];
}

Size ScreenOverlay::GetGraphicSize() const
{
    return Size(game.SpriteInfos[_sprnum].Width, game.SpriteInfos[_sprnum].Height);
}

void ScreenOverlay::SetAutoPosition(int for_character, int x, int y)
{
    _flags |= kOver_AutoPosition;
    _speechForChar = for_character;
    _x = x;
    _y = y;
}

void ScreenOverlay::SetFixedPosition(int x, int y)
{
    _flags &= ~kOver_AutoPosition;
    _speechForChar = -1;
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

void ScreenOverlay::SetRotation(float rotation)
{
    _rotation = rotation;
    MarkChanged();
}

void ScreenOverlay::SetTransparency(int trans)
{
    _transparency = trans;
}

void ScreenOverlay::SetBlendMode(Common::BlendMode blend_mode)
{
    _blendMode = blend_mode;
}

void ScreenOverlay::SetZOrder(int zorder)
{
    _zorder = zorder;
}

void ScreenOverlay::ResetImage()
{
    if (_sprnum > 0 && !IsSpriteShared())
        free_dynamic_sprite(_sprnum, false);
    _sprnum = 0;
    _flags &= ~(kOver_SpriteShared);
    _scaledSize = {};
    _offx = _offy = 0;
}

void ScreenOverlay::SetImage(std::unique_ptr<Common::Bitmap> pic, int offx, int offy)
{
    ResetImage();

    if (pic)
    {
        _offx = offx;
        _offy = offy;
        _scaledSize = Size(pic->GetWidth(), pic->GetHeight());
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
    _offx = offx;
    _offy = offy;
    _scaledSize = Size(game.SpriteInfos[sprnum].Width, game.SpriteInfos[sprnum].Height);
    MarkChanged();
}

void ScreenOverlay::SetText(const String &text)
{
    _text = text;
}

void ScreenOverlay::MarkImageChanged()
{
    game_sprite_updated(_sprnum);
}

void ScreenOverlay::SetFlip(GraphicFlip flip)
{
    _spritetf = GfxDef::GetFlagsFromFlip(flip);
    MarkChanged();
}

void ScreenOverlay::SetTint(int red, int green, int blue, int opacity, int luminance)
{
    _tintR = red;
    _tintG = green;
    _tintB = blue;
    _tintLevel = opacity;
    _tintLight = luminance;
    _flags &= ~kOver_HasLightLevel;
    _flags |= kOver_HasTint;
    MarkChanged();
}

void ScreenOverlay::SetLightLevel(int light_level)
{
    _tintLevel = _tintR = _tintG = _tintB = 0;
    _tintLight = light_level;
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
    // release shader reference
    ccRemoveObjectHandle(GetShaderHandle());
    _shaderID = 0;
    _shaderHandle = 0;
    MarkChanged();
}

void ScreenOverlay::SetAsSpeech(int char_id, int timeout)
{
    _speechForChar = char_id;
    _timeout = timeout;
}

ScriptOverlay *ScreenOverlay::CreateScriptObject()
{
    ScriptOverlay *scover = new ScriptOverlay(_id);
    _scriptHandle = ccRegisterManagedObject(scover, scover);
    return scover;
}

void ScreenOverlay::AssignScriptObject(ScriptOverlay *scover)
{
    _scriptHandle = ccRegisterManagedObject(scover, scover);
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
        scover->Invalidate();
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
    RemoveShader();
    DisposeScriptObject();
    if (_removeCb)
        _removeCb(*this);
}

void ScreenOverlay::UpdateGraphicSpace()
{
    Point pos = Point(GetDrawX(), GetDrawY());
    Bitmap *pic = GetImage();
    _gs = GraphicSpace(
        pos.X, pos.Y, Pointf(0.f, 0.f), // origin
        Size(pic->GetWidth(), pic->GetHeight()), // source sprite size
        Size(_scaledSize.Width, _scaledSize.Height), // destination size (scaled)
        // real graphical aabb (maybe with extra offsets)
        RectWH(0, 0, pic->GetWidth(), pic->GetHeight()),
        _rotation // transforms
    );
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
    _speechForChar = in->ReadInt32();
    _scriptHandle = in->ReadInt32();
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
    if (cmp_ver >= kOverSvgVersion_36304)
    {
        _text = StrUtil::ReadString(in);
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
    
    if ((cmp_ver < kOverSvgVersion_36303) || ((cmp_ver >= kOverSvgVersion_400) && (cmp_ver < kOverSvgVersion_40024)))
    {
        _flags |= kOver_Visible;
    }

    if (cmp_ver >= kOverSvgVersion_400)
    {
        _blendMode = (BlendMode)in->ReadInt32();
        // Reserved for colour options
        in->ReadInt32(); // colour flags
        // tint rgb + s (4 uint8)
        _tintR = in->ReadInt8();
        _tintG = in->ReadInt8();
        _tintB = in->ReadInt8();
        _tintLevel = in->ReadInt8();
        _tintLight = in->ReadInt32(); // tint light (or light level)
        // Reserved for transform options
        _spritetf = (SpriteTransformFlags)in->ReadInt32(); // sprite transform flags1
        in->ReadInt32(); // sprite transform flags2
        in->ReadInt32(); // transform scale x
        in->ReadInt32(); // transform scale y
        in->ReadInt32(); // transform skew x
        in->ReadInt32(); // transform skew y
        _rotation = in->ReadFloat32(); // transform rotate
        in->ReadInt32(); // sprite pivot x
        in->ReadInt32(); // sprite pivot y
        in->ReadInt32(); // sprite anchor x
        in->ReadInt32(); // sprite anchor y
    }

    if (cmp_ver >= kOverSvgVersion_40018)
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
        if (!IsRoomLayer() && (_x == OVR_AUTOPLACE))
        {
            SetAutoPosition(_y); // character index was stored in y
        }
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
    out->WriteInt32(_speechForChar);
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
    // kOverSvgVersion_36304
    StrUtil::WriteString(_text, out);
    // since cmp_ver = 10
    out->WriteInt32(_blendMode);
    // Reserved for colour options
    out->WriteInt32(0); // colour flags
    // tint rgb + s (4 uint8)
    out->WriteInt8(_tintR);
    out->WriteInt8(_tintG);
    out->WriteInt8(_tintB);
    out->WriteInt8(_tintLevel);
    out->WriteInt32(_tintLight); // tint light (or light level)
    // Reserved for transform options
    out->WriteInt32(_spritetf); // sprite transform flags1
    out->WriteInt32(0); // sprite transform flags2
    out->WriteInt32(0); // transform scale x
    out->WriteInt32(0); // transform scale y
    out->WriteInt32(0); // transform skew x
    out->WriteInt32(0); // transform skew y
    out->WriteFloat32(_rotation); // transform rotate
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

const ViewFrame *AnimatedOverlay::GetViewFrame() const
{
    return &views[_view].loops[_loop].frames[_frame];
}

void AnimatedOverlay::Begin(int view, int loop, int frame, const ViewAnimateParams &params)
{
    _view = view;
    _loop = loop;
    _frame = SetFirstAnimFrame(view, loop, frame, params.InitialDirection);
    _anim = params;
    CheckViewFrame(_view, _loop, _frame, _anim.AudioVolume);
    _wait = _anim.Delay + views[_view].loops[_loop].frames[_frame].speed;
}

bool AnimatedOverlay::UpdateOnce()
{
    if (!_anim.IsValid())
        return false;

    if (_wait > 0)
    {
        _wait--;
        return true;
    }
    if (!CycleViewAnim(_view, _loop, _frame, _anim))
        return false;
    CheckViewFrame(_view, _loop, _frame, _anim.AudioVolume);
    _wait = _anim.Delay + views[_view].loops[_loop].frames[_frame].speed;
    return true;
}

void AnimatedOverlay::Reset()
{
    _view = -1;
    _loop = _frame = 0;
    _anim = {};
}

void AnimatedOverlay::ReadFromSavegame(Common::Stream *in, int cmp_ver)
{
    // Overlay's id and current view state
    _overid = in->ReadInt32();
    _flags = static_cast<uint32_t>(in->ReadInt32());
    _view = in->ReadInt16();
    _loop = in->ReadInt16();
    _frame = in->ReadInt16();

    // Animation properties and flow state
    AnimFlowStyle anim_flow = static_cast<AnimFlowStyle>(in->ReadInt8());
    AnimFlowDirection anim_dir_initial = static_cast<AnimFlowDirection>(in->ReadInt8());
    AnimFlowDirection anim_dir_current = static_cast<AnimFlowDirection>(in->ReadInt8());
    in->ReadInt8(); // reserved to fill int32
    int anim_delay = in->ReadInt16();
    int wait = in->ReadInt16();
    int anim_volume = in->ReadInt8();
    in->ReadInt8(); // reserved to fill int32
    in->ReadInt8();
    in->ReadInt8();

    _anim = ViewAnimateParams(anim_flow, anim_dir_initial, anim_dir_current, anim_delay, anim_volume);
}

void AnimatedOverlay::WriteToSavegame(Common::Stream *out) const
{
    out->WriteInt32(_overid);
    out->WriteInt32(_flags);
    out->WriteInt16(_view);
    out->WriteInt16(_loop);
    out->WriteInt16(_frame);
    out->WriteInt8(_anim.Flow);
    out->WriteInt8(_anim.InitialDirection);
    out->WriteInt8(_anim.Direction);
    out->WriteInt8(0); // reserved to fill int32
    out->WriteInt16(_anim.Delay);
    out->WriteInt16(_wait);
    out->WriteInt8(static_cast<uint8_t>(_anim.AudioVolume));
    out->WriteInt8(0); // reserved to fill int32
    out->WriteInt8(0);
    out->WriteInt8(0);
}
