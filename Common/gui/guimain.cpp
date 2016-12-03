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

#include <algorithm>
#include "gui/guimain.h"
#include "ac/common.h"	// quit()
#include "ac/gamesetupstruct.h"
#include "gui/guibutton.h"
#include "gui/guilabel.h"
#include "gui/guislider.h"
#include "gui/guiinv.h"
#include "gui/guitextbox.h"
#include "gui/guilistbox.h"
#include "font/fonts.h"
#include "ac/spritecache.h"
#include "util/stream.h"
#include "gfx/bitmap.h"
#include "gfx/gfx_def.h"
#include "debug/out.h"
#include "util/math.h"
#include "util/string_utils.h"

using namespace AGS::Common;

#define MOVER_MOUSEDOWNLOCKED -4000

int guis_need_update = 1;
int all_buttons_disabled = 0, gui_inv_pic = -1;
int gui_disabled_style = 0;

namespace AGS
{
namespace Common
{

/* static */ String GUIMain::FixupGUIName(const String &name)
{
    if (name.GetLength() > 0 && name[0u] != 'g')
        return String::FromFormat("g%c%s", name[0u], name.Mid(1).Lower().GetCStr());
    return name;
}

GUIMain::GUIMain()
{
    Init();
}

void GUIMain::Init()
{
    Id            = 0;
    Name.Empty();
    Flags         = 0;

    X             = 0;
    Y             = 0;
    Width         = 0;
    Height        = 0;
    BgColor       = 8;
    BgImage       = 0;
    FgColor       = 1;
    Padding       = TEXTWINDOW_PADDING_DEFAULT;
    PopupStyle    = kGUIPopupNone;
    PopupAtMouseY = -1;
    Transparency  = 0;
    ZOrder        = -1;

    _visibility   = kGUIVisibility_On;
    FocusCtrl     = 0;
    HighlightCtrl = -1;
    MouseOverCtrl = -1;
    MouseDownCtrl = -1;
    MouseWasAt.X  = -1;
    MouseWasAt.Y  = -1;

    OnClickHandler.Empty();

    ControlCount  = 0;
}

int GUIMain::FindControlUnderMouse(int leeway, bool must_be_clickable) const
{
    if (loaded_game_file_version <= kGameVersion_262)
    {
        // Ignore draw order On 2.6.2 and lower
        for (int i = 0; i < ControlCount; ++i)
        {
            if (!Controls[i]->IsVisible())
                continue;
            if (!Controls[i]->IsClickable() && must_be_clickable)
                continue;
            if (Controls[i]->IsOverControl(mousex, mousey, leeway))
                return i;
        }
    }
    else
    {
        for (int i = ControlCount - 1; i >= 0; --i)
        {
            const int ctrl_index = CtrlDrawOrder[i];
            if (!Controls[ctrl_index]->IsVisible())
                continue;
            if (!Controls[ctrl_index]->IsClickable() && must_be_clickable)
                continue;
            if (Controls[ctrl_index]->IsOverControl(mousex, mousey, leeway))
                return ctrl_index;
        }
    }
    return -1;
}

int GUIMain::FindControlUnderMouse() const
{
    return FindControlUnderMouse(0, true);
}

int GUIMain::FindControlUnderMouse(int leeway) const
{
    return FindControlUnderMouse(leeway, true);
}

GUIControlType GUIMain::GetControlType(int index) const
{
    if (index < 0 || index >= ControlCount)
        return kGUIControlUndefined;
    return (GUIControlType)((CtrlRefs[index] >> 16) & 0x0000ffff);
}

bool GUIMain::IsInteractableAt(int x, int y) const
{
    if (!IsVisible())
        return false;
    if (Flags & kGUIMain_NoClick)
        return false;
    if ((x >= X) & (y >= Y) & (x < X + Width) & (y < Y + Height))
        return true;
    return false;
}

bool GUIMain::IsTextWindow() const
{
    return (Flags & kGUIMain_TextWindow) != 0;
}

bool GUIMain::BringControlToFront(int index)
{
    return SetControlZOrder(index, ControlCount - 1);
}

void GUIMain::Draw(Common::Bitmap *ds)
{
    DrawAt(ds, X, Y);
}

void GUIMain::DrawAt(Common::Bitmap *ds, int x, int y)
{
    SET_EIP(375)

    if ((Width < 1) || (Height < 1))
        return;

    Bitmap subbmp;
    subbmp.CreateSubBitmap(ds, RectWH(x, y, Width, Height));

    SET_EIP(376)
    // stop border being transparent, if the whole GUI isn't
    if ((FgColor == 0) && (BgColor != 0))
        FgColor = 16;

    if (BgColor != 0)
        subbmp.Fill(subbmp.GetCompatibleColor(BgColor));

    SET_EIP(377)

    color_t draw_color;
    if (FgColor != BgColor)
    {
        draw_color = subbmp.GetCompatibleColor(FgColor);
        subbmp.DrawRect(Rect(0, 0, subbmp.GetWidth() - 1, subbmp.GetHeight() - 1), draw_color);
        if (get_fixed_pixel_size(1) > 1)
            subbmp.DrawRect(Rect(1, 1, subbmp.GetWidth() - 2, subbmp.GetHeight() - 2), draw_color);
    }

    SET_EIP(378)

    if (BgImage > 0 && spriteset[BgImage] != NULL)
        draw_gui_sprite(&subbmp, BgImage, 0, 0, false);

    SET_EIP(379)

    for (int ctrl_index = 0; ctrl_index < ControlCount; ++ctrl_index)
    {
        set_eip_guiobj(CtrlDrawOrder[ctrl_index]);

        GUIObject *objToDraw = Controls[CtrlDrawOrder[ctrl_index]];

        if (objToDraw->IsDisabled() && gui_disabled_style == GUIDIS_BLACKOUT)
            continue;
        if (!objToDraw->IsVisible())
            continue;

        objToDraw->Draw(&subbmp);

        int selectedColour = 14;

        if (HighlightCtrl == CtrlDrawOrder[ctrl_index])
        {
            if (outlineGuiObjects)
                selectedColour = 13;
            draw_color = subbmp.GetCompatibleColor(selectedColour);
            DrawBlob(&subbmp, objToDraw->x + objToDraw->wid - get_fixed_pixel_size(1) - 1, objToDraw->y, draw_color);
            DrawBlob(&subbmp, objToDraw->x, objToDraw->y + objToDraw->hit - get_fixed_pixel_size(1) - 1, draw_color);
            DrawBlob(&subbmp, objToDraw->x, objToDraw->y, draw_color);
            DrawBlob(&subbmp, objToDraw->x + objToDraw->wid - get_fixed_pixel_size(1) - 1, 
                    objToDraw->y + objToDraw->hit - get_fixed_pixel_size(1) - 1, draw_color);
        }
        if (outlineGuiObjects)
        {
            // draw a dotted outline round all objects
            draw_color = subbmp.GetCompatibleColor(selectedColour);
            for (int i = 0; i < objToDraw->wid; i += 2)
            {
                subbmp.PutPixel(i + objToDraw->x, objToDraw->y, draw_color);
                subbmp.PutPixel(i + objToDraw->x, objToDraw->y + objToDraw->hit - 1, draw_color);
            }
            for (int i = 0; i < objToDraw->hit; i += 2)
            {
                subbmp.PutPixel(objToDraw->x, i + objToDraw->y, draw_color);
                subbmp.PutPixel(objToDraw->x + objToDraw->wid - 1, i + objToDraw->y, draw_color);
            }
        }
    }

    SET_EIP(380)
}

void GUIMain::DrawBlob(Common::Bitmap *ds, int x, int y, color_t draw_color)
{
    ds->FillRect(Rect(x, y, x + get_fixed_pixel_size(1), y + get_fixed_pixel_size(1)), draw_color);
}

void GUIMain::Poll()
{
    int mxwas = mousex, mywas = mousey;

    mousex -= X;
    mousey -= Y;
    if (mousex != MouseWasAt.X || mousey != MouseWasAt.Y)
    {
        int ctrl_index = FindControlUnderMouse();

        if (MouseOverCtrl == MOVER_MOUSEDOWNLOCKED)
            Controls[MouseDownCtrl]->MouseMove(mousex, mousey);
        else if (ctrl_index != MouseOverCtrl)
        {
            if (MouseOverCtrl >= 0)
                Controls[MouseOverCtrl]->MouseLeave();

            if (ctrl_index >= 0 && Controls[ctrl_index]->IsDisabled())
                // the control is disabled - ignore it
                MouseOverCtrl = -1;
            else if (ctrl_index >= 0 && !Controls[ctrl_index]->IsClickable())
                // the control is not clickable - ignore it
                MouseOverCtrl = -1;
            else
            {
                // over a different control
                MouseOverCtrl = ctrl_index;
                if (MouseOverCtrl >= 0)
                {
                    Controls[MouseOverCtrl]->MouseOver();
                    Controls[MouseOverCtrl]->MouseMove(mousex, mousey);
                }
            }
            guis_need_update = 1;
        } 
        else if (MouseOverCtrl >= 0)
            Controls[MouseOverCtrl]->MouseMove(mousex, mousey);
    }

    MouseWasAt.X = mousex;
    MouseWasAt.Y = mousey;
    mousex = mxwas;
    mousey = mywas;
}

void GUIMain::RebuildArray()
{
    int thistype, thisnum;

    Controls.resize(ControlCount);
    for (int i = 0; i < ControlCount; ++i)
    {
        thistype = (CtrlRefs[i] >> 16) & 0x000ffff;
        thisnum = CtrlRefs[i] & 0x0000ffff;

        if (thisnum < 0 || thisnum >= 2000)
            quit("GUIMain: rebuild array failed (invalid object index)");

        if (thistype == kGUIButton)
            Controls[i] = &guibuts[thisnum];
        else if (thistype == kGUILabel)
            Controls[i] = &guilabels[thisnum];
        else if (thistype == kGUIInvWindow)
            Controls[i] = &guiinv[thisnum];
        else if (thistype == kGUISlider)
            Controls[i] = &guislider[thisnum];
        else if (thistype == kGUITextBox)
            Controls[i] = &guitext[thisnum];
        else if (thistype == kGUIListBox)
            Controls[i] = &guilist[thisnum];
        else
            quit("guimain: unknown control type found On gui");

        Controls[i]->guin = Id;
        Controls[i]->objn = i;
    }

    ResortZOrder();
}

bool GUIControlZOrder(const GUIObject *e1, const GUIObject *e2)
{
    return e1->zorder < e2->zorder;
}

void GUIMain::ResortZOrder()
{
    std::vector<GUIObject*> ctrl_sort = Controls;
    std::sort(ctrl_sort.begin(), ctrl_sort.end(), GUIControlZOrder);

    CtrlDrawOrder.resize(ctrl_sort.size());
    for (int i = 0; i < ControlCount; ++i)
        CtrlDrawOrder[i] = ctrl_sort[i]->objn;
}

bool GUIMain::SendControlToBack(int index)
{
    return SetControlZOrder(index, 0);
}

bool GUIMain::SetControlZOrder(int index, int zorder)
{
    if (index < 0 || index >= ControlCount)
        return false; // no such control

    zorder = Math::Clamp(0, ControlCount - 1, zorder);
    const int old_zorder = Controls[index]->zorder;
    if (old_zorder == zorder)
        return false; // no change

    const bool move_back = zorder < old_zorder; // back is at zero index
    const int  left      = move_back ? zorder : old_zorder;
    const int  right     = move_back ? old_zorder : zorder;
    for (int i = 0; i < ControlCount; ++i)
    {
        const int i_zorder = Controls[i]->zorder;
        if (i_zorder == old_zorder)
            Controls[i]->zorder = zorder; // the control we are moving
        else if (i_zorder >= left && i_zorder <= right)
        {
            // controls in between old and new positions shift towards free place
            if (move_back)
                Controls[i]->zorder++; // move to front
            else
                Controls[i]->zorder--; // move to back
        }
    }
    ResortZOrder();
    OnControlPositionChanged();
    return true;
}

void GUIMain::SetTransparencyAsPercentage(int percent)
{
    Transparency = GfxDef::Trans100ToLegacyTrans255(percent);
}

void GUIMain::SetVisibility(GUIVisibilityState visibility)
{
    _visibility = visibility;
}

void GUIMain::OnControlPositionChanged()
{
    // force it to re-check for which control is under the mouse
    MouseWasAt.X = -1;
    MouseWasAt.Y = -1;
}

void GUIMain::OnMouseButtonDown()
{
    if (MouseOverCtrl < 0)
        return;

    // don't activate disabled buttons
    if (Controls[MouseOverCtrl]->IsDisabled() || !Controls[MouseOverCtrl]->IsVisible() ||
        !Controls[MouseOverCtrl]->IsClickable())
    return;

    MouseDownCtrl = MouseOverCtrl;
    if (Controls[MouseOverCtrl]->MouseDown())
        MouseOverCtrl = MOVER_MOUSEDOWNLOCKED;
    Controls[MouseDownCtrl]->MouseMove(mousex - X, mousey - Y);
    guis_need_update = 1;
}

void GUIMain::OnMouseButtonUp()
{
    // FocusCtrl was locked - reset it back to normal, but On the
    // locked object so that a MouseLeave gets fired if necessary
    if (MouseOverCtrl == MOVER_MOUSEDOWNLOCKED)
    {
        MouseOverCtrl = MouseDownCtrl;
        MouseWasAt.X = -1;  // force update
    }

    if (MouseDownCtrl < 0)
        return;

    Controls[MouseDownCtrl]->MouseUp();
    MouseDownCtrl = -1;
    guis_need_update = 1;
}

void GUIMain::ReadFromFile(Stream *in, GuiVersion gui_version)
{
    char tw_flags[GUIMAIN_LEGACY_TW_FLAGS_SIZE];
    in->Read(tw_flags, sizeof(tw_flags));
    if (gui_version < kGuiVersion_340)
    {
        Name.ReadCount(in, GUIMAIN_NAME_LENGTH);
        OnClickHandler.ReadCount(in, GUIMAIN_EVENTHANDLER_LENGTH);
    }
    else
    {
        Name = StrUtil::ReadString(in);
        OnClickHandler = StrUtil::ReadString(in);
    }
    X             = in->ReadInt32();
    Y             = in->ReadInt32();
    Width         = in->ReadInt32();
    Height        = in->ReadInt32();
    FocusCtrl     = in->ReadInt32();
    ControlCount  = in->ReadInt32();
    PopupStyle    = (GUIPopupStyle)in->ReadInt32();
    PopupAtMouseY = in->ReadInt32();
    BgColor       = in->ReadInt32();
    BgImage       = in->ReadInt32();
    FgColor       = in->ReadInt32();
    MouseOverCtrl = in->ReadInt32();
    MouseWasAt.X  = in->ReadInt32();
    MouseWasAt.Y  = in->ReadInt32();
    MouseDownCtrl = in->ReadInt32();
    HighlightCtrl = in->ReadInt32();
    Flags         = in->ReadInt32();
    if (tw_flags[0] == kGUIMain_LegacyTextWindow)
    {
        Flags |= kGUIMain_TextWindow;
    }
    Transparency  = in->ReadInt32();
    ZOrder        = in->ReadInt32();
    Id            = in->ReadInt32();
    Padding       = in->ReadInt32();
    in->Seek(sizeof(int32_t) * GUIMAIN_RESERVED_INTS);
    _visibility = (GUIVisibilityState)in->ReadInt32();

    if (gui_version < kGuiVersion_340)
    {
        CtrlRefs.resize(LEGACY_MAX_OBJS_ON_GUI);
        // array of 32-bit pointers; these values are unused
        in->Seek(LEGACY_MAX_OBJS_ON_GUI * sizeof(int32_t));
        in->ReadArrayOfInt32(&CtrlRefs.front(), LEGACY_MAX_OBJS_ON_GUI);
    }
    else
    {
        CtrlRefs.resize(ControlCount);
        if (ControlCount > 0)
            in->ReadArrayOfInt32(&CtrlRefs.front(), ControlCount);
    }
}

void GUIMain::WriteToFile(Stream *out, GuiVersion gui_version) const
{
    char tw_flags[GUIMAIN_LEGACY_TW_FLAGS_SIZE] = {0};
    if (Flags & kGUIMain_TextWindow)
        tw_flags[0] = kGUIMain_LegacyTextWindow;
    out->Write(tw_flags, sizeof(tw_flags));
    if (gui_version < kGuiVersion_340)
    {
        Name.WriteCount(out, GUIMAIN_NAME_LENGTH);
        OnClickHandler.WriteCount(out, GUIMAIN_EVENTHANDLER_LENGTH);
    }
    else
    {
        StrUtil::WriteString(Name, out);
        StrUtil::WriteString(OnClickHandler, out);
    }
    out->WriteInt32(X);
    out->WriteInt32(Y);
    out->WriteInt32(Width);
    out->WriteInt32(Height);
    out->WriteInt32(FocusCtrl);
    out->WriteInt32(ControlCount);
    out->WriteInt32(PopupStyle);
    out->WriteInt32(PopupAtMouseY);
    out->WriteInt32(BgColor);
    out->WriteInt32(BgImage);
    out->WriteInt32(FgColor);
    out->WriteInt32(MouseOverCtrl);
    out->WriteInt32(MouseWasAt.X);
    out->WriteInt32(MouseWasAt.Y);
    out->WriteInt32(MouseDownCtrl);
    out->WriteInt32(HighlightCtrl);
    out->WriteInt32(Flags);
    out->WriteInt32(Transparency);
    out->WriteInt32(ZOrder);
    out->WriteInt32(Id);
    out->WriteInt32(Padding);
    int32_t reserved_ints[GUIMAIN_RESERVED_INTS] = {0};
    out->WriteArrayOfInt32(reserved_ints, GUIMAIN_RESERVED_INTS);
    out->WriteInt32(_visibility);

    if (gui_version < kGuiVersion_340)
    {
        // array of dummy 32-bit pointers
        int32_t dummy_arr[LEGACY_MAX_OBJS_ON_GUI] = {0};
        out->WriteArrayOfInt32(dummy_arr, LEGACY_MAX_OBJS_ON_GUI);
        out->WriteArrayOfInt32(&CtrlRefs.front(), LEGACY_MAX_OBJS_ON_GUI);
    }
    else if (ControlCount > 0)
    {
        out->WriteArrayOfInt32(&CtrlRefs.front(), ControlCount);
    }
}

} // namespace Common
} // namespace AGS

GuiVersion GameGuiVersion = kGuiVersion_Initial;
void read_gui(Stream *in, std::vector<GUIMain> &guiread, GameSetupStruct * gss)
{
  int ee;

  if (in->ReadInt32() != (int)GUIMAGIC)
    quit("read_gui: file is corrupt");

  GameGuiVersion = (GuiVersion)in->ReadInt32();
  Debug::Printf("Game GUI version: %d", GameGuiVersion);
  if (GameGuiVersion < kGuiVersion_214) {
    gss->numgui = (int)GameGuiVersion;
    GameGuiVersion = kGuiVersion_Initial;
  }
  else if (GameGuiVersion > kGuiVersion_Current)
    quit("read_gui: this game requires a newer version of AGS");
  else
    gss->numgui = in->ReadInt32();

  if ((gss->numgui < 0) || (gss->numgui > 1000))
    quit("read_gui: invalid number of GUIs, file corrupt?");

  guiread.resize(gss->numgui);

  // import the main GUI elements
  for (int iteratorCount = 0; iteratorCount < gss->numgui; ++iteratorCount)
  {
    guiread[iteratorCount].Init();
    guiread[iteratorCount].ReadFromFile(in, GameGuiVersion);
  }

  for (ee = 0; ee < gss->numgui; ee++) {
    if (guiread[ee].Height < 2)
      guiread[ee].Height = 2;

    if (GameGuiVersion < kGuiVersion_unkn_103)
      guiread[ee].Name.Format("GUI%d", ee);
    if (GameGuiVersion < kGuiVersion_260)
      guiread[ee].ZOrder = ee;
    if (GameGuiVersion < kGuiVersion_331)
      guiread[ee].Padding = TEXTWINDOW_PADDING_DEFAULT;

    if (loaded_game_file_version <= kGameVersion_272) // Fix names for 2.x: "GUI" -> "gGui"
        guiread[ee].Name = GUIMain::FixupGUIName(guiread[ee].Name);

    guiread[ee].Id = ee;
  }

  // import the buttons
  numguibuts = in->ReadInt32();
  guibuts.resize(numguibuts);

  for (ee = 0; ee < numguibuts; ee++)
    guibuts[ee].ReadFromFile(in, GameGuiVersion);

  // labels
  numguilabels = in->ReadInt32();
  guilabels.resize(numguilabels);

  for (ee = 0; ee < numguilabels; ee++)
    guilabels[ee].ReadFromFile(in, GameGuiVersion);

  // inv controls
  numguiinv = in->ReadInt32();
  guiinv.resize(numguiinv);

  for (ee = 0; ee < numguiinv; ee++)
    guiinv[ee].ReadFromFile(in, GameGuiVersion);

  if (GameGuiVersion >= kGuiVersion_214) {
    // sliders
    numguislider = in->ReadInt32();
    guislider.resize(numguislider);

    for (ee = 0; ee < numguislider; ee++)
      guislider[ee].ReadFromFile(in, GameGuiVersion);
  }

  if (GameGuiVersion >= kGuiVersion_222) {
    // text boxes
    numguitext = in->ReadInt32();
    guitext.resize(numguitext);

    for (ee = 0; ee < numguitext; ee++)
      guitext[ee].ReadFromFile(in, GameGuiVersion);
  }

  if (GameGuiVersion >= kGuiVersion_230) {
    // list boxes
    numguilist = in->ReadInt32();
    guilist.resize(numguilist);

    for (ee = 0; ee < numguilist; ee++)
      guilist[ee].ReadFromFile(in, GameGuiVersion);
  }

  // set up the reverse-lookup array
  for (ee = 0; ee < gss->numgui; ee++) {
    guiread[ee].RebuildArray();

    if (GameGuiVersion < kGuiVersion_270)
      guiread[ee].OnClickHandler.Empty();

    for (int ff = 0; ff < guiread[ee].ControlCount; ff++) {
      guiread[ee].Controls[ff]->guin = ee;
      guiread[ee].Controls[ff]->objn = ff;

      if (GameGuiVersion < kGuiVersion_272e)
        guiread[ee].Controls[ff]->zorder = ff;
    }

    guiread[ee].ResortZOrder();
  }

  guis_need_update = 1;
}

void write_gui(Stream *out, const std::vector<GUIMain> &guiwrite, GameSetupStruct * gss, bool savedgame)
{
  int ee;

  out->WriteInt32(GUIMAGIC);

  GuiVersion write_version;
  if (savedgame)
    write_version = GameGuiVersion > kGuiVersion_ForwardCompatible ? GameGuiVersion : kGuiVersion_ForwardCompatible;
  else
    write_version = kGuiVersion_Current;

  out->WriteInt32(write_version);
  out->WriteInt32(gss->numgui);

  for (int iteratorCount = 0; iteratorCount < gss->numgui; ++iteratorCount)
  {
    guiwrite[iteratorCount].WriteToFile(out, write_version);
  }

  out->WriteInt32(numguibuts);
  for (ee = 0; ee < numguibuts; ee++)
    guibuts[ee].WriteToFile(out);

  out->WriteInt32(numguilabels);
  for (ee = 0; ee < numguilabels; ee++)
    guilabels[ee].WriteToFile(out);

  out->WriteInt32(numguiinv);
  for (ee = 0; ee < numguiinv; ee++)
    guiinv[ee].WriteToFile(out);

  out->WriteInt32(numguislider);
  for (ee = 0; ee < numguislider; ee++)
    guislider[ee].WriteToFile(out);

  out->WriteInt32(numguitext);
  for (ee = 0; ee < numguitext; ee++)
    guitext[ee].WriteToFile(out);

  out->WriteInt32(numguilist);
  for (ee = 0; ee < numguilist; ee++)
    guilist[ee].WriteToFile(out);
}
