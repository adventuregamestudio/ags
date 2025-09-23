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

void GUIControl::SetParentID(int parent_id)
{
    _parentID = parent_id;
}

uint32_t GUIControl::GetEventCount() const
{
    return 0;
}

String GUIControl::GetEventName(uint32_t event) const
{
    return "";
}

String GUIControl::GetEventArgs(uint32_t event) const
{
    return "";
}

String GUIControl::GetEventHandler(uint32_t event) const
{
    if (event >= _eventHandlers.size())
        return "";
    return _eventHandlers[event];
}

void GUIControl::SetEventHandler(uint32_t event, const String &fn_name)
{
    if (event >= _eventHandlers.size())
        return;
    _eventHandlers[event] = fn_name;
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

void GUIControl::SetActivated(bool on)
{
    _isActivated = on;
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
    out->WriteInt32(_eventHandlers.size());
    for (uint32_t i = 0; i < _eventHandlers.size(); ++i)
        _eventHandlers[i].Write(out);
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

    _eventHandlers.clear();

    uint32_t evt_count = static_cast<uint32_t>(in->ReadInt32());
    _eventHandlers.resize(evt_count);
    for (uint32_t i = 0; i < evt_count; ++i)
    {
        _eventHandlers[i].Read(in);
    }

    //UpdateGraphicSpace(); // can't do here, because sprite infos may not be loaded yet
    MarkChanged();
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
