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
#include <algorithm>
#include <cstdio>
#include <numeric>
#include <vector>
#include "ac/gui.h"
#include "ac/common.h"
#include "ac/display.h"
#include "ac/draw.h"
#include "ac/event.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_character.h"
#include "ac/global_game.h"
#include "ac/global_gui.h"
#include "ac/global_inventoryitem.h"
#include "ac/global_screen.h"
#include "ac/guicontrol.h"
#include "ac/interfacebutton.h"
#include "ac/invwindow.h"
#include "ac/mouse.h"
#include "ac/runtime_defines.h"
#include "ac/string.h"
#include "ac/system.h"
#include "ac/dynobj/cc_guiobject.h"
#include "ac/dynobj/scriptgui.h"
#include "ac/dynobj/dynobj_manager.h"
#include "script/cc_instance.h"
#include "debug/debug_log.h"
#include "device/mousew32.h"
#include "gfx/gfxfilter.h"
#include "gui/guibutton.h"
#include "gui/guimain.h"
#include "script/script.h"
#include "script/script_runtime.h"
#include "gfx/graphicsdriver.h"
#include "gfx/bitmap.h"
#include "ac/dynobj/cc_gui.h"
#include "ac/dynobj/cc_guiobject.h"
#include "script/runtimescriptvalue.h"
#include "util/string_compat.h"


using namespace AGS::Common;
using namespace AGS::Engine;


extern RoomStruct thisroom;
extern int cur_mode,cur_cursor;
extern std::vector<ScriptGUI> scrGui;
extern GameSetupStruct game;
extern CCGUIObject ccDynamicGUIObject;
extern IGraphicsDriver *gfxDriver;

extern CCGUI ccDynamicGUI;
extern CCGUIObject ccDynamicGUIObject;


int ifacepopped=-1;  // currently displayed pop-up GUI (-1 if none)
int mouse_on_iface=-1;   // mouse cursor is over this interface
int mouse_ifacebut_xoffs=-1,mouse_ifacebut_yoffs=-1;

int eip_guinum, eip_guiobj;


namespace AGS
{
namespace Engine
{
namespace GUIE
{

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
    for (auto &tbox : guitext)
    {
        if (tbox.IsTranslated())
            tbox.MarkChanged();
    }
}

void MarkForFontUpdate(int font)
{
    const bool update_all = (font < 0);
    for (auto &btn : guibuts)
    {
        if (update_all || btn.GetFont() == font)
            btn.MarkChanged();
    }
    for (auto &lbl : guilabels)
    {
        if (update_all || lbl.GetFont() == font)
            lbl.MarkChanged();
    }
    for (auto &list : guilist)
    {
        if (update_all || list.GetFont() == font)
            list.MarkChanged();
    }
    for (auto &tb : guitext)
    {
        if (update_all || tb.GetFont() == font)
            tb.MarkChanged();
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
        if ((char_id < 0) || (inv.GetCharacterID() == char_id) || (is_player && inv.GetCharacterID() < 0))
        {
            inv.MarkChanged();
        }
    }
}

void MarkDisabledGUIForUpdate()
{
    for (auto &gui : guis)
    {
        for (int i = 0; i < gui.GetControlCount(); ++i)
        {
            auto *gc = gui.GetControl(i);
            if (!gc->IsEnabled())
                gc->MarkChanged();
        }
    }
}

} // namespace GUI
} // namespace Engine
} // namespace AGS


ScriptGUI* GUI_AsTextWindow(ScriptGUI *tehgui)
{ // Internally both GUI and TextWindow are implemented by same class
    return guis[tehgui->id].IsTextWindow() ? &scrGui[tehgui->id] : nullptr;
}

int GUI_GetPopupStyle(ScriptGUI *tehgui)
{
    return guis[tehgui->id].GetPopupStyle();
}

void GUI_SetPopupStyle(ScriptGUI *tehgui, int popup_style)
{
    const GUIPopupStyle old_style = guis[tehgui->id].GetPopupStyle();
    if (popup_style == old_style)
        return;

    GUIMain &g = guis[tehgui->id];
    g.SetPopupStyle(static_cast<GUIPopupStyle>(popup_style));
    if (g.IsVisible())
    {
        if (popup_style == kGUIPopupMouseY)
        {
            g.SetConceal(true);
        }
        else if (old_style == kGUIPopupMouseY)
        {
            if (g.IsDisplayed())
                remove_popup_interface(tehgui->id);
            g.SetConceal(false);
        }

        if (popup_style == kGUIPopupModal)
        {
            PauseGame();
        }
        else if (old_style == kGUIPopupModal)
        {
            UnPauseGame();
        }

        if (g.IsDisplayed())
        {
            g.Poll(mousex, mousey);
        }
    }
}

void GUI_SetVisible(ScriptGUI *tehgui, int isvisible) {
  if (isvisible)
    InterfaceOn(tehgui->id);
  else
    InterfaceOff(tehgui->id);
}

int GUI_GetVisible(ScriptGUI *tehgui) {
  // Since 3.5.0 this always returns honest state of the Visible property as set by the game
  if (loaded_game_file_version >= kGameVersion_350)
      return (guis[tehgui->id].IsVisible()) ? 1 : 0;
  // Prior to 3.5.0 PopupY guis overrided Visible property and set it to 0 when auto-hidden;
  // in order to simulate same behavior we only return positive if such gui is popped up:
  return (guis[tehgui->id].IsDisplayed()) ? 1 : 0;
}

bool GUI_GetShown(ScriptGUI *tehgui) {
    return guis[tehgui->id].IsDisplayed();
}

int GUI_GetX(ScriptGUI *tehgui) {
    return game_to_data_coord(guis[tehgui->id].GetX());
}

void GUI_SetX(ScriptGUI *tehgui, int xx) {
    guis[tehgui->id].SetX(data_to_game_coord(xx));
}

int GUI_GetY(ScriptGUI *tehgui) {
    return game_to_data_coord(guis[tehgui->id].GetY());
}

void GUI_SetY(ScriptGUI *tehgui, int yy) {
    guis[tehgui->id].SetY(data_to_game_coord(yy));
}

void GUI_SetPosition(ScriptGUI *tehgui, int xx, int yy) {
    guis[tehgui->id].SetPosition(data_to_game_coord(xx), data_to_game_coord(yy));
}

void GUI_SetSize(ScriptGUI *sgui, int widd, int hitt) {
  if ((widd < 1) || (hitt < 1))
  {
      debug_script_warn("GUI.SetSize: invalid dimensions (tried to set to %d x %d)", widd, hitt);
      widd = std::max(1, widd);
      hitt = std::max(1, hitt);
  }

  GUIMain *tehgui = &guis[sgui->id];
  data_to_game_coords(&widd, &hitt);
  tehgui->SetSize(widd, hitt);
}

int GUI_GetWidth(ScriptGUI *sgui) {
  return game_to_data_coord(guis[sgui->id].GetWidth());
}

int GUI_GetHeight(ScriptGUI *sgui) {
  return game_to_data_coord(guis[sgui->id].GetHeight());
}

void GUI_SetWidth(ScriptGUI *sgui, int newwid) {
    if (newwid < 1)
    {
        debug_script_warn("GUI.SetWidth: invalid value (tried to set to %d)", newwid);
        newwid = std::max(1, newwid);
    }
    guis[sgui->id].SetWidth(data_to_game_coord(newwid));
}

void GUI_SetHeight(ScriptGUI *sgui, int newhit) {
    if (newhit < 1)
    {
        debug_script_warn("GUI.SetHeight: invalid value (tried to set to %d)", newhit);
        newhit = std::max(1, newhit);
    }
    guis[sgui->id].SetHeight(data_to_game_coord(newhit));
}

void GUI_SetZOrder(ScriptGUI *tehgui, int z) {
  guis[tehgui->id].SetZOrder(z);
  update_gui_zorder();
}

int GUI_GetZOrder(ScriptGUI *tehgui) {
  return guis[tehgui->id].GetZOrder();
}

void GUI_SetClickable(ScriptGUI *tehgui, int clickable) {
  guis[tehgui->id].SetClickable(clickable != 0);
}

int GUI_GetClickable(ScriptGUI *tehgui) {
  return guis[tehgui->id].IsClickable() ? 1 : 0;
}

int GUI_GetID(ScriptGUI *tehgui) {
  return tehgui->id;
}

const char *GUI_GetScriptName(ScriptGUI *tehgui)
{
    return CreateNewScriptString(guis[tehgui->id].GetName());
}

GUIObject* GUI_GetiControls(ScriptGUI *tehgui, int idx) {
  if ((idx < 0) || (idx >= guis[tehgui->id].GetControlCount()))
    return nullptr;
  return guis[tehgui->id].GetControl(idx);
}

int GUI_GetControlCount(ScriptGUI *tehgui) {
  return guis[tehgui->id].GetControlCount();
}

int GUI_GetPopupYPos(ScriptGUI *tehgui)
{
    return guis[tehgui->id].GetPopupAtY();
}

void GUI_SetPopupYPos(ScriptGUI *tehgui, int newpos)
{
    if (!guis[tehgui->id].IsTextWindow())
        guis[tehgui->id].SetPopupAtY(newpos);
}

void GUI_SetTransparency(ScriptGUI *tehgui, int trans) {
  if ((trans < 0) | (trans > 100))
    quit("!SetGUITransparency: transparency value must be between 0 and 100");

  guis[tehgui->id].SetTransparencyAsPercentage(trans);
}

int GUI_GetTransparency(ScriptGUI *tehgui) {
  return GfxDef::LegacyTrans255ToTrans100(guis[tehgui->id].GetTransparency());
}

void GUI_Centre(ScriptGUI *sgui) {
  GUIMain *tehgui = &guis[sgui->id];
  tehgui->SetX(play.GetUIViewport().GetWidth() / 2 - tehgui->GetWidth() / 2);
  tehgui->SetY(play.GetUIViewport().GetHeight() / 2 - tehgui->GetHeight() / 2);
}

void GUI_SetBackgroundGraphic(ScriptGUI *tehgui, int slotn)
{
    guis[tehgui->id].SetBgImage(slotn);
    // Old games hack: always force redraw if this is a dynamic sprite with drawing surface
    // (in case they assign same sprite id, but forgot to release a drawing surface)
    if ((loaded_game_file_version < kGameVersion_351) && game.SpriteInfos[slotn].IsSurfaceAcquired())
        guis[tehgui->id].MarkChanged();
}

int GUI_GetBackgroundGraphic(ScriptGUI *tehgui)
{
    if (guis[tehgui->id].GetBgImage() < 1) // ???
        return 0;
    return guis[tehgui->id].GetBgImage();
}

void GUI_SetBackgroundColor(ScriptGUI *tehgui, int newcol)
{
    guis[tehgui->id].SetBgColor(newcol);
}

int GUI_GetBackgroundColor(ScriptGUI *tehgui)
{
    return guis[tehgui->id].GetBgColor();
}

void GUI_SetBorderColor(ScriptGUI *tehgui, int newcol)
{
    if (guis[tehgui->id].IsTextWindow())
        return;
    guis[tehgui->id].SetFgColor(newcol);
}

int GUI_GetBorderColor(ScriptGUI *tehgui)
{
    if (guis[tehgui->id].IsTextWindow())
        return 0;
    return guis[tehgui->id].GetFgColor();
}

void GUI_SetTextColor(ScriptGUI *tehgui, int newcol)
{
    if (!guis[tehgui->id].IsTextWindow())
        return;
    guis[tehgui->id].SetFgColor(newcol);
}

int GUI_GetTextColor(ScriptGUI *tehgui)
{
    if (!guis[tehgui->id].IsTextWindow())
        return 0;
    return guis[tehgui->id].GetFgColor();
}

int GUI_GetTextPadding(ScriptGUI *tehgui)
{
    return guis[tehgui->id].GetPadding();
}

void GUI_SetTextPadding(ScriptGUI *tehgui, int newpos)
{
    if (guis[tehgui->id].IsTextWindow())
        guis[tehgui->id].SetPadding(newpos);
}

int GUI_GetLeftGraphic(ScriptGUI *tehgui)
{
    return guis[tehgui->id].IsTextWindow() ?
        get_but_pic(&guis[tehgui->id], kTW_Left) : 0;
}

int GUI_GetTopLeftGraphic(ScriptGUI *tehgui)
{
    return guis[tehgui->id].IsTextWindow() ?
        get_but_pic(&guis[tehgui->id], kTW_TopLeft) : 0;
}

int GUI_GetTopGraphic(ScriptGUI *tehgui)
{
    return guis[tehgui->id].IsTextWindow() ?
        get_but_pic(&guis[tehgui->id], kTW_Top) : 0;
}

int GUI_GetTopRightGraphic(ScriptGUI *tehgui)
{
    return guis[tehgui->id].IsTextWindow() ?
        get_but_pic(&guis[tehgui->id], kTW_TopRight) : 0;
}

int GUI_GetRightGraphic(ScriptGUI *tehgui)
{
    return guis[tehgui->id].IsTextWindow() ?
        get_but_pic(&guis[tehgui->id], kTW_Right) : 0;
}

int GUI_GetBottomRightGraphic(ScriptGUI *tehgui)
{
    return guis[tehgui->id].IsTextWindow() ?
        get_but_pic(&guis[tehgui->id], kTW_BottomRight) : 0;
}

int GUI_GetBottomGraphic(ScriptGUI *tehgui)
{
    return guis[tehgui->id].IsTextWindow() ?
        get_but_pic(&guis[tehgui->id], kTW_Bottom) : 0;
}

int GUI_GetBottomLeftGraphic(ScriptGUI *tehgui)
{
    return guis[tehgui->id].IsTextWindow() ?
        get_but_pic(&guis[tehgui->id], kTW_BottomLeft) : 0;
}

ScriptGUI *GetGUIAtLocation(int xx, int yy) {
    int guiid = GetGUIAt(xx, yy);
    if (guiid < 0)
        return nullptr;
    return &scrGui[guiid];
}

void GUI_Click(ScriptGUI *scgui, int mbut)
{
    process_interface_click(scgui->id, -1, mbut);
}

void GUI_ProcessClick(int x, int y, int mbut)
{
    int guiid = gui_get_interactable(x, y);
    if (guiid >= 0)
    { // simulate mouse click at the given coordinates
        guis[guiid].Poll(x, y);
        gui_on_mouse_down(guiid, mbut, x, y);
        gui_on_mouse_up(guiid, mbut, x, y);
    }
}

//=============================================================================

void remove_popup_interface(int ifacenum) {
    if (ifacepopped != ifacenum) return;
    ifacepopped=-1;
    UnPauseGame();
    guis[ifacenum].SetConceal(true);
    if (mousey<=guis[ifacenum].GetPopupAtY())
        Mouse::SetPosition(Point(mousex, guis[ifacenum].GetPopupAtY() +2));
    if ((!IsInterfaceEnabled()) && (cur_cursor == cur_mode))
        // Only change the mouse cursor if it hasn't been specifically changed first
        set_mouse_cursor(CURS_WAIT);
    else if (IsInterfaceEnabled())
        set_default_cursor();

    if (ifacenum==mouse_on_iface) mouse_on_iface=-1;
}

void process_interface_click(int ifce, int btn, int mbut) {
    if (btn < 0) {
        // click on GUI background
        RuntimeScriptValue params[]{ RuntimeScriptValue().SetScriptObject(&scrGui[ifce], &ccDynamicGUI),
            RuntimeScriptValue().SetInt32(mbut) };
        QueueScriptFunction(kScTypeGame, ScriptFunctionRef(guis[ifce].GetScriptModule(), guis[ifce].GetOnClickHandler()), 2, params);
        return;
    }

    int btype = guis[ifce].GetControlType(btn);
    int rtype=kGUIAction_None,rdata=0;
    if (btype==kGUIButton) {
        GUIButton*gbuto=(GUIButton*)guis[ifce].GetControl(btn);
        rtype=gbuto->GetClickAction(kGUIClickLeft);
        rdata=gbuto->GetClickData(kGUIClickLeft);
    }
    else if ((btype==kGUISlider) || (btype == kGUITextBox) || (btype == kGUIListBox))
        rtype = kGUIAction_RunScript;
    else quit("unknown GUI object triggered process_interface");

    if (rtype==kGUIAction_None) ;
    else if (rtype==kGUIAction_SetMode)
        set_cursor_mode(rdata);
    else if (rtype==kGUIAction_RunScript) {
        GUIObject *theObj = guis[ifce].GetControl(btn);
        // if the object has a special handler script then run it;
        // otherwise, run interface_click
        // FIXME: do not call DoesScriptFunctionExist* every time, remember last result,
        // similar to the interaction event handler test.
        if ((theObj->GetEventCount() > 0) &&
            (!theObj->GetEventHandler(0).IsEmpty()) &&
            DoesScriptFunctionExistInModule(guis[ifce].GetScriptModule(), theObj->GetEventHandler(0)))
        {
            // control-specific event handler
            const ScriptFunctionRef fn_ref(guis[ifce].GetScriptModule(), theObj->GetEventHandler(0));
            if (theObj->GetEventArgs(0).FindChar(',') != String::NoIndex)
            {
                RuntimeScriptValue params[]{ RuntimeScriptValue().SetScriptObject(theObj, &ccDynamicGUIObject),
                    RuntimeScriptValue().SetInt32(mbut) };
                QueueScriptFunction(kScTypeGame, fn_ref, 2, params);
            }
            else
            {
                RuntimeScriptValue params[]{ RuntimeScriptValue().SetScriptObject(theObj, &ccDynamicGUIObject) };
                QueueScriptFunction(kScTypeGame, fn_ref, 1, params);
            }
        }
        else
        {
            RuntimeScriptValue params[]{ ifce , btn };
            QueueScriptFunction(kScTypeGame, "interface_click", 2, params);
        }
    }
}

bool sort_gui_less(const int g1, const int g2)
{
    return (guis[g1].GetZOrder() < guis[g2].GetZOrder()) ||
        ((guis[g1].GetZOrder() == guis[g2].GetZOrder()) && (g1 < g2));
}

void update_gui_zorder()
{
    std::sort(play.gui_draw_order.begin(), play.gui_draw_order.end(), sort_gui_less);
}

void prepare_gui_runtime(bool startup)
{
    // Trigger all guis and controls to recalculate their dynamic state
    for (auto &gui : guis)
    {
        for (int i = 0; i < gui.GetControlCount(); ++i)
        {
            GUIObject *guio = gui.GetControl(i);
            guio->UpdateVisualState();
            guio->SetActivated(false);
        }
    }
    // Reset particular states after loading game data
    if (startup)
    {
        // labels are not clickable by default
        // CHECKME: why are we doing this at all?
        for (auto &label : guilabels)
        {
            label.SetClickable(false);
        }
    }
    play.gui_draw_order.resize(guis.size());
    std::iota(play.gui_draw_order.begin(), play.gui_draw_order.end(), 0);
    update_gui_zorder();

    GUI::Options.DisabledStyle = static_cast<GuiDisableStyle>(game.options[OPT_DISABLEOFF]);
    GUIE::MarkAllGUIForUpdate(true, true);
}

void export_gui_controls(int ee)
{
    for (int ff = 0; ff < guis[ee].GetControlCount(); ff++)
    {
        GUIObject *guio = guis[ee].GetControl(ff);
        if (!guio->GetName().IsEmpty())
            ccAddExternalScriptObject(guio->GetName(), guio, &ccDynamicGUIObject);
        ccRegisterManagedObject(guio, &ccDynamicGUIObject);
    }
}

void unexport_gui_controls(int ee)
{
    for (int ff = 0; ff < guis[ee].GetControlCount(); ff++)
    {
        GUIObject *guio = guis[ee].GetControl(ff);
        if (!guio->GetName().IsEmpty())
            ccRemoveExternalSymbol(guio->GetName());
        if (!ccUnRegisterManagedObject(guio))
            quit("unable to unregister guicontrol object");
    }
}

void update_gui_disabled_status()
{
    // update GUI display status (perhaps we've gone into an interface disabled state)
    const GuiDisableStyle disabled_state_was = GUI::Context.DisabledState;
    GUI::Context.DisabledState = IsInterfaceEnabled() ?
        kGuiDis_Undefined : GUI::Options.DisabledStyle;

    if (disabled_state_was != GUI::Context.DisabledState)
    {
        // Mark guis for redraw and reset control-under-mouse detection
        const bool keep_visuals = (disabled_state_was == kGuiDis_Undefined || disabled_state_was == kGuiDis_Unchanged)
            && (GUI::Context.DisabledState == kGuiDis_Undefined || GUI::Context.DisabledState == kGuiDis_Unchanged);
        GUIE::MarkAllGUIForUpdate(!keep_visuals, true);
        if (!keep_visuals)
        {
            invalidate_screen();
        }
    }
}

static bool should_skip_adjust_for_gui(const GUIMain &gui)
{
    return
        // not shown
        !gui.IsDisplayed() ||
        // completely offscreen
        !IsRectInsideRect(play.GetUIViewport(), gui.GetRect()) ||
        // fully transparent (? FIXME: this only checks background, but not controls)
        ((gui.GetBgColor() == 0) && (gui.GetBgImage() < 1)) || (gui.GetTransparency() == 255);
}

int adjust_x_for_guis(int x, int y, bool assume_blocking)
{
    if ((GUI::Options.DisabledStyle == kGuiDis_Off) &&
        (!IsInterfaceEnabled() || assume_blocking))
        return x; // All GUI off (or will be when the message is displayed)
    // If it's covered by a GUI, move it right a bit
    // FIXME: should not we also account for the text's width here?
    //        and then, maybe we should also able to move it left if rightmost part of text touches GUI?
    for (const auto &gui : guis)
    {
        if (should_skip_adjust_for_gui(gui))
            continue;
        // lower or higher than the message
        if ((gui.GetY() > y) || (gui.GetY() + gui.GetHeight() < y))
            continue;
        // to the left or to the right from the message
        if ((gui.GetX() > x) || (gui.GetX() + gui.GetWidth() < x))
            continue;
        // try to deal with very wide GUIs
        const float gui_width_cap = 0.75f; // NOTE: originally was 280 pixels in 320-wide game
        if (gui.GetWidth() >= game.GetGameRes().Width * gui_width_cap)
            continue;
        // fix coordinates if x is inside the gui
        x = gui.GetX() + gui.GetWidth() + 2;
    }
    return x;
}

int adjust_y_for_guis(int y, bool assume_blocking)
{
    if ((GUI::Options.DisabledStyle == kGuiDis_Off) &&
        (!IsInterfaceEnabled() || assume_blocking))
        return y; // All GUI off (or will be when the message is displayed)
    // If it's covered by a GUI, move it down a bit
    // FIXME: should not we also account for the text's height here?
    //        and then, maybe we should also able to move it up if lower part of text touches GUI below?
    for (const auto &gui : guis)
    {
        if (should_skip_adjust_for_gui(gui))
            continue;
        // lower or higher than the message
        if ((gui.GetY() > y) || (gui.GetY() + gui.GetHeight() < y))
            continue;
        // try to deal with tall GUIs; CHECKME this later
        const float gui_height_cap = 0.25f; // NOTE: originally was 50 pixels in 200-height game
        if (gui.GetHeight() >= game.GetGameRes().Height * gui_height_cap)
            continue;
        // fix coordinates if y is inside the gui
        y = gui.GetY() + gui.GetHeight() + 2;
    }
    return y;
}

int gui_get_interactable(int x, int y)
{
    if (GUI::Context.DisabledState == kGuiDis_Off)
        return -1;
    return GetGUIAt(x, y);
}

int gui_on_mouse_move(const int mx, const int my)
{
    int mouse_over_gui = -1;
    // If all GUIs are off, skip the loop
    if ((game.options[OPT_DISABLEOFF] == kGuiDis_Off) && (GUI::Context.DisabledState >= 0)) ;
    else
    {
        // Scan for mouse-y-pos GUIs, and pop one up if appropriate
        // Also work out the mouse-over GUI while we're at it
        // CHECKME: not sure why, but we're testing forward draw order here -
        // from farthest to nearest (this was in original code?)
        for (int guin : play.gui_draw_order)
        {
            if (guis[guin].IsInteractableAt(mx, my)) mouse_over_gui=guin;

            if (guis[guin].GetPopupStyle()!=kGUIPopupMouseY) continue;
            if (play.complete_overlay_on > 0) break;  // interfaces disabled
            if (ifacepopped==guin) continue;
            if (!guis[guin].IsVisible()) continue;
            // Don't allow it to be popped up while skipping cutscene
            if (play.fast_forward) continue;

            if (mousey < guis[guin].GetPopupAtY())
            {
                set_mouse_cursor(CURS_ARROW);
                guis[guin].SetConceal(false);
                ifacepopped=guin;
                PauseGame();
                break;
            }
        }
    }
    return mouse_over_gui;
}

// Tells if GUI common controls should react to this mouse button.
// TODO: redesign this all, pass mouse button into GUI OnMouseButtonDown instead,
// handle the button difference there, per control.
inline bool gui_control_should_handle_button(int mbut)
{
    return (game.options[OPT_GUICONTROLMOUSEBUT] == 0) || (mbut == kMouseLeft);
}

void gui_on_mouse_hold(const int wasongui, const int wasbutdown)
{
    for (int i = 0; i < guis[wasongui].GetControlCount(); ++i)
    {
        GUIObject *guio = guis[wasongui].GetControl(i);
        if (!guio->IsActivated())
            continue;
        // We only handle "hold" event for Sliders, and only if mouse button is not restricted
        if (guis[wasongui].GetControlType(i) != kGUISlider)
            continue;
        if (!gui_control_should_handle_button(wasbutdown))
            continue;
        // GUI Slider repeatedly activates while being dragged
        guio->SetActivated(false);
        force_event(AGSEvent_GUI(wasongui, i, static_cast<eAGSMouseButton>(wasbutdown)));
        break;
    }
}

void gui_on_mouse_up(const int wasongui, const int wasbutdown, const int mx, const int my)
{
    guis[wasongui].OnMouseButtonUp();

    for (int i = 0; i < guis[wasongui].GetControlCount(); ++i)
    {
        GUIObject *guio = guis[wasongui].GetControl(i);
        if (!guio->IsActivated())
            continue;

        guio->SetActivated(false);
        if (!IsInterfaceEnabled())
            break;

        bool click_handled = false;
        int cttype = guis[wasongui].GetControlType(i);
        if ((cttype == kGUIButton) || (cttype == kGUISlider) || (cttype == kGUIListBox))
        {
            if (gui_control_should_handle_button(wasbutdown))
            {
                click_handled = true;
                force_event(AGSEvent_GUI(wasongui, i, static_cast<eAGSMouseButton>(wasbutdown)));
            }
        }
        else if (cttype == kGUIInvWindow)
        {
            click_handled = true;
            mouse_ifacebut_xoffs = mx - (guio->GetX()) - guis[wasongui].GetX();
            mouse_ifacebut_yoffs = my - (guio->GetY()) - guis[wasongui].GetY();
            int iit = offset_over_inv((GUIInvWindow*)guio);
            if (iit >= 0)
            {
                play.used_inv_on = iit;
                if (game.options[OPT_HANDLEINVCLICKS])
                {
                    // Let the script handle the click
                    // LEFTINV is 5, RIGHTINV is 6
                    force_event(AGSEvent_Script(kTS_MouseClick, wasbutdown + 4, mx, my));
                }
                else if (wasbutdown == kMouseRight) // right-click is always Look
                {
                    RunInventoryInteraction(iit, MODE_LOOK);
                }
                else if (cur_mode == MODE_HAND)
                {
                    SetActiveInventory(iit);
                }
                else
                {
                    RunInventoryInteraction(iit, cur_mode);
                }
            }
        }
        else
        {
            quit("clicked on unknown control type");
        }

        // Built-in behavior for PopupAtY guis: hide one if interacted with any control
        if ((guis[wasongui].GetPopupStyle() == kGUIPopupMouseY) && click_handled)
            remove_popup_interface(wasongui);
        break;
    }

    run_on_event(kScriptEvent_GUIMouseUp, wasongui, wasbutdown, mx - guis[wasongui].GetX(), my - guis[wasongui].GetY());
}

void gui_on_mouse_down(const int guin, const int mbut, const int mx, const int my)
{
    debug_script_log("Mouse click over GUI %d", guin);

    // TODO: redesign this, pass mouse button into GUI OnMouseButtonDown instead,
    // handle the button difference there, per control.
    int over_ctrl = guis[guin].GetControlUnderMouse();
    if (over_ctrl >= 0)
    {
        // NOTE: we make exception for InvWindow control, as it has a special action for RMB
        if (gui_control_should_handle_button(mbut) ||
            (guis[guin].GetControlType(over_ctrl) == kGUIInvWindow))
        {
            guis[guin].OnMouseButtonDown(mx, my);
        }
    }
    else
    {
        // run GUI click handler if not on any control
        if (!guis[guin].GetOnClickHandler().IsEmpty())
            force_event(AGSEvent_GUI(guin, -1, static_cast<eAGSMouseButton>(mbut)));
    }

    run_on_event(kScriptEvent_GUIMouseDown, guin, mbut, mx - guis[guin].GetX(), my - guis[guin].GetY());
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "ac/dynobj/scriptstring.h"
#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"


ScriptGUI *GUI_GetByName(const char *name)
{
    return static_cast<ScriptGUI*>(ccGetScriptObjectAddress(name, ccDynamicGUI.GetType()));
}


RuntimeScriptValue Sc_GUI_GetByName(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_POBJ(ScriptGUI, ccDynamicGUI, GUI_GetByName, const char);
}

// void GUI_Centre(ScriptGUI *sgui)
RuntimeScriptValue Sc_GUI_Centre(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptGUI, GUI_Centre);
}

// ScriptGUI *(int xx, int yy)
RuntimeScriptValue Sc_GetGUIAtLocation(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT2(ScriptGUI, ccDynamicGUI, GetGUIAtLocation);
}

// void (ScriptGUI *tehgui, int xx, int yy)
RuntimeScriptValue Sc_GUI_SetPosition(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(ScriptGUI, GUI_SetPosition);
}

// void (ScriptGUI *sgui, int widd, int hitt)
RuntimeScriptValue Sc_GUI_SetSize(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(ScriptGUI, GUI_SetSize);
}

// int (ScriptGUI *tehgui)
RuntimeScriptValue Sc_GUI_GetBackgroundGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetBackgroundGraphic);
}

// void (ScriptGUI *tehgui, int slotn)
RuntimeScriptValue Sc_GUI_SetBackgroundGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetBackgroundGraphic);
}

RuntimeScriptValue Sc_GUI_GetBackgroundColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetBackgroundColor);
}

RuntimeScriptValue Sc_GUI_SetBackgroundColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetBackgroundColor);
}

RuntimeScriptValue Sc_GUI_GetBorderColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetBorderColor);
}

RuntimeScriptValue Sc_GUI_SetBorderColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetBorderColor);
}

RuntimeScriptValue Sc_GUI_GetTextColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetTextColor);
}

RuntimeScriptValue Sc_GUI_SetTextColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetTextColor);
}

// int (ScriptGUI *tehgui)
RuntimeScriptValue Sc_GUI_GetClickable(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetClickable);
}

// void (ScriptGUI *tehgui, int clickable)
RuntimeScriptValue Sc_GUI_SetClickable(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetClickable);
}

// int (ScriptGUI *tehgui)
RuntimeScriptValue Sc_GUI_GetControlCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetControlCount);
}

// GUIObject* (ScriptGUI *tehgui, int idx)
RuntimeScriptValue Sc_GUI_GetiControls(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT(ScriptGUI, GUIObject, ccDynamicGUIObject, GUI_GetiControls);
}

// int (ScriptGUI *sgui)
RuntimeScriptValue Sc_GUI_GetHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetHeight);
}

// void (ScriptGUI *sgui, int newhit)
RuntimeScriptValue Sc_GUI_SetHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetHeight);
}

// int (ScriptGUI *tehgui)
RuntimeScriptValue Sc_GUI_GetID(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetID);
}

RuntimeScriptValue Sc_GUI_GetScriptName(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptGUI, const char, myScriptStringImpl, GUI_GetScriptName);
}

RuntimeScriptValue Sc_GUI_GetPopupYPos(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetPopupYPos);
}

RuntimeScriptValue Sc_GUI_SetPopupYPos(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetPopupYPos);
}

RuntimeScriptValue Sc_GUI_GetTextPadding(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetTextPadding);
}

RuntimeScriptValue Sc_GUI_SetTextPadding(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetTextPadding);
}

// int (ScriptGUI *tehgui)
RuntimeScriptValue Sc_GUI_GetTransparency(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetTransparency);
}

// void (ScriptGUI *tehgui, int trans)
RuntimeScriptValue Sc_GUI_SetTransparency(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetTransparency);
}

// int (ScriptGUI *tehgui)
RuntimeScriptValue Sc_GUI_GetVisible(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetVisible);
}

// void (ScriptGUI *tehgui, int isvisible)
RuntimeScriptValue Sc_GUI_SetVisible(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetVisible);
}

// int (ScriptGUI *sgui)
RuntimeScriptValue Sc_GUI_GetWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetWidth);
}

// void (ScriptGUI *sgui, int newwid)
RuntimeScriptValue Sc_GUI_SetWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetWidth);
}

// int (ScriptGUI *tehgui)
RuntimeScriptValue Sc_GUI_GetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetX);
}

// void (ScriptGUI *tehgui, int xx)
RuntimeScriptValue Sc_GUI_SetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetX);
}

// int (ScriptGUI *tehgui)
RuntimeScriptValue Sc_GUI_GetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetY);
}

// void (ScriptGUI *tehgui, int yy)
RuntimeScriptValue Sc_GUI_SetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetY);
}

// int (ScriptGUI *tehgui)
RuntimeScriptValue Sc_GUI_GetZOrder(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetZOrder);
}

// void (ScriptGUI *tehgui, int z)
RuntimeScriptValue Sc_GUI_SetZOrder(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetZOrder);
}

RuntimeScriptValue Sc_GUI_AsTextWindow(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptGUI, ScriptGUI, ccDynamicGUI, GUI_AsTextWindow);
}

RuntimeScriptValue Sc_GUI_GetPopupStyle(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetPopupStyle);
}

RuntimeScriptValue Sc_GUI_SetPopupStyle(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetPopupStyle);
}

RuntimeScriptValue Sc_GUI_Click(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_Click);
}

RuntimeScriptValue Sc_GUI_ProcessClick(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(GUI_ProcessClick);
}

RuntimeScriptValue Sc_GUI_GetShown(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptGUI, GUI_GetShown);
}

RuntimeScriptValue Sc_GUI_GetLeftGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetLeftGraphic);
}

RuntimeScriptValue Sc_GUI_GetTopLeftGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetTopLeftGraphic);
}

RuntimeScriptValue Sc_GUI_GetTopGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetTopGraphic);
}

RuntimeScriptValue Sc_GUI_GetTopRightGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetTopRightGraphic);
}

RuntimeScriptValue Sc_GUI_GetRightGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetRightGraphic);
}

RuntimeScriptValue Sc_GUI_GetBottomRightGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetBottomRightGraphic);
}

RuntimeScriptValue Sc_GUI_GetBottomGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetBottomGraphic);
}

RuntimeScriptValue Sc_GUI_GetBottomLeftGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetBottomLeftGraphic);
}

void RegisterGUIAPI()
{
    ScFnRegister gui_api[] = {
        { "GUI::GetAtScreenXY^2",         API_FN_PAIR(GetGUIAtLocation) },
        { "GUI::GetByName",               API_FN_PAIR(GUI_GetByName) },

        { "GUI::ProcessClick^3",          API_FN_PAIR(GUI_ProcessClick) },

        { "GUI::Centre^0",                API_FN_PAIR(GUI_Centre) },
        { "GUI::Click^1",                 API_FN_PAIR(GUI_Click) },
        { "GUI::SetPosition^2",           API_FN_PAIR(GUI_SetPosition) },
        { "GUI::SetSize^2",               API_FN_PAIR(GUI_SetSize) },
        { "GUI::get_BackgroundGraphic",   API_FN_PAIR(GUI_GetBackgroundGraphic) },
        { "GUI::set_BackgroundGraphic",   API_FN_PAIR(GUI_SetBackgroundGraphic) },
        { "GUI::get_BackgroundColor",     API_FN_PAIR(GUI_GetBackgroundColor) },
        { "GUI::set_BackgroundColor",     API_FN_PAIR(GUI_SetBackgroundColor) },
        { "GUI::get_BorderColor",         API_FN_PAIR(GUI_GetBorderColor) },
        { "GUI::set_BorderColor",         API_FN_PAIR(GUI_SetBorderColor) },
        { "GUI::get_Clickable",           API_FN_PAIR(GUI_GetClickable) },
        { "GUI::set_Clickable",           API_FN_PAIR(GUI_SetClickable) },
        { "GUI::get_ControlCount",        API_FN_PAIR(GUI_GetControlCount) },
        { "GUI::geti_Controls",           API_FN_PAIR(GUI_GetiControls) },
        { "GUI::get_Height",              API_FN_PAIR(GUI_GetHeight) },
        { "GUI::set_Height",              API_FN_PAIR(GUI_SetHeight) },
        { "GUI::get_ID",                  API_FN_PAIR(GUI_GetID) },
        { "GUI::get_AsTextWindow",        API_FN_PAIR(GUI_AsTextWindow) },
        { "GUI::get_PopupStyle",          API_FN_PAIR(GUI_GetPopupStyle) },
        { "GUI::set_PopupStyle",          API_FN_PAIR(GUI_SetPopupStyle) },
        { "GUI::get_PopupYPos",           API_FN_PAIR(GUI_GetPopupYPos) },
        { "GUI::set_PopupYPos",           API_FN_PAIR(GUI_SetPopupYPos) },
        { "GUI::get_ScriptName",          API_FN_PAIR(GUI_GetScriptName) },
        { "GUI::get_Transparency",        API_FN_PAIR(GUI_GetTransparency) },
        { "GUI::set_Transparency",        API_FN_PAIR(GUI_SetTransparency) },
        { "GUI::get_Visible",             API_FN_PAIR(GUI_GetVisible) },
        { "GUI::set_Visible",             API_FN_PAIR(GUI_SetVisible) },
        { "GUI::get_Width",               API_FN_PAIR(GUI_GetWidth) },
        { "GUI::set_Width",               API_FN_PAIR(GUI_SetWidth) },
        { "GUI::get_X",                   API_FN_PAIR(GUI_GetX) },
        { "GUI::set_X",                   API_FN_PAIR(GUI_SetX) },
        { "GUI::get_Y",                   API_FN_PAIR(GUI_GetY) },
        { "GUI::set_Y",                   API_FN_PAIR(GUI_SetY) },
        { "GUI::get_ZOrder",              API_FN_PAIR(GUI_GetZOrder) },
        { "GUI::set_ZOrder",              API_FN_PAIR(GUI_SetZOrder) },
        { "GUI::get_Shown",               API_FN_PAIR(GUI_GetShown) },
        { "TextWindowGUI::get_TextColor", API_FN_PAIR(GUI_GetTextColor) },
        { "TextWindowGUI::set_TextColor", API_FN_PAIR(GUI_SetTextColor) },
        { "TextWindowGUI::get_TextPadding", API_FN_PAIR(GUI_GetTextPadding) },
        { "TextWindowGUI::set_TextPadding", API_FN_PAIR(GUI_SetTextPadding) },
        { "TextWindowGUI::get_LeftGraphic", API_FN_PAIR(GUI_GetLeftGraphic) },
        { "TextWindowGUI::get_TopLeftGraphic", API_FN_PAIR(GUI_GetTopLeftGraphic) },
        { "TextWindowGUI::get_TopGraphic", API_FN_PAIR(GUI_GetTopGraphic) },
        { "TextWindowGUI::get_TopRightGraphic", API_FN_PAIR(GUI_GetTopRightGraphic) },
        { "TextWindowGUI::get_RightGraphic", API_FN_PAIR(GUI_GetRightGraphic) },
        { "TextWindowGUI::get_BottomRightGraphic", API_FN_PAIR(GUI_GetBottomRightGraphic) },
        { "TextWindowGUI::get_BottomGraphic", API_FN_PAIR(GUI_GetBottomGraphic) },
        { "TextWindowGUI::get_BottomLeftGraphic", API_FN_PAIR(GUI_GetBottomLeftGraphic) },
    };

    ccAddExternalFunctions(gui_api);
}
