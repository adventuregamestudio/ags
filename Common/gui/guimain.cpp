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
#include "gui/guimain.h"
#include <algorithm>
#include "ac/game_version.h"
#include "ac/spritecache.h"
#include "debug/out.h"
#include "font/fonts.h"
#include "gui/guibutton.h"
#include "gui/guiinv.h"
#include "gui/guilabel.h"
#include "gui/guilistbox.h"
#include "gui/guislider.h"
#include "gui/guitextbox.h"
#include "util/stream.h"
#include "util/string_utils.h"
#include "util/string_compat.h"

using namespace AGS::Common;

#define MOVER_MOUSEDOWNLOCKED -4000

int all_buttons_disabled = -1, gui_inv_pic = -1;

namespace AGS
{
namespace Common
{

GuiOptions GUI::Options;


GUIMain::GUIMain()
{
    InitDefaults();
}

// TODO: remove and use constructor instead,
// assign constructor result to object when want to clean it up (gui = GUIMain();)
void GUIMain::InitDefaults()
{
    ID            = 0;
    Name.Empty();
    _flags        = kGUIMain_DefFlags;
    _hasChanged   = true; // must be updated after creation
    _hasControlsChanged = true;
    _polling      = false;

    X             = 0;
    Y             = 0;
    Width         = 0;
    Height        = 0;
    BgColor       = 8;
    BgImage       = 0;
    FgColor       = 1;
    Padding       = TEXTWINDOW_PADDING_DEFAULT;
    PopupStyle    = kGUIPopupNormal;
    PopupAtMouseY = -1;
    Transparency  = 0;
    ZOrder        = -1;

    FocusCtrl     = 0;
    HighlightCtrl = -1;
    MouseOverCtrl = -1;
    MouseDownCtrl = -1;
    MouseWasAt.X  = -1;
    MouseWasAt.Y  = -1;

    BlendMode     = kBlend_Normal;
    Rotation      = 0.f;

    OnClickHandler.Empty();

    _controls.clear();
    _ctrlRefs.clear();
    _ctrlDrawOrder.clear();

    UpdateGraphicSpace();
}

int GUIMain::FindControlAt(int atx, int aty, int leeway, bool must_be_clickable) const
{
    // translate to GUI's local coordinates
    Point pt = _gs.WorldToLocal(atx, aty);
    return FindControlAtLocal(pt.X, pt.Y, leeway, must_be_clickable);
}

int32_t GUIMain::FindControlAtLocal(int atx, int aty, int leeway, bool must_be_clickable) const
{
    for (size_t i = _controls.size(); i-- > 0;)
    {
        const int ctrl_index = _ctrlDrawOrder[i];
        if (!_controls[ctrl_index]->IsVisible())
            continue;
        if (!_controls[ctrl_index]->IsClickable() && must_be_clickable)
            continue;
        if (_controls[ctrl_index]->IsOverControl(atx, aty, leeway))
            return ctrl_index;
    }
    return -1;
}

int GUIMain::GetControlCount() const
{
    return (int32_t)_controls.size();
}

GUIObject *GUIMain::GetControl(int index) const
{
    if (index < 0 || (size_t)index >= _controls.size())
        return nullptr;
    return _controls[index];
}

GUIControlType GUIMain::GetControlType(int index) const
{
    if (index < 0 || (size_t)index >= _ctrlRefs.size())
        return kGUIControlUndefined;
    return _ctrlRefs[index].first;
}

int32_t GUIMain::GetControlID(int index) const
{
    if (index < 0 || (size_t)index >= _ctrlRefs.size())
        return -1;
    return _ctrlRefs[index].second;
}

const std::vector<int> &GUIMain::GetControlsDrawOrder() const
{
    return _ctrlDrawOrder;
}

bool GUIMain::IsInteractableAt(int x, int y) const
{
    if (!IsDisplayed())
        return false;
    // The Transparency test was unintentionally added in 3.5.0 as a side effect,
    // and unfortunately there are already games which require it to work.
    if ((game_compiled_version.AsNumber() == 30500) &&
        (Transparency == 255))
        return false;
    if (!IsClickable())
        return false;

    // transform to GUI's local coordinates
    Point pt = _gs.WorldToLocal(x, y);
    return ((pt.X >= 0) && (pt.Y >= 0) && (pt.X < Width) && (pt.Y < Height));
}

void GUIMain::MarkChanged()
{
    _hasChanged = true;
}

void GUIMain::MarkControlChanged()
{
    _hasControlsChanged = true;
}

void GUIMain::NotifyControlPosition()
{
    // Force it to re-check for which control is under the mouse
    MouseWasAt.X = -1;
    MouseWasAt.Y = -1;
    _hasControlsChanged = true; // for software render, and in case of shape change
}

void GUIMain::NotifyControlState(int objid, bool mark_changed)
{
    MouseWasAt.X = -1;
    MouseWasAt.Y = -1;
    _hasControlsChanged |= mark_changed;
    // Update cursor-over-control state, if necessary
    const int overctrl = MouseOverCtrl;
    if (!_polling &&
        (objid >= 0) && (objid == overctrl) && ((size_t)objid < _controls.size()) &&
        (!_controls[overctrl]->IsClickable() ||
            !_controls[overctrl]->IsVisible() ||
            !_controls[overctrl]->IsEnabled()))
    {
        MouseOverCtrl = -1;
        _controls[overctrl]->OnMouseLeave();
    }
}

void GUIMain::ClearChanged()
{
    _hasChanged = false;
    _hasControlsChanged = false;
}

void GUIMain::ResetOverControl()
{
    if ((MouseOverCtrl >= 0) && ((size_t)MouseOverCtrl < _controls.size()))
        _controls[MouseOverCtrl]->OnMouseLeave();
    // Force it to re-check for which control is under the mouse
    MouseWasAt.X = -1;
    MouseWasAt.Y = -1;
    MouseOverCtrl = -1;
}

void GUIMain::AddControl(GUIControlType type, int id, GUIObject *control)
{
    _ctrlRefs.emplace_back(type, id);
    _controls.push_back(control);
}

void GUIMain::RemoveAllControls()
{
    _ctrlRefs.clear();
    _controls.clear();
}

bool GUIMain::BringControlToFront(int index)
{
    return SetControlZOrder(index, (int)_controls.size() - 1);
}

void GUIMain::DrawSelf(Bitmap *ds)
{
    SET_EIP(375);

    if ((Width < 1) || (Height < 1))
        return;

    SET_EIP(376);
    // stop border being transparent, if the whole GUI isn't
    if ((FgColor == 0) && (BgColor != 0))
        FgColor = 16;

    if (BgColor != 0)
        ds->Fill(ds->GetCompatibleColor(BgColor));

    SET_EIP(377);

    color_t draw_color;
    if (FgColor != BgColor)
    {
        draw_color = ds->GetCompatibleColor(FgColor);
        ds->DrawRect(Rect(0, 0, ds->GetWidth() - 1, ds->GetHeight() - 1), draw_color);
    }

    SET_EIP(378);

    if (BgImage > 0 && spriteset.DoesSpriteExist(BgImage))
        draw_gui_sprite(ds, BgImage, 0, 0);

    SET_EIP(379);
}

void GUIMain::DrawWithControls(Bitmap *ds)
{
    ds->ResetClip();
    DrawSelf(ds);

    if ((all_buttons_disabled >= 0) && (GUI::Options.DisabledStyle == kGuiDis_Blackout))
        return; // don't draw GUI controls

    Bitmap tempbmp; // in case we need transforms
    for (size_t ctrl_index = 0; ctrl_index < _controls.size(); ++ctrl_index)
    {
        set_eip_guiobj(_ctrlDrawOrder[ctrl_index]);

        GUIObject *objToDraw = _controls[_ctrlDrawOrder[ctrl_index]];
        Size obj_size = objToDraw->GetSize();

        if (!objToDraw->IsVisible() || (obj_size.Width <= 0 || obj_size.Height <= 0))
            continue;
        if (!objToDraw->IsEnabled() && (GUI::Options.DisabledStyle == kGuiDis_Blackout))
            continue;

        // Depending on draw properties - draw directly on the gui surface, or use a buffer
        if (objToDraw->GetTransparency() == 0)
        {
            if (GUI::Options.ClipControls && objToDraw->IsContentClipped())
                ds->SetClip(RectWH(objToDraw->X, objToDraw->Y, obj_size.Width, obj_size.Height));
            else
                ds->ResetClip();
            objToDraw->Draw(ds, objToDraw->X, objToDraw->Y);
        }
        else
        {
            const Rect rc = objToDraw->CalcGraphicRect(GUI::Options.ClipControls && objToDraw->IsContentClipped());
            tempbmp.CreateTransparent(rc.GetWidth(), rc.GetHeight());
            objToDraw->Draw(&tempbmp, -rc.Left, -rc.Top);
            draw_gui_sprite(ds, objToDraw->X + rc.Left, objToDraw->Y + rc.Top,
                &tempbmp, kBlend_Normal,
                GfxDef::LegacyTrans255ToAlpha255(objToDraw->GetTransparency()));
        }

        int selectedColour = 14;

        if (HighlightCtrl == _ctrlDrawOrder[ctrl_index])
        {
            if (GUI::Options.OutlineControls)
                selectedColour = 13;
            color_t draw_color = ds->GetCompatibleColor(selectedColour);
            DrawBlob(ds, objToDraw->X + obj_size.Width - 1 - 1, objToDraw->Y, draw_color);
            DrawBlob(ds, objToDraw->X, objToDraw->Y + obj_size.Height - 1 - 1, draw_color);
            DrawBlob(ds, objToDraw->X, objToDraw->Y, draw_color);
            DrawBlob(ds, objToDraw->X + obj_size.Width - 1 - 1,
                    objToDraw->Y + obj_size.Height - 1 - 1, draw_color);
        }
        if (GUI::Options.OutlineControls)
        {
            // draw a dotted outline round all objects
            color_t draw_color = ds->GetCompatibleColor(selectedColour);
            for (int i = 0; i < obj_size.Width; i += 2)
            {
                ds->PutPixel(i + objToDraw->X, objToDraw->Y, draw_color);
                ds->PutPixel(i + objToDraw->X, objToDraw->Y + obj_size.Height - 1, draw_color);
            }
            for (int i = 0; i < obj_size.Height; i += 2)
            {
                ds->PutPixel(objToDraw->X, i + objToDraw->Y, draw_color);
                ds->PutPixel(objToDraw->X + obj_size.Width - 1, i + objToDraw->Y, draw_color);
            }
        }
    }

    SET_EIP(380);
}

void GUIMain::DrawBlob(Bitmap *ds, int x, int y, color_t draw_color)
{
    ds->FillRect(Rect(x, y, x + 1, y + 1), draw_color);
}

void GUIMain::UpdateGraphicSpace()
{
    _gs = GraphicSpace(X, Y, Width, Height, Width, Height, Rotation);
}

void GUIMain::Poll(int mx, int my)
{
    _polling = true;
    // transform to GUI's local coordinates
    Point pt = _gs.WorldToLocal(mx, my);
    mx = pt.X; my = pt.Y;

    if (mx != MouseWasAt.X || my != MouseWasAt.Y)
    {
        int ctrl_index = FindControlAtLocal(mx, my, 0, true);

        if (MouseOverCtrl == MOVER_MOUSEDOWNLOCKED)
            _controls[MouseDownCtrl]->OnMouseMove(mx, my);
        else if (ctrl_index != MouseOverCtrl)
        {
            if (MouseOverCtrl >= 0)
                _controls[MouseOverCtrl]->OnMouseLeave();

            if (ctrl_index >= 0 && !IsGUIEnabled(_controls[ctrl_index]))
                // the control is disabled - ignore it
                MouseOverCtrl = -1;
            else if (ctrl_index >= 0 && !_controls[ctrl_index]->IsClickable())
                // the control is not clickable - ignore it
                MouseOverCtrl = -1;
            else
            {
                // over a different control
                MouseOverCtrl = ctrl_index;
                if (MouseOverCtrl >= 0)
                {
                    _controls[MouseOverCtrl]->OnMouseEnter();
                    _controls[MouseOverCtrl]->OnMouseMove(mx, my);
                }
            }
        } 
        else if (MouseOverCtrl >= 0)
            _controls[MouseOverCtrl]->OnMouseMove(mx, my);
    }

    MouseWasAt.X = mx;
    MouseWasAt.Y = my;
    _polling = false;
}

HError GUIMain::RebuildArray()
{
    GUIControlType thistype;
    int32_t thisnum;

    _controls.resize(_ctrlRefs.size());
    for (size_t i = 0; i < _controls.size(); ++i)
    {
        thistype = _ctrlRefs[i].first;
        thisnum = _ctrlRefs[i].second;

        if (thisnum < 0)
            return new Error(String::FromFormat("GUIMain (%d): invalid control ID %d in ref #%d", ID, thisnum, i));

        if (thistype == kGUIButton)
            _controls[i] = &guibuts[thisnum];
        else if (thistype == kGUILabel)
            _controls[i] = &guilabels[thisnum];
        else if (thistype == kGUIInvWindow)
            _controls[i] = &guiinv[thisnum];
        else if (thistype == kGUISlider)
            _controls[i] = &guislider[thisnum];
        else if (thistype == kGUITextBox)
            _controls[i] = &guitext[thisnum];
        else if (thistype == kGUIListBox)
            _controls[i] = &guilist[thisnum];
        else
            return new Error(String::FromFormat("GUIMain (%d): unknown control type %d in ref #%d", ID, thistype, i));

        _controls[i]->ParentId = ID;
        _controls[i]->Id = i;
    }

    ResortZOrder();
    return HError::None();
}

bool GUIControlZOrder(const GUIObject *e1, const GUIObject *e2)
{
    return e1->ZOrder < e2->ZOrder;
}

void GUIMain::ResortZOrder()
{
    std::vector<GUIObject*> ctrl_sort = _controls;
    std::sort(ctrl_sort.begin(), ctrl_sort.end(), GUIControlZOrder);

    _ctrlDrawOrder.resize(ctrl_sort.size());
    for (size_t i = 0; i < ctrl_sort.size(); ++i)
        _ctrlDrawOrder[i] = ctrl_sort[i]->Id;
}

void GUIMain::SetAt(int x, int y)
{
    X = x;
    Y = y;
    UpdateGraphicSpace();
}

void GUIMain::SetClickable(bool on)
{
    if (on != ((_flags & kGUIMain_Clickable) != 0))
    {
        _flags = (_flags & ~kGUIMain_Clickable) | kGUIMain_Clickable * on;
        ResetOverControl(); // clear the cursor-over-control
    }
}

void GUIMain::SetConceal(bool on)
{
    if (on != ((_flags & kGUIMain_Concealed) != 0))
    {
        _flags = (_flags & ~kGUIMain_Concealed) | kGUIMain_Concealed * on;
        ResetOverControl(); // clear the cursor-over-control
    }
}

bool GUIMain::SendControlToBack(int index)
{
    return SetControlZOrder(index, 0);
}

bool GUIMain::SetControlZOrder(int index, int zorder)
{
    if (index < 0 || (size_t)index >= _controls.size())
        return false; // no such control

    zorder = Math::Clamp(zorder, 0, (int)_controls.size() - 1);
    const int old_zorder = _controls[index]->ZOrder;
    if (old_zorder == zorder)
        return false; // no change

    const bool move_back = zorder < old_zorder; // back is at zero index
    const int  left      = move_back ? zorder : old_zorder;
    const int  right     = move_back ? old_zorder : zorder;
    for (size_t i = 0; i < _controls.size(); ++i)
    {
        const int i_zorder = _controls[i]->ZOrder;
        if (i_zorder == old_zorder)
            _controls[i]->ZOrder = zorder; // the control we are moving
        else if (i_zorder >= left && i_zorder <= right)
        {
            // controls in between old and new positions shift towards free place
            if (move_back)
                _controls[i]->ZOrder++; // move to front
            else
                _controls[i]->ZOrder--; // move to back
        }
    }
    ResortZOrder();
    NotifyControlPosition();
    return true;
}

void GUIMain::SetRotation(float degrees)
{
    Rotation = Math::ClampAngle360(degrees);
    MarkChanged();
    UpdateGraphicSpace();
}

void GUIMain::SetSize(int w, int h)
{
    Width = w;
    Height = h;
    UpdateGraphicSpace();
}

void GUIMain::SetTextWindow(bool on)
{
    _flags = (_flags & ~kGUIMain_TextWindow) | kGUIMain_TextWindow * on;
}

void GUIMain::SetTransparencyAsPercentage(int percent)
{
    Transparency = GfxDef::Trans100ToLegacyTrans255(percent);
}

void GUIMain::SetVisible(bool on)
{
    if (on != ((_flags & kGUIMain_Visible) != 0))
    {
        _flags = (_flags & ~kGUIMain_Visible) | kGUIMain_Visible * on;
        ResetOverControl(); // clear the cursor-over-control
    }
}

void GUIMain::OnMouseButtonDown(int mx, int my)
{
    if (MouseOverCtrl < 0)
        return;

    // don't activate disabled buttons
    if (!IsGUIEnabled(_controls[MouseOverCtrl]) || !_controls[MouseOverCtrl]->IsVisible() ||
        !_controls[MouseOverCtrl]->IsClickable())
        return;

    // transform to GUI's local coordinates
    Point pt = _gs.WorldToLocal(mx, my);

    MouseDownCtrl = MouseOverCtrl;
    if (_controls[MouseOverCtrl]->OnMouseDown())
        MouseOverCtrl = MOVER_MOUSEDOWNLOCKED;
    _controls[MouseDownCtrl]->OnMouseMove(pt.X, pt.Y);
}

void GUIMain::OnMouseButtonUp()
{
    // FocusCtrl was locked - reset it back to normal, but On the
    // locked object so that a OnMouseLeave gets fired if necessary
    if (MouseOverCtrl == MOVER_MOUSEDOWNLOCKED)
    {
        MouseOverCtrl = MouseDownCtrl;
        MouseWasAt.X = -1;  // force update
    }

    if (MouseDownCtrl < 0)
        return;

    _controls[MouseDownCtrl]->OnMouseUp();
    MouseDownCtrl = -1;
}

void GUIMain::ReadFromFile(Stream *in, GuiVersion gui_version)
{
    Name = StrUtil::ReadString(in);
    OnClickHandler = StrUtil::ReadString(in);
    X             = in->ReadInt32();
    Y             = in->ReadInt32();
    Width         = in->ReadInt32();
    Height        = in->ReadInt32();
    const size_t ctrl_count = in->ReadInt32();
    PopupStyle    = (GUIPopupStyle)in->ReadInt32();
    PopupAtMouseY = in->ReadInt32();
    BgColor       = in->ReadInt32();
    BgImage       = in->ReadInt32();
    FgColor       = in->ReadInt32();
    _flags         = in->ReadInt32();
    Transparency  = in->ReadInt32();
    ZOrder        = in->ReadInt32();
    ID            = in->ReadInt32();
    Padding       = in->ReadInt32();

    if (ctrl_count > 0)
    {
        _ctrlRefs.resize(ctrl_count);
        for (size_t i = 0; i < ctrl_count; ++i)
        {
            const int32_t ref_packed = in->ReadInt32();
            _ctrlRefs[i].first = (GUIControlType)((ref_packed >> 16) & 0xFFFF);
            _ctrlRefs[i].second = ref_packed & 0xFFFF;
        }
    }

    UpdateGraphicSpace();
}

void GUIMain::WriteToFile(Stream *out) const
{
    StrUtil::WriteString(Name, out);
    StrUtil::WriteString(OnClickHandler, out);
    out->WriteInt32(X);
    out->WriteInt32(Y);
    out->WriteInt32(Width);
    out->WriteInt32(Height);
    out->WriteInt32(_ctrlRefs.size());
    out->WriteInt32(PopupStyle);
    out->WriteInt32(PopupAtMouseY);
    out->WriteInt32(BgColor);
    out->WriteInt32(BgImage);
    out->WriteInt32(FgColor);
    out->WriteInt32(_flags);
    out->WriteInt32(Transparency);
    out->WriteInt32(ZOrder);
    out->WriteInt32(ID);
    out->WriteInt32(Padding);
    for (size_t i = 0; i < _ctrlRefs.size(); ++i)
    {
        int32_t ref_packed = ((_ctrlRefs[i].first & 0xFFFF) << 16) | (_ctrlRefs[i].second & 0xFFFF);
        out->WriteInt32(ref_packed);
    }
}

void GUIMain::ReadFromSavegame(Common::Stream *in, GuiSvgVersion svg_version)
{
    // Properties
    _flags = in->ReadInt32();
    X = in->ReadInt32();
    Y = in->ReadInt32();
    Width = in->ReadInt32();
    Height = in->ReadInt32();
    BgImage = in->ReadInt32();
    Transparency = in->ReadInt32();
    ZOrder = in->ReadInt32();

    if (svg_version >= kGuiSvgVersion_350)
    {
        BgColor = in->ReadInt32();
        FgColor = in->ReadInt32();
        Padding = in->ReadInt32();
        PopupAtMouseY = in->ReadInt32();
    }

    // Dynamic values
    FocusCtrl = in->ReadInt32();
    HighlightCtrl = in->ReadInt32();
    MouseOverCtrl = in->ReadInt32();
    MouseDownCtrl = in->ReadInt32();
    MouseWasAt.X = in->ReadInt32();
    MouseWasAt.Y = in->ReadInt32();

    if (svg_version >= kGuiSvgVersion_400)
    {
        BlendMode = (Common::BlendMode)in->ReadInt32();

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
        Rotation = in->ReadFloat32(); // transform rotate
        in->ReadInt32(); // sprite pivot x
        in->ReadInt32(); // sprite pivot y
        in->ReadInt32(); // sprite anchor x
        in->ReadInt32(); // sprite anchor y
    }

    UpdateGraphicSpace();
}

void GUIMain::WriteToSavegame(Common::Stream *out) const
{
    // Properties
    out->WriteInt32(_flags);
    out->WriteInt32(X);
    out->WriteInt32(Y);
    out->WriteInt32(Width);
    out->WriteInt32(Height);
    out->WriteInt32(BgImage);
    out->WriteInt32(Transparency);
    out->WriteInt32(ZOrder);
    out->WriteInt32(BgColor);
    out->WriteInt32(FgColor);
    out->WriteInt32(Padding);
    out->WriteInt32(PopupAtMouseY);
    // Dynamic values
    out->WriteInt32(FocusCtrl);
    out->WriteInt32(HighlightCtrl);
    out->WriteInt32(MouseOverCtrl);
    out->WriteInt32(MouseDownCtrl);
    out->WriteInt32(MouseWasAt.X);
    out->WriteInt32(MouseWasAt.Y);
    // since version 10 (kGuiSvgVersion_399)
    out->WriteInt32(BlendMode);
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
    out->WriteFloat32(Rotation); // transform rotate
    out->WriteInt32(0); // sprite pivot x
    out->WriteInt32(0); // sprite pivot y
    out->WriteInt32(0); // sprite anchor x
    out->WriteInt32(0); // sprite anchor y
}


namespace GUI
{

GuiVersion GameGuiVersion = kGuiVersion_Undefined;

Rect CalcTextPosition(const char *text, int font, const Rect &frame, FrameAlignment align)
{
    int use_height = get_font_height_outlined(font);
    Rect rc = AlignInRect(frame, RectWH(0, 0, get_text_width_outlined(text, font), use_height), align);
    rc.SetHeight(get_font_surface_height(font));
    return rc;
}

Line CalcTextPositionHor(const char *text, int font, int x1, int x2, int y, FrameAlignment align)
{
    int w = get_text_width_outlined(text, font);
    int x = AlignInHRange(x1, x2, 0, w, align);
    return Line(x, y, x + w - 1, y);
}

void DrawDisabledEffect(Bitmap *ds, const Rect &rc)
{
    color_t draw_color = ds->GetCompatibleColor(8);
    for (int at_x = rc.Left; at_x <= rc.Right; ++at_x)
    {
        for (int at_y = rc.Top + at_x % 2; at_y <= rc.Bottom; at_y += 2)
        {
            ds->PutPixel(at_x, at_y, draw_color);
        }
    }
}

void DrawTextAligned(Bitmap *ds, const char *text, int font, color_t text_color, const Rect &frame, FrameAlignment align)
{
    Rect item = CalcTextPosition(text, font, frame, align);
    wouttext_outline(ds, item.Left, item.Top, font, text_color, text);
}

void DrawTextAlignedHor(Bitmap *ds, const char *text, int font, color_t text_color, int x1, int x2, int y, FrameAlignment align)
{
    Line line = CalcTextPositionHor(text, font, x1, x2, y, align);
    wouttext_outline(ds, line.X1, y, font, text_color, text);
}

void MarkAllGUIForUpdate(bool redraw, bool reset_over_ctrl)
{
    for (auto &gui : guis)
    {
        if (redraw)
        {
            gui.MarkChanged();
            for (int i = 0; i < gui.GetControlCount(); ++i)
                gui.GetControl(i)->MarkChanged();
        }
        if (reset_over_ctrl)
            gui.ResetOverControl();
    }
}

void MarkForTranslationUpdate()
{
    for (auto &btn : guibuts)
    {
        if (btn.IsTranslated())
            btn.MarkChanged();
    }
    for (auto &lbl : guilabels)
    {
        if (lbl.IsTranslated())
            lbl.MarkChanged();
    }
    for (auto &list : guilist)
    {
        if (list.IsTranslated())
            list.MarkChanged();
    }
}

void MarkForFontUpdate(int font)
{
    const bool update_all = (font < 0);
    for (auto &btn : guibuts)
    {
        if (update_all || btn.Font == font)
            btn.OnResized();
    }
    for (auto &lbl : guilabels)
    {
        if (update_all || lbl.Font == font)
            lbl.OnResized();
    }
    for (auto &list : guilist)
    {
        if (update_all || list.Font == font)
            list.OnResized();
    }
    for (auto &tb : guitext)
    {
        if (update_all || tb.Font == font)
            tb.OnResized();
    }
}

void MarkSpecialLabelsForUpdate(GUILabelMacro macro)
{
    for (auto &lbl : guilabels)
    {
        if ((lbl.GetTextMacros() & macro) != 0)
        {
            lbl.MarkChanged();
        }
    }
}

void MarkInventoryForUpdate(int char_id, bool is_player)
{
    for (auto &btn : guibuts)
    {
        if (btn.GetPlaceholder() != kButtonPlace_None)
            btn.MarkChanged();
    }
    for (auto &inv : guiinv)
    {
        if ((char_id < 0) || (inv.CharId == char_id) || (is_player && inv.CharId < 0))
        {
            inv.MarkChanged();
        }
    }
}

GUILabelMacro FindLabelMacros(const String &text)
{
    int macro_flags = 0;
    const char *macro_at = nullptr;
    for (const char *ptr = text.GetCStr(); *ptr; ++ptr)
    {
        // Haven't began parsing macro
        if (!macro_at)
        {
            if (*ptr == '@')
                macro_at = ptr;
        }
        // Began parsing macro
        else
        {
            // Found macro's end
            if (*ptr == '@')
            {
                // Test which macro it is (if any)
                macro_at++;
                const size_t macro_len = ptr - macro_at;
                if (ags_strnicmp(macro_at, "gamename", macro_len) == 0)
                    macro_flags |= kLabelMacro_Gamename;
                else if (ags_strnicmp(macro_at, "overhotspot", macro_len) == 0)
                    macro_flags |= kLabelMacro_Overhotspot;
                macro_at = nullptr;
            }
        }
    }
    return (GUILabelMacro)macro_flags;
}

HError RebuildGUI()
{
    // set up the reverse-lookup array
    for (auto &gui : guis)
    {
        HError err = gui.RebuildArray();
        if (!err)
            return err;
        for (int ctrl_index = 0; ctrl_index < gui.GetControlCount(); ++ctrl_index)
        {
            GUIObject *gui_ctrl = gui.GetControl(ctrl_index);
            gui_ctrl->ParentId = gui.ID;
            gui_ctrl->Id = ctrl_index;
        }
        gui.ResortZOrder();
    }
    MarkAllGUIForUpdate(true, true);
    return HError::None();
}

HError ReadGUI(Stream *in)
{
    if (in->ReadInt32() != (int)GUIMAGIC)
        return new Error("ReadGUI: unknown format or file is corrupt");

    GameGuiVersion = (GuiVersion)in->ReadInt32();
    Debug::Printf(kDbgMsg_Info, "Game GUI version: %d", GameGuiVersion);
    if (GameGuiVersion < GameGuiVersion, GameGuiVersion > kGuiVersion_Current)
        return new Error(String::FromFormat("ReadGUI: format version not supported (required %d, supported %d - %d)",
            GameGuiVersion, kGuiVersion_LowSupported, kGuiVersion_Current));

    size_t gui_count = in->ReadInt32();
    guis.resize(gui_count);

    // import the main GUI elements
    for (size_t i = 0; i < gui_count; ++i)
    {
        GUIMain &gui = guis[i];
        gui.InitDefaults();
        gui.ReadFromFile(in, GameGuiVersion);

        // perform fixups
        if (gui.Height < 2)
            gui.SetSize(gui.Width, 2);

        // PopupMouseY GUIs should be initially concealed
        if (gui.PopupStyle == kGUIPopupMouseY)
            gui.SetConceal(true);
        // Assign ID to order in array
        gui.ID = i;
    }

    // buttons
    size_t numguibuts = static_cast<uint32_t>(in->ReadInt32());
    guibuts.resize(numguibuts);
    for (size_t i = 0; i < numguibuts; ++i)
    {
        guibuts[i].ReadFromFile(in, GameGuiVersion);
    }
    // labels
    size_t numguilabels = static_cast<uint32_t>(in->ReadInt32());
    guilabels.resize(numguilabels);
    for (size_t i = 0; i < numguilabels; ++i)
    {
        guilabels[i].ReadFromFile(in, GameGuiVersion);
    }
    // inv controls
    size_t numguiinv = static_cast<uint32_t>(in->ReadInt32());
    guiinv.resize(numguiinv);
    for (size_t i = 0; i < numguiinv; ++i)
    {
        guiinv[i].ReadFromFile(in, GameGuiVersion);
    }
    // sliders
    size_t numguislider = static_cast<uint32_t>(in->ReadInt32());
    guislider.resize(numguislider);
    for (size_t i = 0; i < numguislider; ++i)
    {
        guislider[i].ReadFromFile(in, GameGuiVersion);
    }
    // text boxes
    size_t numguitext = static_cast<uint32_t>(in->ReadInt32());
    guitext.resize(numguitext);
    for (size_t i = 0; i < numguitext; ++i)
    {
        guitext[i].ReadFromFile(in, GameGuiVersion);
    }
    // list boxes
    size_t numguilist = static_cast<uint32_t>(in->ReadInt32());
    guilist.resize(numguilist);
    for (size_t i = 0; i < numguilist; ++i)
    {
        guilist[i].ReadFromFile(in, GameGuiVersion);
    }

    return RebuildGUI();
}

void WriteGUI(Stream *out)
{
    out->WriteInt32(GUIMAGIC);
    out->WriteInt32(kGuiVersion_Current);
    out->WriteInt32(guis.size());

    for (const auto &gui : guis)
    {
        gui.WriteToFile(out);
    }
    out->WriteInt32(static_cast<int32_t>(guibuts.size()));
    for (const auto &but : guibuts)
    {
        but.WriteToFile(out);
    }
    out->WriteInt32(static_cast<int32_t>(guilabels.size()));
    for (const auto &label : guilabels)
    {
        label.WriteToFile(out);
    }
    out->WriteInt32(static_cast<int32_t>(guiinv.size()));
    for (const auto &inv : guiinv)
    {
        inv.WriteToFile(out);
    }
    out->WriteInt32(static_cast<int32_t>(guislider.size()));
    for (const auto &slider : guislider)
    {
        slider.WriteToFile(out);
    }
    out->WriteInt32(static_cast<int32_t>(guitext.size()));
    for (const auto &tb : guitext)
    {
        tb.WriteToFile(out);
    }
    out->WriteInt32(static_cast<int32_t>(guilist.size()));
    for (const auto &list : guilist)
    {
        list.WriteToFile(out);
    }
}

} // namespace GUI

} // namespace Common
} // namespace AGS
