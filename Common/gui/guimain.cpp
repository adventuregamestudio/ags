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

#include <ctype.h>
#include "ac/common.h"
#include "ac/spritecache.h"
#include "gui/guibutton.h"
#include "gui/guiinv.h"
#include "gui/guilabel.h"
#include "gui/guilistbox.h"
#include "gui/guimain.h"
#include "gui/guiobject.h"
#include "gui/guislider.h"
#include "gui/guitextbox.h"

#define MOVER_MOUSEDOWNLOCKED -4000
extern SpriteCache spriteset;

GuiVersion LoadedGuiVersion;

namespace AGS
{
namespace Common
{

GuiMain::GuiMain()
{
    Init();
}

/* static */ String GuiMain::FixupGuiName(const String &name)
{
    if (name.GetLength() > 0 && name[0] != 'g')
    {
        String lower_name = name.Mid(1);
        lower_name.MakeLower();
        String fixed_name = "g";
        fixed_name.AppendChar(name[0]);
        fixed_name.Append(lower_name);
        return fixed_name;
    }
    return name;
}

/* static */ String GuiMain::MakeScriptName(const String &name)
{
    if (name.IsEmpty())
    {
        return name;
    }
    else
    {
        String script_name = String::FromFormat("g%s", name);
        script_name.MakeLower();
        script_name.SetAt(1, toupper(script_name[1]));
        return script_name;
    }
}

void GuiMain::Init()
{
    Id                  = 0;
    Name.Empty();
    Flags               = 0;
    BackgroundColor     = 8;
    BackgroundImage     = 0;
    ForegroundColor     = 1;
    Transparency        = 0;
    PopupStyle          = kGuiPopupNone;
    PopupAtMouseY       = -1;

    _visibility         = kGuiVisibility_On;
    ZOrder              = -1;
    FocusedControl      = 0;
    HighlightControl    = -1;
    MouseOverControl    = -1;
    MouseDownControl    = -1;
    MouseWasAt.X        = -1;
    MouseWasAt.Y        = -1;

    ControlCount        = 0;

    OnClickHandler.Empty();
}

int GuiMain::FindControlUnderMouse() const
{
  return FindControlUnderMouse(0, true);
}

// this version allows some extra leeway in the Editor so that
// the user can grab tiny controls
int GuiMain::FindControlUnderMouse(int leeway) const
{
    return FindControlUnderMouse(leeway, true);
}

int GuiMain::FindControlUnderMouse(int leeway, bool must_be_clickable) const
{
    if (loaded_game_file_version <= kGameVersion_262)
    {
        // Ignore draw order IsVisible 2.6.2 and lower
        for (int i = 0; i < ControlCount; ++i)
        {
            const int ctrl_index = i;
            const GuiObject *const control = Controls[ctrl_index];
            if (!control->IsVisible())
            {
                continue;
            }
            if ((!control->IsClickable()) && (must_be_clickable))
            {
                continue;
            }
            if (control->IsOverControl(mousex, mousey, leeway))
            {
                return ctrl_index;
            }
        }
    }
    else
    {
        for (int i = ControlCount - 1; i >= 0; --i)
        {
            const int ctrl_index = ControlDrawOrder[i];
            const GuiObject * const control = Controls[ctrl_index];
            if (!control->IsVisible())
            {
                continue;
            }
            if ((!control->IsClickable()) && (must_be_clickable))
            {
                continue;
            }
            if (control->IsOverControl(mousex, mousey, leeway))
            {
                return ctrl_index;
            }
        }
    }

    return -1;
}

GuiControlType GuiMain::GetControlType(int index) const
{
    if ((index < 0) || (index >= ControlCount))
    {
        return kGuiControlUndefined;
    }
    return (GuiControlType)((ControlRefs[index] >> 16) & 0xFFFF);
}

bool GuiMain::HasAlphaChannel() const
{
    if (BackgroundImage > 0)
    {
        // alpha state depends IsVisible background image
        return is_sprite_alpha(BackgroundImage);
    }
    if (BackgroundColor > 0)
    {
        // not alpha transparent if there is a background color
        return false;
    }
    // transparent background, enable alpha blending
    return (final_col_dep >= 24);
}

bool GuiMain::IsMouseOnGui() const
{
    if (!IsVisible())
    {
        return false;
    }
    if (Flags & kGuiMain_NoClick)
    {
        return false;
    }
    if (Frame.IsInside(Point(mousex, mousey)))
    {
        return true;
    }
    return false;
}

bool GuiMain::IsTextWindow() const
{
    return (Flags & kGuiMain_TextWindow) != 0;
}

bool GuiMain::BringControlToFront(int index)
{
    if (index < 0)
    {
        return false;
    }

    GuiObject *const control = Controls[index];
    if (control->ZOrder < ControlCount - 1)
    {
        int old_order = control->ZOrder;
        for (int i = 0; i < ControlCount; ++i)
        {
            if (Controls[i]->ZOrder > old_order)
            {
                Controls[i]->ZOrder--;
            }
        }
        control->ZOrder = ControlCount - 1;
        ResortZOrder();
        OnControlPositionChanged();
        return true;
    }
    return false;
}

void GuiMain::RebuildArray()
{
    Controls.SetLength(ControlCount);
    ControlRefs.SetLength(ControlCount);
    ControlDrawOrder.SetLength(ControlCount);

    GuiControlType control_type;
    int control_arr_slot;
    for (int i = 0; i < ControlCount; ++i)
    {
        control_type = (GuiControlType)((ControlRefs[i] >> 16) & 0xFFFF);
        control_arr_slot = ControlRefs[i] & 0xFFFF;

        if (control_type == kGuiButton)
        {
            Controls[i] = &guibuts[control_arr_slot];
        }
        else if (control_type == kGuiLabel)
        {
            Controls[i] = &guilabels[control_arr_slot];
        }
        else if (control_type == kGuiInvWindow)
        {
            Controls[i] = &guiinv[control_arr_slot];
        }
        else if (control_type == kGuiSlider)
        {
            Controls[i] = &guislider[control_arr_slot];
        }
        else if (control_type == kGuiTextBox)
        {
            Controls[i] = &guitext[control_arr_slot];
        }
        else if (control_type == kGuiListBox)
        {
            Controls[i] = &guilist[control_arr_slot];
        }
        else
        {
            quit("guimain: unknown control type found on gui");
        }

        Controls[i]->ParentId = Id;
        Controls[i]->Id = i;
    }
    ResortZOrder();
}

int CompareGuiControlZOrder(GuiObject *const *elem1, GuiObject *const *elem2)
{
    // returns >0 if e1 is lower down, <0 if higher, =0 if the same
    return (*elem1)->ZOrder - (*elem2)->ZOrder;
}

void GuiMain::ResortZOrder()
{
    Common::Array<GuiObject*> control_arr;
    control_arr.New(ControlCount);
    for (int i = 0; i < ControlCount; ++i)
    {
        control_arr[i] = Controls[i];
    }
    control_arr.QSort(CompareGuiControlZOrder);
    for (int i = 0; i < ControlCount; ++i)
    {
        ControlDrawOrder[i] = control_arr[i]->Id;
    }
}

bool GuiMain::SendControlToBack(int index)
{
    if (index < 0 || index >= ControlCount)
    {
        return false;
    }

    GuiObject *const control = Controls[index];
    if (control->ZOrder > 0)
    {
        int old_order = control->ZOrder;
        for (int i = 0; i < ControlCount; ++i)
        {
            if (Controls[i]->ZOrder < old_order)
            {
                Controls[i]->ZOrder++;
            }
        }
        control->ZOrder = 0;
        ResortZOrder();
        OnControlPositionChanged();
        return true;
    }
    return false;
}

void GuiMain::SetTransparencyAsPercentage(int percent)
{
    // convert from % transparent to Opacity from 0-255
	if (percent == 0)
    {
        Transparency = 0;
    }
	else if (percent == 100)
    {
        Transparency = 255;
    }
	else
    {
        Transparency = ((100 - percent) * 25) / 10;
    }
}

void GuiMain::SetVisibility(GuiVisibilityState visibility)
{
    _visibility = visibility;
}

void GuiMain::DrawBlob(Bitmap *ds, int x, int y, color_t draw_color)
{
    ds->FillRect(Rect(x, y, x + get_fixed_pixel_size(1), y + get_fixed_pixel_size(1)), draw_color);
}

void GuiMain::DrawAt(Bitmap *ds, int x, int y)
{
    SET_EIP(375)

    if (Frame.IsEmpty())
    {
        return;
    }

    Bitmap subbmp;
    subbmp.CreateSubBitmap(ds, RectWH(x, y, Frame.GetWidth(), Frame.GetHeight()));

    SET_EIP(376)
    // stop border being transparent, if the whole GUI isn't
    if ((ForegroundColor == 0) && (BackgroundColor != 0))
    {
        ForegroundColor = 16;
    }
    if (BackgroundColor != 0)
    {
        subbmp.Fill(subbmp.GetCompatibleColor(BackgroundColor));
    }
    SET_EIP(377)

    color_t draw_color;
    if (ForegroundColor != BackgroundColor)
    {
        draw_color = subbmp.GetCompatibleColor(ForegroundColor);
        subbmp.DrawRect(Rect(0, 0, subbmp.GetWidth() - 1, subbmp.GetHeight() - 1), draw_color);
        if (get_fixed_pixel_size(1) > 1)
        {
            subbmp.DrawRect(Rect(1, 1, subbmp.GetWidth() - 2, subbmp.GetHeight() - 2), draw_color);
        }
    }
    SET_EIP(378)

    if (BackgroundImage > 0 && (spriteset[BackgroundImage] != NULL))
    draw_sprite_compensate(&subbmp, BackgroundImage, 0, 0, 0);
    SET_EIP(379)

    for (int ctrl_index = 0; ctrl_index < ControlCount; ++ctrl_index)
    {
        set_eip_guiobj(ControlDrawOrder[ctrl_index]);
        GuiObject *const control = Controls[ControlDrawOrder[ctrl_index]];
        if ((control->IsDisabled()) && (gui_disabled_style == kGuiDisabled_HideControls))
        {
            continue;
        }
        if (!control->IsVisible())
        {
            continue;
        }
        control->Draw(&subbmp);
        color_t selectedColour = 14;
        if (HighlightControl == ControlDrawOrder[ctrl_index])
        {
            if (outlineGuiObjects)
            {
                selectedColour = 13;
            }
            draw_color = subbmp.GetCompatibleColor(selectedColour);
            DrawBlob(&subbmp, control->GetX() + control->GetWidth() - get_fixed_pixel_size(1) - 1, control->GetY(), draw_color);
            DrawBlob(&subbmp, control->GetX(), control->GetY() + control->GetHeight() - get_fixed_pixel_size(1) - 1, draw_color);
            DrawBlob(&subbmp, control->GetX(), control->GetY(), draw_color);
            DrawBlob(&subbmp, control->GetX() + control->GetWidth() - get_fixed_pixel_size(1) - 1, 
                control->GetY() + control->GetHeight() - get_fixed_pixel_size(1) - 1, draw_color);
        }
        if (outlineGuiObjects)
        {
            // draw a dotted outline round all objects
            draw_color = subbmp.GetCompatibleColor(selectedColour);
            for (int i = 0; i < control->GetWidth(); i += 2)
            {
                subbmp.PutPixel(i + control->GetX(), control->GetY(), draw_color);
                subbmp.PutPixel(i + control->GetX(), control->GetY() + control->GetHeight() - 1, draw_color);
            }
            for (int i = 0; i < control->GetHeight(); i += 2)
            {
                subbmp.PutPixel(control->GetX(), i + control->GetY(), draw_color);
                subbmp.PutPixel(control->GetX() + control->GetWidth() - 1, i + control->GetY(), draw_color);
            }
        }
    }
    SET_EIP(380)
}

void GuiMain::Draw(Bitmap *ds)
{
    DrawAt(ds, Frame.Left, Frame.Top);
}

void GuiMain::OnControlPositionChanged()
{
    // force it to re-check for which control is under the mouse
    MouseWasAt.X = -1;
    MouseWasAt.Y = -1;
}

void GuiMain::OnMouseButtonDown()
{
    if (MouseOverControl < 0)
    {
        return;
    }

    // don't activate disabled buttons
    GuiObject * const control = Controls[MouseOverControl];
    if ((control->IsDisabled()) || (!control->IsVisible()) ||
        (!control->IsClickable()))
    {
        return;
    }
    MouseDownControl = MouseOverControl;
    if (control->OnMouseDown())
    {
        MouseOverControl = MOVER_MOUSEDOWNLOCKED;
    }
    Controls[MouseDownControl]->OnMouseMove(mousex - Frame.Left, mousey - Frame.Top);
    guis_need_update = 1;
}

void GuiMain::OnMouseButtonUp()
{
    // focus was locked - reset it back to normal, but IsVisible the
    // locked object so that a OnMouseLeave gets fired if necessary
    if (MouseOverControl == MOVER_MOUSEDOWNLOCKED)
    {
        MouseOverControl = MouseDownControl;
        MouseWasAt.X = -1;  // force update
    }
    if (MouseDownControl < 0)
    {
        return;
    }
    Controls[MouseDownControl]->OnMouseUp();
    MouseDownControl = -1;
    guis_need_update = 1;
}

void GuiMain::Poll()
{
    Point mouse_was(mousex, mousey);
    mousex -= Frame.Left;
    mousey -= Frame.Top;

    if ((mousex != MouseWasAt.X) || (mousey != MouseWasAt.Y))
    {
        int new_ctrl_index = FindControlUnderMouse();
    
        if (MouseOverControl == MOVER_MOUSEDOWNLOCKED)
        {
            Controls[MouseDownControl]->OnMouseMove(mousex, mousey);
        }
        else if (new_ctrl_index != MouseOverControl)
        {
            if (MouseOverControl >= 0)
            {
                Controls[MouseOverControl]->OnMouseLeave();
            }
            if ((new_ctrl_index >= 0) && (Controls[new_ctrl_index]->IsDisabled()))
            {
                // the control is disabled - ignore it
                MouseOverControl = -1;
            }
            else if ((new_ctrl_index >= 0) && (!Controls[new_ctrl_index]->IsClickable()))
            {
                // the control is not clickable - ignore it
                MouseOverControl = -1;
            }
            else
            {
                // over a different control
                MouseOverControl = new_ctrl_index;
                if (MouseOverControl >= 0)
                {
                    Controls[MouseOverControl]->OnMouseOver();
                    Controls[MouseOverControl]->OnMouseMove(mousex, mousey);
                }
            }
            guis_need_update = 1;
        }
        else if (MouseOverControl >= 0)
        {
            Controls[MouseOverControl]->OnMouseMove(mousex, mousey);
        }
    }

    MouseWasAt.X = mousex;
    MouseWasAt.Y = mousey;
    mousex = mouse_was.X;
    mousey = mouse_was.Y;
}

void GuiMain::SetX(int x)
{
    Frame.MoveToX(x);
}

void GuiMain::SetY(int y)
{
    Frame.MoveToY(y);
}

void GuiMain::SetWidth(int width)
{
    Frame.SetWidth(width);
}

void GuiMain::SetHeight(int height)
{
    Frame.SetHeight(height);
}

void GuiMain::ReadFromFile(Stream *in, GuiVersion gui_version)
{
    if (gui_version < kGuiVersion_340_alpha)
    {
        char vtext[4];
        in->Read(vtext, sizeof(vtext));
        Name.ReadCount(in, 16);
        OnClickHandler.ReadCount(in, 20);
        Frame.Left = in->ReadInt32();
        Frame.Top = in->ReadInt32();
        Frame.SetWidth(in->ReadInt32());
        Frame.SetHeight(in->ReadInt32());
        in->ReadInt32(); // focus
        ControlCount = in->ReadInt32();
        PopupStyle = (GuiPopupStyle)in->ReadInt32();
        PopupAtMouseY = in->ReadInt32();
        BackgroundColor = in->ReadInt32();
        BackgroundImage = in->ReadInt32();
        ForegroundColor = in->ReadInt32();
        in->ReadInt32(); // mouseover
        in->ReadInt32(); // mousewasx
        in->ReadInt32(); // mousewasy
        in->ReadInt32(); // mousedownon
        in->ReadInt32(); // highlightobj
        Flags = in->ReadInt32();
        if (vtext[0] == kGuiMain_TextWindow)
        {
            Flags |= kGuiMain_TextWindow;
        }
        Transparency = in->ReadInt32();
        ZOrder = in->ReadInt32();
        Id = in->ReadInt32();
        int reserved[6];
        in->Read(reserved, sizeof(reserved));
        _visibility = (GuiVisibilityState)in->ReadInt32();
        // 64 bit fix: Read 4 byte int values into array of 8 byte long ints
        char buffer[LEGACY_MAX_CONTROLS_ON_GUI * sizeof(int32_t)];
        in->Read(buffer, sizeof(buffer));
    }
    else
    {
        Id = in->ReadInt32();
        Name.Read(in);
        Flags = in->ReadInt32();
        Frame.Left = in->ReadInt32();
        Frame.Top = in->ReadInt32();
        Frame.SetWidth(in->ReadInt32());
        Frame.SetHeight(in->ReadInt32());
        BackgroundColor = in->ReadInt32();
        BackgroundImage = in->ReadInt32();
        ForegroundColor = in->ReadInt32();
        Transparency = in->ReadInt32();
        PopupStyle = (GuiPopupStyle)in->ReadInt32();
        PopupAtMouseY = in->ReadInt32();
        OnClickHandler.Read(in);
        ZOrder = in->ReadInt32();
    }

    int read_control_count = LEGACY_MAX_CONTROLS_ON_GUI;
    if (gui_version >= kGuiVersion_340_alpha)
    {
        ControlCount = in->ReadInt32();
        read_control_count = ControlCount;
    }
    ControlRefs.ReadRaw(in, read_control_count);
}

void GuiMain::WriteToFile(Stream *out)
{
    out->WriteInt32(Id);
    Name.Write(out);
    out->WriteInt32(Flags);
    out->WriteInt32(Frame.Left);
    out->WriteInt32(Frame.Top);
    out->WriteInt32(Frame.GetWidth());
    out->WriteInt32(Frame.GetHeight());
    out->WriteInt32(BackgroundColor);
    out->WriteInt32(BackgroundImage);
    out->WriteInt32(ForegroundColor);
    out->WriteInt32(Transparency);
    out->WriteInt32(PopupStyle);
    out->WriteInt32(PopupAtMouseY);
    OnClickHandler.Write(out);
    out->WriteInt32(ZOrder);

    out->WriteInt32(ControlCount);
    ControlRefs.WriteRaw(out);
}

void GuiMain::ReadFromSavedGame(Stream *in, RuntimeGuiVersion version)
{
    Id = in->ReadInt32();
    Name.Read(in);
    Flags = in->ReadInt32();
    Frame.Left = in->ReadInt32();
    Frame.Top = in->ReadInt32();
    Frame.SetWidth(in->ReadInt32());
    Frame.SetHeight(in->ReadInt32());
    BackgroundColor = in->ReadInt32();
    BackgroundImage = in->ReadInt32();
    ForegroundColor = in->ReadInt32();
    Transparency = in->ReadInt32();
    PopupStyle = (GuiPopupStyle)in->ReadInt32();
    PopupAtMouseY = in->ReadInt32();
    OnClickHandler.Read(in);
    _visibility = (GuiVisibilityState)in->ReadInt32();
    ZOrder = in->ReadInt32();
}

void GuiMain::WriteToSavedGame(Stream *out)
{
    out->WriteInt32(Id);
    Name.Write(out);
    out->WriteInt32(Flags);
    out->WriteInt32(Frame.Left);
    out->WriteInt32(Frame.Top);
    out->WriteInt32(Frame.GetWidth());
    out->WriteInt32(Frame.GetHeight());
    out->WriteInt32(BackgroundColor);
    out->WriteInt32(BackgroundImage);
    out->WriteInt32(ForegroundColor);
    out->WriteInt32(Transparency);
    out->WriteInt32(PopupStyle);
    out->WriteInt32(PopupAtMouseY);
    OnClickHandler.Write(out);
    out->WriteInt32(_visibility);
    out->WriteInt32(ZOrder);
}

namespace Gui
{
    void ResortGuis(ObjectArray<GuiMain> &guis, bool bwcompat_ctrl_zorder = false);
}

void Gui::ResortGuis(ObjectArray<GuiMain> &guis, bool bwcompat_ctrl_zorder)
{
    // set up the reverse-lookup array
    for (int gui_index = 0; gui_index < guis.GetCount(); ++gui_index)
    {
        guis[gui_index].RebuildArray();
        for (int ctrl_index = 0; ctrl_index < guis[gui_index].ControlCount; ++ctrl_index)
        {
            guis[gui_index].Controls[ctrl_index]->ParentId = gui_index;
            guis[gui_index].Controls[ctrl_index]->Id = ctrl_index;

            if (bwcompat_ctrl_zorder)
            {
                guis[gui_index].Controls[ctrl_index]->ZOrder = ctrl_index;
            }
        }
        guis[gui_index].ResortZOrder();
    }
    guis_need_update = 1;
}

bool Gui::ReadGui(ObjectArray<GuiMain> &guis, Stream *in)
{
    if (in->ReadInt32() != (int)GUIMAGIC)
    {
        quit("ReadGui: file is corrupt");
    }

    LoadedGuiVersion = (GuiVersion)in->ReadInt32();
    int gui_count;
    if (LoadedGuiVersion < kGuiVersion_214)
    {
        gui_count = (int)LoadedGuiVersion;
        LoadedGuiVersion = kGuiVersion_Initial;
    }
    else if (LoadedGuiVersion > kGuiVersion_Current)
    {
        quit("read_gui: this game requires a newer version of AGS");
    }
    else
    {
        gui_count = in->ReadInt32();
    }

    guis.SetLength(gui_count);
    // import the main GUI elements
    for (int i = 0; i < gui_count; ++i)
    {
        guis[i].Init();
        guis[i].ReadFromFile(in, LoadedGuiVersion);

        // Perform fixups
        if (guis[i].GetHeight() < 2)
        {
            guis[i].SetHeight(2);
        }
        if (LoadedGuiVersion < kGuiVersion_unkn_103)
        {
            guis[i].Name.Format("GUI%d", i);
        }
        if (LoadedGuiVersion < kGuiVersion_260)
        {
            guis[i].ZOrder = i;
        }
        if (LoadedGuiVersion < kGuiVersion_270)
        {
            guis[i].OnClickHandler.Empty();
        }
        if (loaded_game_file_version <= kGameVersion_272) // Fix names for 2.x: "GUI" -> "gGui"
        {
            guis[i].Name = GuiMain::FixupGuiName(guis[i].Name);
        }        
        guis[i].Id = i;
    }

    // import the buttons
    numguibuts = in->ReadInt32();
    guibuts.SetLength(numguibuts);
    for (int i = 0; i < numguibuts; ++i)
    {
        guibuts[i].ReadFromFile(in, LoadedGuiVersion);
    }
    // labels
    numguilabels = in->ReadInt32();
    guilabels.SetLength(numguilabels);
    for (int i = 0; i < numguilabels; ++i)
    {
        guilabels[i].ReadFromFile(in, LoadedGuiVersion);
    }
    // inv controls
    numguiinv = in->ReadInt32();
    guiinv.SetLength(numguiinv);
    for (int i = 0; i < numguiinv; ++i)
    {
        guiinv[i].ReadFromFile(in, LoadedGuiVersion);
    }

    if (LoadedGuiVersion >= kGuiVersion_214)
    {
        // sliders
        numguislider = in->ReadInt32();
        guislider.SetLength(numguislider);
        for (int i = 0; i < numguislider; ++i)
        {
            guislider[i].ReadFromFile(in, LoadedGuiVersion);
        }
    }

    if (LoadedGuiVersion >= kGuiVersion_222)
    {
        // text boxes
        numguitext = in->ReadInt32();
        guitext.SetLength(numguitext);
        for (int i = 0; i < numguitext; ++i)
        {
            guitext[i].ReadFromFile(in, LoadedGuiVersion);
        }
    }

    if (LoadedGuiVersion >= kGuiVersion_230)
    {
        // list boxes
        numguilist = in->ReadInt32();
        guilist.SetLength(numguilist);
        for (int i = 0; i < numguilist; ++i)
        {
            guilist[i].ReadFromFile(in, LoadedGuiVersion);
        }
    }

    ResortGuis(guis, LoadedGuiVersion < kGuiVersion_272e);
    return true;
}

void Gui::WriteGui(ObjectArray<GuiMain> &guis, Stream *out)
{
    out->WriteInt32(GUIMAGIC);
    out->WriteInt32(kGuiVersion_Current);
    const int gui_count = guis.GetCount();
    out->WriteInt32(gui_count);
    for (int i = 0; i < gui_count; ++i)
    {
        guis[i].WriteToFile(out);
    }
    out->WriteInt32(numguibuts);
    for (int i = 0; i < numguibuts; ++i)
    {
        guibuts[i].WriteToFile(out);
    }
    out->WriteInt32(numguilabels);
    for (int i = 0; i < numguilabels; ++i)
    {
        guilabels[i].WriteToFile(out);
    }
    out->WriteInt32(numguiinv);
    for (int i = 0; i < numguiinv; ++i)
    {
        guiinv[i].WriteToFile(out);
    }
    out->WriteInt32(numguislider);
    for (int i = 0; i < numguislider; ++i)
    {
        guislider[i].WriteToFile(out);
    }
    out->WriteInt32(numguitext);
    for (int i = 0; i < numguitext; ++i)
    {
        guitext[i].WriteToFile(out);
    }
    out->WriteInt32(numguilist);
    for (int i = 0; i < numguilist; ++i)
    {
        guilist[i].WriteToFile(out);
    }
}

bool Gui::ReadGuiFromSavedGame(ObjectArray<GuiMain> &guis, Common::Stream *in, RuntimeGuiVersion version)
{
    // import the main GUI elements
    const int gui_count = guis.GetCount();
    for (int i = 0; i < gui_count; ++i)
    {
        guis[i].ReadFromSavedGame(in, version);
    }
    // import the buttons
    numguibuts = in->ReadInt32();
    guibuts.SetLength(numguibuts);
    for (int i = 0; i < numguibuts; ++i)
    {
        guibuts[i].ReadFromSavedGame(in, version);
    }
    // labels
    numguilabels = in->ReadInt32();
    guilabels.SetLength(numguilabels);
    for (int i = 0; i < numguilabels; ++i)
    {
        guilabels[i].ReadFromSavedGame(in, version);
    }
    // inv controls
    numguiinv = in->ReadInt32();
    guiinv.SetLength(numguiinv);
    for (int i = 0; i < numguiinv; ++i)
    {
        guiinv[i].ReadFromSavedGame(in, version);
    }
    // sliders
    numguislider = in->ReadInt32();
    guislider.SetLength(numguislider);
    for (int i = 0; i < numguislider; ++i)
    {
        guislider[i].ReadFromSavedGame(in, version);
    }
    // text boxes
    numguitext = in->ReadInt32();
    guitext.SetLength(numguitext);
    for (int i = 0; i < numguitext; ++i)
    {
        guitext[i].ReadFromSavedGame(in, version);
    }
    // list boxes
    numguilist = in->ReadInt32();
    guilist.SetLength(numguilist);
    for (int i = 0; i < numguilist; ++i)
    {
        guilist[i].ReadFromSavedGame(in, version);
    }

    ResortGuis(guis);
    return true;
}

void Gui::WriteGuiToSavedGame(ObjectArray<GuiMain> &guis, Common::Stream *out)
{
    const int gui_count = guis.GetCount();
    for (int i = 0; i < gui_count; ++i)
    {
        guis[i].WriteToSavedGame(out);
    }
    out->WriteInt32(numguibuts);
    for (int i = 0; i < numguibuts; ++i)
    {
        guibuts[i].WriteToSavedGame(out);
    }
    out->WriteInt32(numguilabels);
    for (int i = 0; i < numguilabels; ++i)
    {
        guilabels[i].WriteToSavedGame(out);
    }
    out->WriteInt32(numguiinv);
    for (int i = 0; i < numguiinv; ++i)
    {
        guiinv[i].WriteToSavedGame(out);
    }
    out->WriteInt32(numguislider);
    for (int i = 0; i < numguislider; ++i)
    {
        guislider[i].WriteToSavedGame(out);
    }
    out->WriteInt32(numguitext);
    for (int i = 0; i < numguitext; ++i)
    {
        guitext[i].WriteToSavedGame(out);
    }
    out->WriteInt32(numguilist);
    for (int i = 0; i < numguilist; ++i)
    {
        guilist[i].WriteToSavedGame(out);
    }
}

} // namespace Common
} // namespace AGS

int guis_need_update = 1;
int all_buttons_disabled = 0, gui_inv_pic = -1;
int gui_disabled_style = 0;
