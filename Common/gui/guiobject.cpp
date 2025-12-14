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

#include "ac/common.h" // quit
#include "gui/guimain.h"
#include "gui/guiobject.h"
#include "util/stream.h"

namespace AGS
{
namespace Common
{

void GUIObject::SetName(const String &name)
{
    _name = name;
}

void GUIObject::SetID(int id)
{
    _id = id;
}

void GUIObject::SetParentID(int parent_id)
{
    _parentID = parent_id;
}

uint32_t GUIObject::GetEventCount() const
{
    return 0;
}

String GUIObject::GetEventName(uint32_t event) const
{
    return "";
}

String GUIObject::GetEventArgs(uint32_t event) const
{
    return "";
}

String GUIObject::GetEventHandler(uint32_t event) const
{
    if (event >= _eventHandlers.size())
        return "";
    return _eventHandlers[event];
}

void GUIObject::SetEventHandler(uint32_t event, const String &fn_name)
{
    if (event >= _eventHandlers.size())
        return;
    _eventHandlers[event] = fn_name;
}

bool GUIObject::IsOverControl(int x, int y, int leeway) const
{
    return x >= _x && y >= _y && x < (_x + _width + leeway) && y < (_y + _height + leeway);
}

void GUIObject::SetClickable(bool on)
{
    if (on != ((_flags & kGUICtrl_Clickable) != 0))
    {
        _flags = (_flags & ~kGUICtrl_Clickable) | kGUICtrl_Clickable * on;
        MarkStateChanged(false, false); // update cursor-over-control only
    }
}

void GUIObject::SetEnabled(bool on)
{
    if (on != ((_flags & kGUICtrl_Enabled) != 0))
    {
        _flags = (_flags & ~kGUICtrl_Enabled) | kGUICtrl_Enabled * on;
        MarkStateChanged(true, true); // may change looks, and update cursor-over-control
    }
}

void GUIObject::SetX(int x)
{
    SetPosition(x, _y);
}

void GUIObject::SetY(int y)
{
    SetPosition(_x, y);
}

void GUIObject::SetPosition(int x, int y)
{
    if (_x != x || _y != y)
    {
        _x = x;
        _y = y;
        MarkPositionChanged(false);
    }
}

void GUIObject::SetWidth(int width)
{
    SetSize(width, _height);
}

void GUIObject::SetHeight(int height)
{
    SetSize(_width, height);
}

void GUIObject::SetSize(int width, int height)
{
    if (_width != width || _height != height)
    {
        _width = width;
        _height = height;
        OnResized();
    }
}

void GUIObject::SetZOrder(int zorder)
{
    _zOrder = zorder;
}

void GUIObject::SetTranslated(bool on)
{
    if (on != ((_flags & kGUICtrl_Translated) != 0))
    {
        _flags = (_flags & ~kGUICtrl_Translated) | kGUICtrl_Translated * on;
        MarkChanged();
    }
}

void GUIObject::SetTransparency(int trans)
{
    if (_transparency != trans)
    {
        _transparency = trans;
        MarkParentChanged(); // for software mode
    }
}

void GUIObject::SetVisible(bool on)
{
    if (on != ((_flags & kGUICtrl_Visible) != 0))
    {
        _flags = (_flags & ~kGUICtrl_Visible) | kGUICtrl_Visible * on;
        MarkStateChanged(false, true); // for software mode, and to update cursor-over-control
    }
}

void GUIObject::SetShowBorder(bool on)
{
    if (on != ((_flags & kGUICtrl_ShowBorder) != 0))
    {
        _flags = (_flags & ~kGUICtrl_ShowBorder) | kGUICtrl_ShowBorder * on;
        MarkChanged();
    }
}

void GUIObject::SetSolidBackground(bool on)
{
    if (on != ((_flags & kGUICtrl_SolidBack) != 0))
    {
        _flags = (_flags & ~kGUICtrl_SolidBack) | kGUICtrl_SolidBack * on;
        MarkChanged();
    }
}

void GUIObject::SetBackColor(int color)
{
    if (_backgroundColor != color)
    {
        _backgroundColor = color;
        OnColorsChanged();
    }
}

void GUIObject::SetBorderColor(int color)
{
    if (_borderColor != color)
    {
        _borderColor = color;
        OnColorsChanged();
    }
}

void GUIObject::SetBorderWidth(int border_width)
{
    if (_borderWidth != border_width)
    {
        _borderWidth = border_width;
        OnColorsChanged();
    }
}

void GUIObject::SetPaddingX(int padx)
{
    if (_paddingX != padx)
    {
        _paddingX = padx;
        MarkChanged();
    }
}

void GUIObject::SetPaddingY(int pady)
{
    if (_paddingY != pady)
    {
        _paddingY = pady;
        MarkChanged();
    }
}

void GUIObject::SetActivated(bool on)
{
    _isActivated = on;
}

void GUIObject::OnColorsChanged()
{
    MarkChanged();
}

void GUIObject::DrawControlFrame(Bitmap *ds, int x, int y)
{
    const int bg_color = ds->GetCompatibleColor(_backgroundColor);
    const int border_color = ds->GetCompatibleColor(_borderColor);
    const int border_width = _borderWidth *
        // Pre-3.6.3 the controls would implicitly double the border thickness
        // for "hi-res" games
        // FIXME: adjust and save border width when it's assigned instead
        (((GUI::GameGuiVersion < kGuiVersion_363) && (get_fixed_pixel_size(1) > 1)) ? 2 : 1);

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

// TODO: replace string serialization with StrUtil::ReadString and WriteString
// methods in the future, to keep this organized.
void GUIObject::WriteToFile(Stream *out) const
{
    out->WriteInt32(_flags);
    out->WriteInt32(_x);
    out->WriteInt32(_y);
    out->WriteInt32(_width);
    out->WriteInt32(_height);
    out->WriteInt32(_zOrder);
    _name.Write(out);
    out->WriteInt32(static_cast<uint32_t>(_eventHandlers.size()));
    for (uint32_t i = 0; i < _eventHandlers.size(); ++i)
        _eventHandlers[i].Write(out);
}

void GUIObject::ReadFromFile(Stream *in, GuiVersion gui_version)
{
    _flags    = in->ReadInt32();
    // reverse particular flags from older format
    if (gui_version < kGuiVersion_350)
        _flags ^= kGUICtrl_OldFmtXorMask;
    _x        = in->ReadInt32();
    _y        = in->ReadInt32();
    _width    = in->ReadInt32();
    _height   = in->ReadInt32();
    _zOrder   = in->ReadInt32();
    if (gui_version < kGuiVersion_350)
    { // NOTE: reading into actual variables only for old savegame support
        _isActivated = in->ReadInt32() != 0;
    }

    if (gui_version >= kGuiVersion_unkn_106)
        _name.Read(in);
    else
        _name.Free();

    _eventHandlers.clear();

    if (gui_version >= kGuiVersion_unkn_108)
    {
        uint32_t evt_count = static_cast<uint32_t>(in->ReadInt32());
        _eventHandlers.resize(evt_count);
        for (uint32_t i = 0; i < evt_count; ++i)
        {
            _eventHandlers[i].Read(in);
        }
    }
}

void GUIObject::ReadFromSavegame(Stream *in, GuiSvgVersion svg_ver)
{
    // Properties
    _flags = in->ReadInt32();
    // reverse particular flags from older format
    if (svg_ver < kGuiSvgVersion_350)
        _flags ^= kGUICtrl_OldFmtXorMask;
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
        in->ReadInt32(); // reserve 3 ints
        in->ReadInt32();
        in->ReadInt32();
    }
}

void GUIObject::WriteToSavegame(Stream *out) const
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
    out->WriteInt32(_transparency);
    out->WriteInt32(0); // reserve 3 ints
    out->WriteInt32(0);
    out->WriteInt32(0);
}


HorAlignment ConvertLegacyGUIAlignment(LegacyGUIAlignment align)
{
    switch (align)
    {
    case kLegacyGUIAlign_Right:
        return kHAlignRight;
    case kLegacyGUIAlign_Center:
        return kHAlignCenter;
    default:
        return kHAlignLeft;
    }
}

LegacyGUIAlignment GetLegacyGUIAlignment(HorAlignment align)
{
    switch (align)
    {
    case kHAlignRight:
        return kLegacyGUIAlign_Right;
    case kHAlignCenter:
        return kLegacyGUIAlign_Center;
    default:
        return kLegacyGUIAlign_Left;
    }
}

} // namespace Common
} // namespace AGS
