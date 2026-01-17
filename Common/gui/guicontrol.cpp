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
#include "ac/common.h" // quit // FIXME: don't use quit here!!
#include "gui/guimain.h"
#include "gui/guicontrol.h"
#include "util/stream.h"

namespace AGS
{
namespace Common
{

GUIControl::GUIControl(const ScriptEventSchema *schema)
    : GUIObject(schema)
{
    UpdateControlRect();
}

void GUIControl::SetParentID(int parent_id)
{
    _parentID = parent_id;
}

bool GUIControl::IsOverControl(int x, int y, int leeway) const
{
    Point at = _gs.WorldToLocal(x, y);
    return IsOverControlImpl(at.X, at.Y, leeway);
}

bool GUIControl::IsOverControlImpl(int x, int y, int leeway) const
{
    return RectWH(-leeway, -leeway, _width + leeway, _height + leeway).IsInside(Point(x, y));
}

void GUIControl::SetClickable(bool on)
{
    if (on != ((_flags & kGUICtrl_Clickable) != 0))
    {
        _flags = (_flags & ~kGUICtrl_Clickable) | kGUICtrl_Clickable * on;
        MarkStateChanged(false, false); // update cursor-over-control only
    }
}

void GUIControl::SetEnabled(bool on)
{
    if (on != ((_flags & kGUICtrl_Enabled) != 0))
    {
        _flags = (_flags & ~kGUICtrl_Enabled) | kGUICtrl_Enabled * on;
        MarkStateChanged(true, true); // may change looks, and update cursor-over-control
    }
}

void GUIControl::SetTranslated(bool on)
{
    if (on != ((_flags & kGUICtrl_Translated) != 0))
    {
        _flags = (_flags & ~kGUICtrl_Translated) | kGUICtrl_Translated * on;
        MarkChanged();
    }
}

void GUIControl::SetVisible(bool on)
{
    if (on != ((_flags & kGUICtrl_Visible) != 0))
    {
        _flags = (_flags & ~kGUICtrl_Visible) | kGUICtrl_Visible * on;
        MarkStateChanged(false, true); // for software mode, and to update cursor-over-control
    }
}

void GUIControl::SetShowBorder(bool on)
{
    if (on != ((_flags & kGUICtrl_ShowBorder) != 0))
    {
        _flags = (_flags & ~kGUICtrl_ShowBorder) | kGUICtrl_ShowBorder * on;
        UpdateControlRect();
    }
}

void GUIControl::SetSolidBackground(bool on)
{
    if (on != ((_flags & kGUICtrl_SolidBack) != 0))
    {
        _flags = (_flags & ~kGUICtrl_SolidBack) | kGUICtrl_SolidBack * on;
        MarkChanged();
    }
}

void GUIControl::SetBackColor(int color)
{
    if (_backgroundColor != color)
    {
        _backgroundColor = color;
        OnColorsChanged();
    }
}

void GUIControl::SetBorderColor(int color)
{
    if (_borderColor != color)
    {
        _borderColor = color;
        OnColorsChanged();
    }
}

void GUIControl::SetBorderWidth(int border_width)
{
    border_width = std::max(0, border_width);
    if (_borderWidth != border_width)
    {
        _borderWidth = border_width;
        UpdateControlRect();
    }
}

void GUIControl::SetPaddingX(int padx)
{
    padx = std::max(0, padx);
    if (_paddingX != padx)
    {
        _paddingX = padx;
        UpdateControlRect();
    }
}

void GUIControl::SetPaddingY(int pady)
{
    pady = std::max(0, pady);
    if (_paddingY != pady)
    {
        _paddingY = pady;
        UpdateControlRect();
    }
}

void GUIControl::SetActivated(bool on)
{
    _isActivated = on;
}

void GUIControl::UpdateVisualState()
{
    UpdateControlRect();
    MarkPositionChanged(true, true);
}

void GUIControl::OnColorsChanged()
{
    MarkChanged();
}

void GUIControl::OnContentRectChanged()
{
    MarkChanged();
}

void GUIControl::OnResized()
{
    UpdateControlRect();
    UpdateGraphicSpace();
    MarkPositionChanged(true, false);
}

void GUIControl::DrawControlFrame(Bitmap *ds, int x, int y)
{
    const int bg_color = ds->GetCompatibleColor(_backgroundColor);
    const int border_color = ds->GetCompatibleColor(_borderColor);
    const int border_width = _borderWidth;

    if (IsSolidBackground())
    {
        ds->FillRect(RectWH(x, y, _width, _height), bg_color);
    }

    if (IsShowBorder())
    {
        for (int i = 0; i < border_width; ++i)
        {
            ds->DrawRect(RectWH(x + i, y + i, _width - i * 2, _height - i * 2), border_color);
        }
    }
}

void GUIControl::UpdateControlRect()
{
    if (IsShowBorder())
        _innerRect = RectWH(_borderWidth + _paddingX, _borderWidth + _paddingY,
                            _width - _borderWidth * 2 - _paddingX * 2, _height - _borderWidth * 2 - _paddingY * 2);
    else
        _innerRect = RectWH(_paddingX, _paddingY,
                            _width - _paddingX * 2, _height - _paddingY * 2);

    OnContentRectChanged();
}

// TODO: replace string serialization with StrUtil::ReadString and WriteString
// methods in the future, to keep this organized.
void GUIControl::WriteToFile(Stream *out) const
{
    out->WriteInt32(_flags);
    out->WriteInt32(_x);
    out->WriteInt32(_y);
    out->WriteInt32(_width);
    out->WriteInt32(_height);
    out->WriteInt32(_zOrder);
    _name.Write(out);
    out->WriteInt32(0); // obsolete (old-style event table)
}

void GUIControl::ReadFromFile(Stream *in, GuiVersion gui_version)
{
    _flags    = in->ReadInt32();
    _x        = in->ReadInt32();
    _y        = in->ReadInt32();
    _width    = in->ReadInt32();
    _height   = in->ReadInt32();
    _zOrder   = in->ReadInt32();
    _name.Read(in);

    // Old-style events table (will be converted to new-style)
    uint32_t evt_count = static_cast<uint32_t>(in->ReadInt32());
    for (uint32_t i = 0; i < evt_count; ++i)
    {
        _events.SetHandler(i, String::FromStream(in));
    }

    //UpdateGraphicSpace(); // can't do here, because sprite infos may not be loaded yet
    UpdateControlRect();
}

void GUIControl::ReadFromFile_Ext363(Common::Stream *in, GuiVersion gui_version)
{
    _backgroundColor = in->ReadInt32();
    _borderColor = in->ReadInt32();
    _borderWidth = in->ReadInt32();
    _paddingX = in->ReadInt32();
    _paddingY = in->ReadInt32();
    in->ReadInt32(); // reserved
    in->ReadInt32();
    in->ReadInt32();
    in->ReadInt32();

    UpdateControlRect();
}

void GUIControl::ReadFromSavegame(Stream *in, GuiSvgVersion svg_ver)
{
    // Properties
    _flags = in->ReadInt32();
    _x = in->ReadInt32();
    _y = in->ReadInt32();
    _width = in->ReadInt32();
    _height = in->ReadInt32();
    _zOrder = in->ReadInt32();
    // Dynamic state
    _isActivated = in->ReadBool() ? 1 : 0;

    if (svg_ver >= kGuiSvgVersion_36023)
    {
        _transparency = in->ReadInt32();
        // valid since kGuiSvgVersion_36304
        _backgroundColor = in->ReadInt32();
        _borderColor = in->ReadInt32();
        _borderWidth = in->ReadInt32();
    }
    if ((svg_ver >= kGuiSvgVersion_36304) && (svg_ver < kGuiSvgVersion_400) ||
        (svg_ver >= kGuiSvgVersion_40026))
    {
        _paddingX = in->ReadInt32();
        _paddingY = in->ReadInt32();
        in->ReadInt32(); // reserved
        in->ReadInt32();
        in->ReadInt32();
        in->ReadInt32();
    }
    // NOTE: bw-compat frame properties have to be assigned by each control type separately

    if (svg_ver >= kGuiSvgVersion_40016)
    {
        _blendMode = static_cast<BlendMode>(in->ReadInt32());
        // Reserved for colour options
        in->ReadInt32(); // colour flags
        in->ReadInt32(); // tint rgb + s
        in->ReadInt32(); // tint light (or light level)
        // Reserved for transform options
        in->ReadInt32(); // sprite transform flags1
        in->ReadInt32(); // sprite transform flags2
        _scale.X = in->ReadInt32(); // transform scale x
        _scale.Y = in->ReadInt32(); // transform scale y
        in->ReadInt32(); // transform skew x
        in->ReadInt32(); // transform skew y
        _rotation = in->ReadInt32(); // transform rotate
        in->ReadInt32(); // sprite pivot x
        in->ReadInt32(); // sprite pivot y
        in->ReadInt32(); // sprite anchor x
        in->ReadInt32(); // sprite anchor y
    }
    else
    {
        _rotation = 0.f;
        _scale = Point(1.f, 1.f);
    }

    if (svg_ver >= kGuiSvgVersion_40018)
    {
        _shaderID = in->ReadInt32();
        _shaderHandle = in->ReadInt32();
        in->ReadInt32(); // reserved
        in->ReadInt32();
    }
    else
    {
        _shaderID = 0;
        _shaderHandle = 0;
    }

    UpdateControlRect();
    UpdateGraphicSpace();
    MarkChanged();
}

void GUIControl::WriteToSavegame(Stream *out) const
{
    // Properties
    out->WriteInt32(_flags);
    out->WriteInt32(_x);
    out->WriteInt32(_y);
    out->WriteInt32(_width);
    out->WriteInt32(_height);
    out->WriteInt32(_zOrder);
    // Dynamic state
    out->WriteBool(_isActivated != 0);
    // kGuiSvgVersion_36023
    out->WriteInt32(_transparency);
    // valid since kGuiSvgVersion_36304
    out->WriteInt32(_backgroundColor);
    out->WriteInt32(_borderColor);
    out->WriteInt32(_borderWidth);
    // kGuiSvgVersion_36304
    out->WriteInt32(_paddingX);
    out->WriteInt32(_paddingY);
    out->WriteInt32(0); // reserved
    out->WriteInt32(0);
    out->WriteInt32(0);
    out->WriteInt32(0);
    // kGuiSvgVersion_40016
    out->WriteInt32(_blendMode);
    // Reserved for colour options
    out->WriteInt32(0); // colour flags
    out->WriteInt32(0); // tint rgb + s
    out->WriteInt32(0); // tint light (or light level)
    // Reserved for transform options
    out->WriteInt32(0); // sprite transform flags1
    out->WriteInt32(0); // sprite transform flags2
    out->WriteInt32(_scale.X); // transform scale x
    out->WriteInt32(_scale.Y); // transform scale y
    out->WriteInt32(0); // transform skew x
    out->WriteInt32(0); // transform skew y
    out->WriteInt32(_rotation); // transform rotate
    out->WriteInt32(0); // sprite pivot x
    out->WriteInt32(0); // sprite pivot y
    out->WriteInt32(0); // sprite anchor x
    out->WriteInt32(0); // sprite anchor y
    // kGuiSvgVersion_40018
    out->WriteInt32(_shaderID);
    out->WriteInt32(_shaderHandle);
    out->WriteInt32(0); // reserved
    out->WriteInt32(0);
}

} // namespace Common
} // namespace AGS
