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
#include "ac/common.h"
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

#define MOVER_MOUSEDOWNLOCKED -4000

namespace AGS
{
namespace Common
{

GuiOptions GUI::Options;
GuiContext GUI::Context;

void GUIMain::SetX(int x)
{
    _x = x;
}

void GUIMain::SetY(int y)
{
    _y = y;
}

void GUIMain::SetWidth(int width)
{
    SetSize(width, _height);
}

void GUIMain::SetHeight(int height)
{
    SetSize(_width, height);
}

void GUIMain::SetPosition(int x, int y)
{
    _x = x;
    _y = y;
}

void GUIMain::SetSize(int width, int height)
{
    if (_width != width || _height != height)
    {
        _width = width;
        _height = height;
        MarkChanged();
    }
}

void GUIMain::SetBgColor(int color)
{
    if (_bgColor != color)
    {
        _bgColor = color;
        MarkChanged();
    }
}

void GUIMain::SetFgColor(int color)
{
    if (_fgColor != color)
    {
        _fgColor = color;
        MarkChanged();
    }
}

void GUIMain::SetBgImage(int image)
{
    if (_bgImage != image)
    {
        _bgImage = image;
        MarkChanged();
    }
}

void GUIMain::SetPopupStyle(GUIPopupStyle style)
{
    _popupStyle = style;
}

void GUIMain::SetPopupAtY(int popup_aty)
{
    _popupAtMouseY = popup_aty;
}

void GUIMain::SetPadding(int padding)
{
    _padding = padding;
}

void GUIMain::SetTransparency(int trans)
{
    _transparency = trans;
}

void GUIMain::SetZOrder(int zorder)
{
    _zOrder = zorder;
}

void GUIMain::SetScriptModule(const String &scmodule)
{
    _scriptModule = scmodule;
}

void GUIMain::SetOnClickHandler(const String &handler)
{
    _onClickHandler = handler;
}

int GUIMain::FindControlAt(int atx, int aty, int leeway, bool must_be_clickable) const
{
    // translate to GUI's local coordinates
    return FindControlAtLocal(atx - _x, aty - _y, leeway, must_be_clickable);
}

int GUIMain::FindControlAtLocal(int atx, int aty, int leeway, bool must_be_clickable) const
{
    if (loaded_game_file_version <= kGameVersion_262)
    {
        // Ignore draw order On 2.6.2 and lower
        for (size_t i = 0; i < _controls.size(); ++i)
        {
            if (!_controls[i]->IsVisible())
                continue;
            if (!_controls[i]->IsClickable() && must_be_clickable)
                continue;
            if (_controls[i]->IsOverControl(atx, aty, leeway))
                return i;
        }
    }
    else
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
    }
    return -1;
}

int GUIMain::GetControlUnderMouse() const
{
    if (_mouseOverCtrl >= 0)
        return _mouseOverCtrl;
    else if (_mouseOverCtrl == MOVER_MOUSEDOWNLOCKED)
        return _mouseDownCtrl;
    else
        return -1;
}

int GUIMain::GetControlCount() const
{
    return static_cast<int>(_controls.size());
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

int GUIMain::GetControlID(int index) const
{
    if (index < 0 || (size_t)index >= _ctrlRefs.size())
        return -1;
    return _ctrlRefs[index].second;
}

const std::vector<int> &GUIMain::GetControlsDrawOrder() const
{
    return _ctrlDrawOrder;
}

const std::vector<GUIMain::ControlRef> &GUIMain::GetControlRefs() const
{
    return _ctrlRefs;
}

bool GUIMain::IsInteractableAt(int x, int y) const
{
    if (!IsDisplayed())
        return false;
    // The _transparency test was unintentionally added in 3.5.0 as a side effect,
    // and unfortunately there are already games which require it to work.
    if ((game_compiled_version.AsNumber() == 30500) &&
        (_transparency == 255))
        return false;
    if (!IsClickable())
        return false;
    if ((x >= _x) && (y >= _y) && (x < _x + _width) && (y < _y + _height))
        return true;
    return false;
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
    _mouseWasAt.X = -1;
    _mouseWasAt.Y = -1;
    _hasControlsChanged = true; // for software render, and in case of shape change
}

void GUIMain::NotifyControlState(int objid, bool mark_changed)
{
    _mouseWasAt.X = -1;
    _mouseWasAt.Y = -1;
    _hasControlsChanged |= mark_changed;
    // Update cursor-over-control state, if necessary
    const int overctrl = _mouseOverCtrl;
    if (!_polling &&
        (objid >= 0) && (objid == overctrl) && ((size_t)objid < _controls.size()) &&
        (!_controls[overctrl]->IsClickable() ||
            !_controls[overctrl]->IsVisible() ||
            !_controls[overctrl]->IsEnabled()))
    {
        _mouseOverCtrl = -1;
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
    if ((_mouseOverCtrl >= 0) && ((size_t)_mouseOverCtrl < _controls.size()))
        _controls[_mouseOverCtrl]->OnMouseLeave();
    // Force it to re-check for which control is under the mouse
    _mouseWasAt.X = -1;
    _mouseWasAt.Y = -1;
    _mouseOverCtrl = -1;
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
    set_our_eip(375);

    if ((_width < 1) || (_height < 1))
        return;

    assert(GUI::Context.Spriteset);
    SpriteCache &spriteset = *GUI::Context.Spriteset;

    set_our_eip(376);
    // stop border being transparent, if the whole GUI isn't
    if ((_fgColor == 0) && (_bgColor != 0))
        _fgColor = 16;

    if (_bgColor != 0)
        ds->Fill(ds->GetCompatibleColor(_bgColor));

    set_our_eip(377);

    color_t draw_color;
    if (_fgColor != _bgColor)
    {
        draw_color = ds->GetCompatibleColor(_fgColor);
        ds->DrawRect(Rect(0, 0, ds->GetWidth() - 1, ds->GetHeight() - 1), draw_color);
        if (get_fixed_pixel_size(1) > 1)
            ds->DrawRect(Rect(1, 1, ds->GetWidth() - 2, ds->GetHeight() - 2), draw_color);
    }

    set_our_eip(378);

    if (_bgImage > 0 && spriteset.DoesSpriteExist(_bgImage))
        draw_gui_sprite(ds, _bgImage, 0, 0, false);

    set_our_eip(379);
}

void GUIMain::DrawWithControls(Bitmap *ds)
{
    ds->ResetClip();
    DrawSelf(ds);
    DrawControls(ds);
}

void GUIMain::DrawControls(Bitmap *ds)
{
    if ((GUI::Context.DisabledState != kGuiDis_Undefined) && (GUI::Options.DisabledStyle == kGuiDis_Blackout))
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

        if (GUI::Options.ClipControls && objToDraw->IsContentClipped())
            ds->SetClip(objToDraw->GetRect());
        else
            ds->ResetClip();

        const int objx = objToDraw->GetX();
        const int objy = objToDraw->GetY();

        // Depending on draw properties - draw directly on the gui surface, or use a buffer
        if (objToDraw->GetTransparency() == 0)
        {
            objToDraw->Draw(ds, objx, objy);
        }
        else
        {
            const Rect rc = objToDraw->CalcGraphicRect(GUI::Options.ClipControls && objToDraw->IsContentClipped());
            tempbmp.CreateTransparent(rc.GetWidth(), rc.GetHeight());
            objToDraw->Draw(&tempbmp, -rc.Left, -rc.Top);
            draw_gui_sprite(ds, true, objx + rc.Left, objy + rc.Top,
                &tempbmp, objToDraw->HasAlphaChannel(), kBlendMode_Alpha,
                GfxDef::LegacyTrans255ToAlpha255(objToDraw->GetTransparency()));
        }

        int selectedColour = 14;

        if (_highlightCtrl == _ctrlDrawOrder[ctrl_index])
        {
            if (GUI::Options.OutlineControls)
                selectedColour = 13;
            color_t draw_color = ds->GetCompatibleColor(selectedColour);
            DrawBlob(ds, objx + obj_size.Width - get_fixed_pixel_size(1) - 1, objy, draw_color);
            DrawBlob(ds, objx, objy + obj_size.Height - get_fixed_pixel_size(1) - 1, draw_color);
            DrawBlob(ds, objx, objy, draw_color);
            DrawBlob(ds, objx + obj_size.Width - get_fixed_pixel_size(1) - 1,
                    objy + obj_size.Height - get_fixed_pixel_size(1) - 1, draw_color);
        }
        if (GUI::Options.OutlineControls)
        {
            // draw a dotted outline round all objects
            color_t draw_color = ds->GetCompatibleColor(selectedColour);
            for (int i = 0; i < obj_size.Width; i += 2)
            {
                ds->PutPixel(i + objx, objy, draw_color);
                ds->PutPixel(i + objx, objy + obj_size.Height - 1, draw_color);
            }
            for (int i = 0; i < obj_size.Height; i += 2)
            {
                ds->PutPixel(objx, i + objy, draw_color);
                ds->PutPixel(objx + obj_size.Width - 1, i + objy, draw_color);
            }
        }
    }

    set_our_eip(380);
}

void GUIMain::DrawBlob(Bitmap *ds, int x, int y, color_t draw_color)
{
    ds->FillRect(Rect(x, y, x + get_fixed_pixel_size(1), y + get_fixed_pixel_size(1)), draw_color);
}

void GUIMain::Poll(int mx, int my)
{
    _polling = true;
    mx -= _x, my -= _y; // translate to GUI's local coordinates
    if (mx != _mouseWasAt.X || my != _mouseWasAt.Y)
    {
        int ctrl_index = FindControlAtLocal(mx, my, 0, true);

        if (_mouseOverCtrl == MOVER_MOUSEDOWNLOCKED)
            _controls[_mouseDownCtrl]->OnMouseMove(mx, my);
        else if (ctrl_index != _mouseOverCtrl)
        {
            if (_mouseOverCtrl >= 0)
                _controls[_mouseOverCtrl]->OnMouseLeave();

            if (ctrl_index >= 0 && !GUI::IsGUIEnabled(_controls[ctrl_index]))
                // the control is disabled - ignore it
                _mouseOverCtrl = -1;
            else if (ctrl_index >= 0 && !_controls[ctrl_index]->IsClickable())
                // the control is not clickable - ignore it
                _mouseOverCtrl = -1;
            else
            {
                // over a different control
                _mouseOverCtrl = ctrl_index;
                if (_mouseOverCtrl >= 0)
                {
                    _controls[_mouseOverCtrl]->OnMouseEnter();
                    _controls[_mouseOverCtrl]->OnMouseMove(mx, my);
                }
            }
        } 
        else if (_mouseOverCtrl >= 0)
            _controls[_mouseOverCtrl]->OnMouseMove(mx, my);
    }

    _mouseWasAt.X = mx;
    _mouseWasAt.Y = my;
    _polling = false;
}

HError GUIMain::RebuildArray(GUIRefCollection &guiobjs)
{
    GUIControlType thistype;
    int thisnum;

    _controls.resize(_ctrlRefs.size());
    for (size_t i = 0; i < _controls.size(); ++i)
    {
        thistype = _ctrlRefs[i].first;
        thisnum = _ctrlRefs[i].second;

        if (thisnum < 0)
            return new Error(String::FromFormat("GUIMain (%d): invalid control _id %d in ref #%d", _id, thisnum, i));

        if (thistype == kGUIButton)
            _controls[i] = &guiobjs.Buttons[thisnum];
        else if (thistype == kGUILabel)
            _controls[i] = &guiobjs.Labels[thisnum];
        else if (thistype == kGUIInvWindow)
            _controls[i] = &guiobjs.InvWindows[thisnum];
        else if (thistype == kGUISlider)
            _controls[i] = &guiobjs.Sliders[thisnum];
        else if (thistype == kGUITextBox)
            _controls[i] = &guiobjs.TextBoxes[thisnum];
        else if (thistype == kGUIListBox)
            _controls[i] = &guiobjs.ListBoxes[thisnum];
        else
            return new Error(String::FromFormat("GUIMain (%d): unknown control type %d in ref #%d", _id, thistype, i));

        _controls[i]->SetParentID(_id);
        _controls[i]->SetID(i);
    }

    ResortZOrder();
    return HError::None();
}

bool GUIControlZOrder(const GUIObject *e1, const GUIObject *e2)
{
    return e1->GetZOrder() < e2->GetZOrder();
}

void GUIMain::ResortZOrder()
{
    std::vector<GUIObject*> ctrl_sort = _controls;
    std::sort(ctrl_sort.begin(), ctrl_sort.end(), GUIControlZOrder);

    _ctrlDrawOrder.resize(ctrl_sort.size());
    for (size_t i = 0; i < ctrl_sort.size(); ++i)
        _ctrlDrawOrder[i] = ctrl_sort[i]->GetID();
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
    const int old_zorder = _controls[index]->GetZOrder();
    if (old_zorder == zorder)
        return false; // no change

    const bool move_back = zorder < old_zorder; // back is at zero index
    const int  left      = move_back ? zorder : old_zorder;
    const int  right     = move_back ? old_zorder : zorder;
    for (size_t i = 0; i < _controls.size(); ++i)
    {
        const int i_zorder = _controls[i]->GetZOrder();
        if (i_zorder == old_zorder)
        {
            _controls[i]->SetZOrder(zorder); // the control we are moving
        }
        else if (i_zorder >= left && i_zorder <= right)
        {
            // controls in between old and new positions shift towards free place
            if (move_back)
                _controls[i]->SetZOrder(i_zorder + 1); // move to front
            else
                _controls[i]->SetZOrder(i_zorder - 1); // move to back
        }
    }
    ResortZOrder();
    NotifyControlPosition();
    return true;
}

void GUIMain::SetTextWindow(bool on)
{
    _flags = (_flags & ~kGUIMain_TextWindow) | kGUIMain_TextWindow * on;
}

void GUIMain::SetTransparencyAsPercentage(int percent)
{
    _transparency = GfxDef::Trans100ToLegacyTrans255(percent);
}

void GUIMain::SetVisible(bool on)
{
    if (on != ((_flags & kGUIMain_Visible) != 0))
    {
        _flags = (_flags & ~kGUIMain_Visible) | kGUIMain_Visible * on;
        ResetOverControl(); // clear the cursor-over-control
    }
}

void GUIMain::SetHighlightControl(int control_index)
{
    _highlightCtrl = control_index;
}

void GUIMain::OnMouseButtonDown(int mx, int my)
{
    if (_mouseOverCtrl < 0)
        return;

    // don't activate disabled buttons
    if (!GUI::IsGUIEnabled(_controls[_mouseOverCtrl]) || !_controls[_mouseOverCtrl]->IsVisible() ||
        !_controls[_mouseOverCtrl]->IsClickable())
        return;

    _mouseDownCtrl = _mouseOverCtrl;
    if (_controls[_mouseOverCtrl]->OnMouseDown())
        _mouseOverCtrl = MOVER_MOUSEDOWNLOCKED;
    _controls[_mouseDownCtrl]->OnMouseMove(mx - _x, my - _y);
}

void GUIMain::OnMouseButtonUp()
{
    // _focusCtrl was locked - reset it back to normal, but On the
    // locked object so that a OnMouseLeave gets fired if necessary
    if (_mouseOverCtrl == MOVER_MOUSEDOWNLOCKED)
    {
        _mouseOverCtrl = _mouseDownCtrl;
        _mouseWasAt.X = -1;  // force update
    }

    if (_mouseDownCtrl < 0)
        return;

    _controls[_mouseDownCtrl]->OnMouseUp();
    _mouseDownCtrl = -1;
}

void GUIMain::ReadFromFile(Stream *in, GuiVersion gui_version)
{
    // Legacy text window tag
    char tw_flags[GUIMAIN_LEGACY_TW_FLAGS_SIZE] = {0};
    if (gui_version < kGuiVersion_350)
        in->Read(tw_flags, sizeof(tw_flags));

    if (gui_version < kGuiVersion_340)
    {
        _name.ReadCount(in, GUIMAIN_LEGACY_NAME_LENGTH);
        _onClickHandler.ReadCount(in, GUIMAIN_LEGACY_EVENTHANDLER_LENGTH);
    }
    else
    {
        _name = StrUtil::ReadString(in);
        _onClickHandler = StrUtil::ReadString(in);
    }
    _x             = in->ReadInt32();
    _y             = in->ReadInt32();
    _width         = in->ReadInt32();
    _height        = in->ReadInt32();
    if (gui_version < kGuiVersion_350)
    { // NOTE: reading into actual variables only for old savegame support
        _focusCtrl = in->ReadInt32();
    }
    const size_t ctrl_count = in->ReadInt32();
    _popupStyle    = (GUIPopupStyle)in->ReadInt32();
    _popupAtMouseY = in->ReadInt32();
    _bgColor       = in->ReadInt32();
    _bgImage       = in->ReadInt32();
    _fgColor       = in->ReadInt32();
    if (gui_version < kGuiVersion_350)
    { // NOTE: reading into actual variables only for old savegame support
        _mouseOverCtrl = in->ReadInt32();
        _mouseWasAt.X  = in->ReadInt32();
        _mouseWasAt.Y  = in->ReadInt32();
        _mouseDownCtrl = in->ReadInt32();
        _highlightCtrl = in->ReadInt32();
    }
    _flags         = in->ReadInt32();
    _transparency  = in->ReadInt32();
    _zOrder        = in->ReadInt32();
    _id            = in->ReadInt32();
    _padding       = in->ReadInt32();
    if (gui_version < kGuiVersion_350)
        in->Seek(sizeof(int32_t) * GUIMAIN_LEGACY_RESERVED_INTS);

    if (gui_version < kGuiVersion_350)
    {
        if (tw_flags[0] == kGUIMain_LegacyTextWindow)
            _flags |= kGUIMain_TextWindow;
        // reverse particular flags from older format
        _flags ^= kGUIMain_OldFmtXorMask;
        GUI::ApplyLegacyVisibility(*this, (LegacyGUIVisState)in->ReadInt32());
    }

    // pre-3.4.0 games contained array of 32-bit pointers; these values are unused
    // TODO: error if ctrl_count > LEGACY_MAX_OBJS_ON_GUI
    if (gui_version < kGuiVersion_340)
        in->Seek(LEGACY_MAX_OBJS_ON_GUI * sizeof(int32_t));
    if (ctrl_count > 0)
    {
        _ctrlRefs.resize(ctrl_count);
        for (uint32_t i = 0; i < ctrl_count; ++i)
        {
            const uint32_t ref_packed = in->ReadInt32();
            _ctrlRefs[i].first = (GUIControlType)((ref_packed >> 16) & 0xFFFF);
            _ctrlRefs[i].second = ref_packed & 0xFFFF;
        }
    }
    // Skip unused control slots in pre-3.4.0 games
    if (gui_version < kGuiVersion_340 && ctrl_count < LEGACY_MAX_OBJS_ON_GUI)
        in->Seek((LEGACY_MAX_OBJS_ON_GUI - ctrl_count) * sizeof(int32_t));
}

void GUIMain::WriteToFile(Stream *out) const
{
    StrUtil::WriteString(_name, out);
    StrUtil::WriteString(_onClickHandler, out);
    out->WriteInt32(_x);
    out->WriteInt32(_y);
    out->WriteInt32(_width);
    out->WriteInt32(_height);
    out->WriteInt32(_ctrlRefs.size());
    out->WriteInt32(_popupStyle);
    out->WriteInt32(_popupAtMouseY);
    out->WriteInt32(_bgColor);
    out->WriteInt32(_bgImage);
    out->WriteInt32(_fgColor);
    out->WriteInt32(_flags);
    out->WriteInt32(_transparency);
    out->WriteInt32(_zOrder);
    out->WriteInt32(_id);
    out->WriteInt32(_padding);
    for (const auto &ref : _ctrlRefs)
    {
        uint32_t ref_packed = ((ref.first & 0xFFFF) << 16) | (ref.second & 0xFFFF);
        out->WriteInt32(ref_packed);
    }
}

void GUIMain::ReadFromSavegame(Common::Stream *in, GuiSvgVersion svg_version, std::vector<ControlRef> &ctrl_refs)
{
    // Properties
    _flags = in->ReadInt32();
    _x = in->ReadInt32();
    _y = in->ReadInt32();
    _width = in->ReadInt32();
    _height = in->ReadInt32();
    _bgImage = in->ReadInt32();
    _transparency = in->ReadInt32();
    if (svg_version < kGuiSvgVersion_350)
    {
        // reverse particular flags from older format
        _flags ^= kGUIMain_OldFmtXorMask;
        GUI::ApplyLegacyVisibility(*this, (LegacyGUIVisState)in->ReadInt32());
    }
    _zOrder = in->ReadInt32();

    if (svg_version >= kGuiSvgVersion_350)
    {
        _bgColor = in->ReadInt32();
        _fgColor = in->ReadInt32();
        _padding = in->ReadInt32();
        _popupAtMouseY = in->ReadInt32();
    }

    // Dynamic values
    _focusCtrl = in->ReadInt32();
    _highlightCtrl = in->ReadInt32();
    _mouseOverCtrl = in->ReadInt32();
    _mouseDownCtrl = in->ReadInt32();
    _mouseWasAt.X = in->ReadInt32();
    _mouseWasAt.Y = in->ReadInt32();

    // Control refs
    if (svg_version >= kGuiSvgVersion_36200)
    {
        uint32_t ctrl_count = in->ReadInt32();
        ctrl_refs.resize(ctrl_count);
        for (uint32_t i = 0; i < ctrl_count; ++i)
        {
            const uint32_t ref_packed = in->ReadInt32();
            ctrl_refs[i].first = (GUIControlType)((ref_packed >> 16) & 0xFFFF);
            ctrl_refs[i].second = ref_packed & 0xFFFF;
        }
    }
}

void GUIMain::SkipSavestate(Stream *in, GuiSvgVersion svg_version, std::vector<ControlRef> *ctrl_refs)
{
    if (svg_version < kGuiSvgVersion_350)
        in->Seek(15 * sizeof(int32_t));
    else
        in->Seek(18 * sizeof(int32_t));

    if (svg_version >= kGuiSvgVersion_36200)
    {
        if (!ctrl_refs)
        {
            in->Seek(in->ReadInt32() * sizeof(int32_t));
            return;
        }

        uint32_t ctrl_count = in->ReadInt32();
        ctrl_refs->resize(ctrl_count);
        for (uint32_t i = 0; i < ctrl_count; ++i)
        {
            const uint32_t ref_packed = in->ReadInt32();
            (*ctrl_refs)[i].first = (GUIControlType)((ref_packed >> 16) & 0xFFFF);
            (*ctrl_refs)[i].second = ref_packed & 0xFFFF;
        }
    }
}

void GUIMain::WriteToSavegame(Common::Stream *out) const
{
    // Properties
    out->WriteInt32(_flags);
    out->WriteInt32(_x);
    out->WriteInt32(_y);
    out->WriteInt32(_width);
    out->WriteInt32(_height);
    out->WriteInt32(_bgImage);
    out->WriteInt32(_transparency);
    out->WriteInt32(_zOrder);
    out->WriteInt32(_bgColor);
    out->WriteInt32(_fgColor);
    out->WriteInt32(_padding);
    out->WriteInt32(_popupAtMouseY);
    // Dynamic values
    out->WriteInt32(_focusCtrl);
    out->WriteInt32(_highlightCtrl);
    out->WriteInt32(_mouseOverCtrl);
    out->WriteInt32(_mouseDownCtrl);
    out->WriteInt32(_mouseWasAt.X);
    out->WriteInt32(_mouseWasAt.Y);
    // Control refs (for asserting controls data on restoration,
    // and also in case if we support dynamic creation of controls)
    out->WriteInt32(_ctrlRefs.size());
    for (const auto &ref : _ctrlRefs)
    {
        uint32_t ref_packed = ((ref.first & 0xFFFF) << 16) | (ref.second & 0xFFFF);
        out->WriteInt32(ref_packed);
    }
}


namespace GUI
{

GuiVersion GameGuiVersion = kGuiVersion_Initial;

String FixupGUIName272(const String &name)
{
    if (name.GetLength() > 0 && name[0u] != 'g')
        return String::FromFormat("g%c%s", name[0u], name.Mid(1).Lower().GetCStr());
    return name;
}

Line CalcFontGraphicalVExtent(int font)
{
    // Following factors are affecting the graphical vertical metrics:
    // * font's real graphical extent (top and bottom offsets relative to the "pen")
    // * custom vertical offset set by user (if non-zero),
    const auto finfo = get_fontinfo(font);
    const auto fextent = get_font_surface_extent(font);
    int top = fextent.first +
        std::min(0, finfo.YOffset); // apply YOffset only if negative
    int bottom = fextent.second +
        std::max(0, finfo.YOffset); // apply YOffset only if positive
    return Line(0, top, 0, bottom);
}

// When aligning the text on GUI we use one of the methods of finding its height,
// depending on the loaded game version, for backwards compatibility.
// < 3.6.0: use text height measurement provided by the font renderer;
// 3.6.0.0 -> 3.6.0.21: font's height;
// 3.6.0.21 onwards: full outlined font's height.
inline int GetTextHeightForAlign(const String &text, int font, FrameAlignment align)
{
    if (loaded_game_file_version < kGameVersion_360)
        return get_text_height(text.GetCStr(), font) + ((align & kMAlignVCenter) ? 1 : 0);
    else if (loaded_game_file_version < kGameVersion_360_21)
        return get_font_height(font) + ((align & kMAlignVCenter) ? 1 : 0);
    else
        return get_font_height_outlined(font);
}

Point CalcTextPosition(const String &text, int font, const Rect &frame, FrameAlignment align, Rect *gr_rect)
{
    const int use_height = GetTextHeightForAlign(text, font, align);
    Rect rc = AlignInRect(frame, RectWH(0, 0, get_text_width_outlined(text.GetCStr(), font), use_height), align);
    if (gr_rect)
    {
        Line vextent = CalcFontGraphicalVExtent(font);
        *gr_rect = RectWH(rc.Left, rc.Top + vextent.Y1, rc.GetWidth(), vextent.Y2 - vextent.Y1 + 1);
    }
    return rc.GetLT();
}

Line CalcTextPositionHor(const String &text, int font, int x1, int x2, int y, FrameAlignment align)
{
    int w = get_text_width_outlined(text.GetCStr(), font);
    int x = AlignInHRange(x1, x2, 0, w, align);
    return Line(x, y, x + w - 1, y);
}

Rect CalcTextGraphicalRect(const String &text, int font, const Point &at)
{
    // Calc only width, and let CalcFontGraphicalVExtent() calc height
    int w = get_text_width_outlined(text.GetCStr(), font);
    Line vextent = CalcFontGraphicalVExtent(font);
    return RectWH(at.X, at.Y + vextent.Y1, w, vextent.Y2 - vextent.Y1 + 1);
}

Rect CalcTextGraphicalRect(const String &text, int font, const Rect &frame, FrameAlignment align)
{
    Rect gr_rect;
    CalcTextPosition(text, font, frame, align, &gr_rect);
    return gr_rect;
}

Rect CalcTextGraphicalRect(const std::vector<String> &text, size_t item_count, int font, int linespace,
    const Rect &frame, FrameAlignment align, bool limit_by_frame)
{
    item_count = std::min(item_count, text.size()); // safety check
    if (item_count <= 0)
        return {};

    int at_y = 0;
    size_t line_idx = 0u;
    if (limit_by_frame && at_y < frame.Top)
    {
        int skip_lines = (frame.Top - at_y) / linespace + 1;
        at_y += linespace * skip_lines;
        line_idx += skip_lines;
    }

    Line max_line;
    for (; line_idx < item_count && (!limit_by_frame || at_y <= frame.Bottom);
        ++line_idx, at_y += linespace)
    {
        Line lpos = GUI::CalcTextPositionHor(text[line_idx], font, frame.Left, frame.Right, at_y, align);
        max_line.X2 = std::max(max_line.X2, lpos.X2);
    }
    // Include font fixes for the first and last text line,
    // in case graphical height is different, and there's a VerticalOffset
    Line vextent = GUI::CalcFontGraphicalVExtent(font);
    Rect text_rc = RectWH(0, vextent.Y1, max_line.X2 - max_line.X1 + 1,
        at_y - linespace + (vextent.Y2 - vextent.Y1 + 1));
    return text_rc;
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

void DrawTextAligned(Bitmap *ds, const String &text, int font, color_t text_color, const Rect &frame, FrameAlignment align)
{
    Point pos = CalcTextPosition(text, font, frame, align);
    wouttext_outline(ds, pos.X, pos.Y, font, text_color, text.GetCStr());
}

void DrawTextAlignedHor(Bitmap *ds, const String &text, int font, color_t text_color, int x1, int x2, int y, FrameAlignment align)
{
    Line line = CalcTextPositionHor(text, font, x1, x2, y, align);
    wouttext_outline(ds, line.X1, y, font, text_color, text.GetCStr());
}

void DrawTextLinesAligned(Bitmap *ds, const std::vector<String> &text, size_t item_count,
    int font, int linespace, color_t text_color, const Rect &frame, FrameAlignment align,
    bool limit_by_frame)
{
    item_count = std::min(item_count, text.size()); // safety check
    if (item_count <= 0)
        return;

    int total_height = (item_count - 1) * linespace + get_font_height(font);
    int at_y = AlignInVRange(frame.Top, frame.Bottom, 0, total_height, align);
    size_t line_idx = 0u;

    if (limit_by_frame && at_y < frame.Top)
    {
        int skip_lines = (frame.Top - at_y) / linespace + 1;
        at_y += linespace * skip_lines;
        line_idx += skip_lines;
    }

    for (; (line_idx < item_count) && (!limit_by_frame || at_y < frame.Bottom);
        ++line_idx, at_y += linespace)
    {
        GUI::DrawTextAlignedHor(ds, text[line_idx], font, text_color, frame.Left, frame.Right, at_y, align);
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
                else if (ags_strnicmp(macro_at, "score", macro_len) == 0)
                    macro_flags |= kLabelMacro_Score;
                else if (ags_strnicmp(macro_at, "scoretext", macro_len) == 0)
                    macro_flags |= kLabelMacro_ScoreText;
                else if (ags_strnicmp(macro_at, "totalscore", macro_len) == 0)
                    macro_flags |= kLabelMacro_TotalScore;
                macro_at = nullptr;
            }
        }
    }
    return (GUILabelMacro)macro_flags;
}

HError RebuildGUI(std::vector<GUIMain> &guis, GUIRefCollection &guiobjs)
{
    const bool bwcompat_ctrl_zorder = GameGuiVersion < kGuiVersion_272e;
    // set up the reverse-lookup array
    for (auto &gui : guis)
    {
        HError err = gui.RebuildArray(guiobjs);
        if (!err)
            return err;
        for (int ctrl_index = 0; ctrl_index < gui.GetControlCount(); ++ctrl_index)
        {
            GUIObject *gui_ctrl = gui.GetControl(ctrl_index);
            gui_ctrl->SetParentID(gui.GetID());
            gui_ctrl->SetID(ctrl_index);
            if (bwcompat_ctrl_zorder)
                gui_ctrl->SetZOrder(ctrl_index);
        }
        gui.ResortZOrder();
    }
    return HError::None();
}

HError ReadGUI(std::vector<GUIMain> &guis, GUIRefCollection &guiobjs, Stream *in)
{
    if (in->ReadInt32() != (int)GUIMAGIC)
        return new Error("ReadGUI: unknown format or file is corrupt");

    GameGuiVersion = (GuiVersion)in->ReadInt32();
    Debug::Printf(kDbgMsg_Info, "Game GUI version: %d", GameGuiVersion);
    size_t gui_count;
    if (GameGuiVersion < kGuiVersion_214)
    {
        gui_count = (size_t)GameGuiVersion;
        GameGuiVersion = kGuiVersion_Initial;
    }
    else if (GameGuiVersion > kGuiVersion_Current)
        return new Error(String::FromFormat("ReadGUI: format version not supported (required %d, supported %d - %d)",
            GameGuiVersion, kGuiVersion_Initial, kGuiVersion_Current));
    else
        gui_count = in->ReadInt32();
    guis.resize(gui_count);

    // import the main GUI elements
    for (size_t i = 0; i < gui_count; ++i)
    {
        GUIMain &gui = guis[i];
        gui.ReadFromFile(in, GameGuiVersion);

        // perform fixups
        if (gui.GetHeight() < 2)
            gui.SetHeight(2);
        if (GameGuiVersion < kGuiVersion_unkn_103)
            gui.SetName(String::FromFormat("GUI%d", i));
        if (GameGuiVersion < kGuiVersion_260)
            gui.SetZOrder(i);
        if (GameGuiVersion < kGuiVersion_270)
            gui.SetOnClickHandler(String()); // CHECKME: not sure if we need to reset this here?
        if (GameGuiVersion < kGuiVersion_331)
            gui.SetPadding(TEXTWINDOW_PADDING_DEFAULT);
        // fix names for 2.x: "GUI" -> "gGui"
        if (loaded_game_file_version <= kGameVersion_272)
            gui.SetName(GUI::FixupGUIName272(gui.GetName()));

        // GUI popup style and visibility
        if (GameGuiVersion < kGuiVersion_350)
        {
            // Convert legacy normal-off style into normal one
            if (gui.GetPopupStyle() == kGUIPopupLegacyNormalOff)
            {
                gui.SetPopupStyle(kGUIPopupNormal);
                gui.SetVisible(false);
            }
            // Normal GUIs and PopupMouseY GUIs should start with Visible = true
            else
            {
                gui.SetVisible(gui.GetPopupStyle() != kGUIPopupModal);
            }
        }

        // PopupMouseY GUIs should be initially concealed
        if (gui.GetPopupStyle() == kGUIPopupMouseY)
            gui.SetConceal(true);
        // Assign _id to order in array
        gui.SetID(i);
    }

    // buttons
    size_t numguibuts = static_cast<uint32_t>(in->ReadInt32());
    auto &guibuts = guiobjs.Buttons;
    guibuts.resize(numguibuts);
    for (size_t i = 0; i < numguibuts; ++i)
    {
        guibuts[i].ReadFromFile(in, GameGuiVersion);
    }
    // labels
    size_t numguilabels = static_cast<uint32_t>(in->ReadInt32());
    auto &guilabels = guiobjs.Labels;
    guilabels.resize(numguilabels);
    for (size_t i = 0; i < numguilabels; ++i)
    {
        guilabels[i].ReadFromFile(in, GameGuiVersion);
    }
    // inv controls
    size_t numguiinv = static_cast<uint32_t>(in->ReadInt32());
    auto &guiinv = guiobjs.InvWindows;
    guiinv.resize(numguiinv);
    for (size_t i = 0; i < numguiinv; ++i)
    {
        guiinv[i].ReadFromFile(in, GameGuiVersion);
    }

    if (GameGuiVersion >= kGuiVersion_214)
    {
        // sliders
        size_t numguislider = static_cast<uint32_t>(in->ReadInt32());
        auto &guislider = guiobjs.Sliders;
        guislider.resize(numguislider);
        for (size_t i = 0; i < numguislider; ++i)
        {
            guislider[i].ReadFromFile(in, GameGuiVersion);
        }
    }
    if (GameGuiVersion >= kGuiVersion_222)
    {
        // text boxes
        size_t numguitext = static_cast<uint32_t>(in->ReadInt32());
        auto &guitext = guiobjs.TextBoxes;
        guitext.resize(numguitext);
        for (size_t i = 0; i < numguitext; ++i)
        {
            guitext[i].ReadFromFile(in, GameGuiVersion);
        }
    }
    if (GameGuiVersion >= kGuiVersion_230)
    {
        // list boxes
        size_t numguilist = static_cast<uint32_t>(in->ReadInt32());
        auto &guilist = guiobjs.ListBoxes;
        guilist.resize(numguilist);
        for (size_t i = 0; i < numguilist; ++i)
        {
            guilist[i].ReadFromFile(in, GameGuiVersion);
        }
    }
    return HError::None();
}

void WriteGUI(const std::vector<GUIMain> &guis, const GUIRefCollection &guiobjs, Stream *out)
{
    out->WriteInt32(GUIMAGIC);
    out->WriteInt32(kGuiVersion_Current);
    out->WriteInt32(guis.size());

    for (const auto &gui : guis)
    {
        gui.WriteToFile(out);
    }
    const auto &guibuts = guiobjs.Buttons;
    out->WriteInt32(static_cast<int32_t>(guibuts.size()));
    for (const auto &but : guibuts)
    {
        but.WriteToFile(out);
    }
    const auto &guilabels = guiobjs.Labels;
    out->WriteInt32(static_cast<int32_t>(guilabels.size()));
    for (const auto &label : guilabels)
    {
        label.WriteToFile(out);
    }
    const auto &guiinv = guiobjs.InvWindows;
    out->WriteInt32(static_cast<int32_t>(guiinv.size()));
    for (const auto &inv : guiinv)
    {
        inv.WriteToFile(out);
    }
    const auto &guislider = guiobjs.Sliders;
    out->WriteInt32(static_cast<int32_t>(guislider.size()));
    for (const auto &slider : guislider)
    {
        slider.WriteToFile(out);
    }
    const auto &guitext = guiobjs.TextBoxes;
    out->WriteInt32(static_cast<int32_t>(guitext.size()));
    for (const auto &tb : guitext)
    {
        tb.WriteToFile(out);
    }
    const auto &guilist = guiobjs.ListBoxes;
    out->WriteInt32(static_cast<int32_t>(guilist.size()));
    for (const auto &list : guilist)
    {
        list.WriteToFile(out);
    }
}

void ApplyLegacyVisibility(GUIMain &gui, LegacyGUIVisState vis)
{
    // kGUIPopupMouseY had its own rules, which we practically reverted now
    if (gui.GetPopupStyle() == kGUIPopupMouseY)
    {
        // it was only !Visible if the legacy Visibility was LockedOff
        gui.SetVisible(vis != kGUIVisibility_LockedOff);
        // and you could tell it's overridden by behavior when legacy Visibility is Off
        gui.SetConceal(vis == kGUIVisibility_Off);
    }
    // Other GUI styles were simple
    else
    {
        gui.SetVisible(vis != kGUIVisibility_Off);
        gui.SetConceal(false);
    }
}

} // namespace GUI

} // namespace Common
} // namespace AGS
