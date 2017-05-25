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

#include "ac/common.h"
#include "gui/guiobject.h"
#include "gui/guimain.h"
#include "util/stream.h"

namespace AGS
{
namespace Common
{

GUIObject::GUIObject()
{
    Id          = 0;
    ParentId    = 0;
    Flags       = 0;
    X           = 0;
    Y           = 0;
    Width       = 0;
    Height      = 0;
    ZOrder      = -1;
    IsActivated    = false;
}

int GUIObject::GetEventCount() const
{
    return _scEventCount;
}

String GUIObject::GetEventName(int event) const
{
    if (event < 0 || event >= _scEventCount)
        return "";
    return _scEventNames[event];
}

String GUIObject::GetEventArgs(int event) const
{
    if (event < 0 || event >= _scEventCount)
        return "";
    return _scEventArgs[event];
}

bool GUIObject::IsEnabled() const
{
    // TODO: a global variable should not be checked by control
    return !((Flags & kGUICtrl_Disabled) || all_buttons_disabled);
}

bool GUIObject::IsOverControl(int x, int y, int leeway) const
{
    return x >= X && y >= Y && x < (X + Width + leeway) && y < (Y + Height + leeway);
}

void GUIObject::Disable()
{
    Flags |= kGUICtrl_Disabled;
}

void GUIObject::Enable()
{
    Flags &= ~kGUICtrl_Disabled;
}

void GUIObject::Hide()
{
    Flags |= kGUICtrl_Invisible;
}

void GUIObject::SetClickable(bool clickable)
{
    if (clickable)
        Flags &= ~kGUICtrl_NoClicks;
    else
        Flags |= kGUICtrl_NoClicks;
}

void GUIObject::Show()
{
    Flags &= ~kGUICtrl_Invisible;
}

// TODO: replace string serialization with StrUtil::ReadString and WriteString
// methods in the future, to keep this organized.
void GUIObject::WriteToFile(Stream *out)
{
    out->WriteInt32(Flags);
    out->WriteInt32(X);
    out->WriteInt32(Y);
    out->WriteInt32(Width);
    out->WriteInt32(Height);
    out->WriteInt32(ZOrder);
    out->WriteInt32(IsActivated);
    Name.Write(out);
    out->WriteInt32(_scEventCount);
    for (int i = 0; i < _scEventCount; ++i)
        EventHandlers[i].Write(out);
}

void GUIObject::ReadFromFile(Stream *in, GuiVersion gui_version)
{
    Flags    = in->ReadInt32();
    X        = in->ReadInt32();
    Y        = in->ReadInt32();
    Width    = in->ReadInt32();
    Height   = in->ReadInt32();
    ZOrder   = in->ReadInt32();
    IsActivated = in->ReadInt32() != 0;

    if (gui_version >= kGuiVersion_unkn_106)
        Name.Read(in);
    else
        Name.Free();

    for (int i = 0; i < _scEventCount; ++i)
    {
        EventHandlers[i].Free();
    }

    if (gui_version >= kGuiVersion_unkn_108)
    {
        int evt_count = in->ReadInt32();
        if (evt_count > _scEventCount)
            quit("Error: too many control events, need newer version");
        for (int i = 0; i < evt_count; ++i)
        {
            EventHandlers[i].Read(in);
        }
    }
}


FrameAlignment ConvertLegacyGUIAlignment(int32_t align)
{
    switch (align)
    {
    case kLegacyGUIAlign_Left:
        return kAlignLeft;
    case kLegacyGUIAlign_Right:
        return kAlignRight;
    case kLegacyGUIAlign_Center:
        return kAlignHCenter;
    }
    return kAlignNone;
}

} // namespace Common
} // namespace AGS
