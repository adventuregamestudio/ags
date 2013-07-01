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
#include "gfx/bitmap.h"
#include "gui/guiobject.h"
#include "util/stream.h"

extern int all_buttons_disabled;

namespace AGS
{
namespace Common
{

GuiObject::GuiObject()
{
    Id          = 0;
    ParentId    = 0;
    Flags       = 0;
    ZOrder      = -1;
    IsActivated   = false;
}

bool GuiObject::IsDisabled() const
{
    if (Flags & kGuiCtrl_Disabled)
    {
        return true;
    }
    if (all_buttons_disabled)
    {
        return true;
    }
    return false;
}

bool GuiObject::IsOverControl(int x, int y, int leeway) const
{
    return Rect(Frame.Left - leeway, Frame.Top - leeway, Frame.Right + leeway, Frame.Bottom + leeway).
        IsInside(Point(x, y));
}

void GuiObject::SetFrame(const Rect &frame)
{
    Frame = frame;
}

void GuiObject::SetX(int x)
{
    Frame.MoveToX(x);
}

void GuiObject::SetY(int y)
{
    Frame.MoveToY(y);
}

void GuiObject::SetWidth(int width)
{
    Frame.SetWidth(width);
}

void GuiObject::SetHeight(int height)
{
    Frame.SetHeight(height);
}

void GuiObject::WriteToFile(Stream *out)
{
    out->WriteInt32(Flags);
    out->WriteInt32(Frame.Left);
    out->WriteInt32(Frame.Top);
    out->WriteInt32(Frame.GetWidth());
    out->WriteInt32(Frame.GetHeight());
    out->WriteInt32(ZOrder);
    out->WriteInt32(IsActivated ? 1 : 0);

    ScriptName.Write(out);
    out->WriteInt32(SupportedEventCount);
    for (int i = 0; i < SupportedEventCount; ++i)
    {
        EventHandlers[i].Write(out);
    }
}

void GuiObject::ReadFromFile(Stream *in, GuiVersion gui_version)
{
    Flags = in->ReadInt32();
    Frame.Left = in->ReadInt32();
    Frame.Top = in->ReadInt32();
    Frame.SetWidth(in->ReadInt32());
    Frame.SetHeight(in->ReadInt32());
    ZOrder = in->ReadInt32();
    IsActivated = in->ReadInt32() != 0;

    if (gui_version >= kGuiVersion_unkn_106)
    {
        ScriptName.Read(in, LEGACY_MAX_GUIOBJ_SCRIPTNAME_LEN);
    }
    else
    {
        ScriptName.Empty();
    }

    for (int i = 0; i < SupportedEventCount; ++i)
    {
        EventHandlers[i].Empty();
    }

    if (gui_version >= kGuiVersion_unkn_108)
    {
        int event_count = in->ReadInt32();
        if (event_count > SupportedEventCount)
        {
            quit("Error: too many control events, need newer version");
        }
        // read in the event handler names
        for (int i = 0; i < event_count; ++i)
        {
            EventHandlers[i].Read(in, LEGACY_MAX_GUIOBJ_EVENTHANDLER_LEN + 1);
        }
    }
}

void GuiObject::ReadFromSavedGame(Common::Stream *in, RuntimeGuiVersion gui_version)
{
    Flags = in->ReadInt32();
    Frame.Left = in->ReadInt32();
    Frame.Top = in->ReadInt32();
    Frame.SetWidth(in->ReadInt32());
    Frame.SetHeight(in->ReadInt32());
    ZOrder = in->ReadInt32();
    IsActivated = in->ReadBool();
}

void GuiObject::WriteToSavedGame(Common::Stream *out)
{
    out->WriteInt32(Flags);
    out->WriteInt32(Frame.Left);
    out->WriteInt32(Frame.Top);
    out->WriteInt32(Frame.GetWidth());
    out->WriteInt32(Frame.GetHeight());
    out->WriteInt32(ZOrder);
    out->WriteBool(IsActivated);
}

/* static */ Alignment GuiObject::ConvertLegacyAlignment(LegacyGuiAlignment legacy_align)
{
    switch (legacy_align)
    {
    case kLegacyGuiAlign_Left:
        return kAlignLeft;
    case kLegacyGuiAlign_Right:
        return kAlignRight;
    case kLegacyGuiAlign_Center:
        return kAlignHCenter;
    }
    return kAlignNone;
}

} // namespace Common
} // namespace AGS
