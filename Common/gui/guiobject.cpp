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

String GUIObject::GetEventName(uint32_t event) const
{
    if (event < 0 || event >= _scEventCount)
        return "";
    return _scEventNames[event];
}

String GUIObject::GetEventArgs(uint32_t event) const
{
    if (event < 0 || event >= _scEventCount)
        return "";
    return _scEventArgs[event];
}

String GUIObject::GetEventHandler(uint32_t event) const
{
    if (event < 0 || event >= _scEventCount)
        return "";
    return _eventHandlers[event];
}

void GUIObject::SetEventHandler(uint32_t event, const String &fn_name)
{
    if (event < 0 || event >= _scEventCount)
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

void GUIObject::SetTransparencyAsPercentage(int percent)
{
    SetTransparency(GfxDef::Trans100ToLegacyTrans255(percent));
}

void GUIObject::SetBlendMode(BlendMode blend_mode)
{
    if (_blendMode != blend_mode)
    {
        _blendMode = blend_mode;
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

void GUIObject::SetActivated(bool on)
{
    _isActivated = on;
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
    out->WriteInt32(_scEventCount);
    for (uint32_t i = 0; i < _scEventCount; ++i)
        _eventHandlers[i].Write(out);
}

void GUIObject::ReadFromFile(Stream *in, GuiVersion gui_version)
{
    _flags    = in->ReadInt32();
    _x        = in->ReadInt32();
    _y        = in->ReadInt32();
    _width    = in->ReadInt32();
    _height   = in->ReadInt32();
    _zOrder   = in->ReadInt32();
    _name.Read(in);

    for (uint32_t i = 0; i < _scEventCount; ++i)
    {
        _eventHandlers[i].Free();
    }

    uint32_t evt_count = static_cast<uint32_t>(in->ReadInt32());
    // FIXME: don't use quit here!!
    if (evt_count > _scEventCount)
        quit("Error: too many control events, need newer version");
    for (uint32_t i = 0; i < evt_count; ++i)
    {
        _eventHandlers[i].Read(in);
    }

    MarkChanged();
}

void GUIObject::ReadFromSavegame(Stream *in, GuiSvgVersion svg_ver)
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
        in->ReadInt32(); // reserve up to 4 ints
        in->ReadInt32();
        in->ReadInt32();
    }
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
        in->ReadInt32(); // transform scale x
        in->ReadInt32(); // transform scale y
        in->ReadInt32(); // transform skew x
        in->ReadInt32(); // transform skew y
        in->ReadInt32(); // transform rotate
        in->ReadInt32(); // sprite pivot x
        in->ReadInt32(); // sprite pivot y
        in->ReadInt32(); // sprite anchor x
        in->ReadInt32(); // sprite anchor y
    }

    MarkChanged();
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
    // kGuiSvgVersion_36023
    out->WriteInt32(_transparency);
    out->WriteInt32(0); // reserve up to 4 ints
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
    out->WriteInt32(0); // transform scale x
    out->WriteInt32(0); // transform scale y
    out->WriteInt32(0); // transform skew x
    out->WriteInt32(0); // transform skew y
    out->WriteInt32(0); // transform rotate
    out->WriteInt32(0); // sprite pivot x
    out->WriteInt32(0); // sprite pivot y
    out->WriteInt32(0); // sprite anchor x
    out->WriteInt32(0); // sprite anchor y
}

} // namespace Common
} // namespace AGS
