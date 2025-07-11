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
#include <algorithm>
#include <cstdio>
#include <numeric>
#include <vector>
#include "ac/gui.h"
#include "ac/common.h"
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

} // namespace GUI
} // namespace Engine
} // namespace AGS


ScriptGUI* GUI_AsTextWindow(ScriptGUI *tehgui)
{ // Internally both GUI and TextWindow are implemented by same class
    return guis[tehgui->id].IsTextWindow() ? &scrGui[tehgui->id] : nullptr;
}

int GUI_GetPopupStyle(ScriptGUI *tehgui)
{
    return guis[tehgui->id].PopupStyle;
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
  return game_to_data_coord(guis[tehgui->id].X);
}

void GUI_SetX(ScriptGUI *tehgui, int xx) {
  guis[tehgui->id].X = data_to_game_coord(xx);
}

int GUI_GetY(ScriptGUI *tehgui) {
  return game_to_data_coord(guis[tehgui->id].Y);
}

void GUI_SetY(ScriptGUI *tehgui, int yy) {
  guis[tehgui->id].Y = data_to_game_coord(yy);
}

void GUI_SetPosition(ScriptGUI *tehgui, int xx, int yy) {
  GUI_SetX(tehgui, xx);
  GUI_SetY(tehgui, yy);
}

void GUI_SetSize(ScriptGUI *sgui, int widd, int hitt) {
  if ((widd < 1) || (hitt < 1))
    quitprintf("!SetGUISize: invalid dimensions (tried to set to %d x %d)", widd, hitt);

  GUIMain *tehgui = &guis[sgui->id];
  data_to_game_coords(&widd, &hitt);

  if ((tehgui->Width == widd) && (tehgui->Height == hitt))
    return;
  
  tehgui->Width = widd;
  tehgui->Height = hitt;

  tehgui->MarkChanged();
}

int GUI_GetWidth(ScriptGUI *sgui) {
  return game_to_data_coord(guis[sgui->id].Width);
}

int GUI_GetHeight(ScriptGUI *sgui) {
  return game_to_data_coord(guis[sgui->id].Height);
}

void GUI_SetWidth(ScriptGUI *sgui, int newwid) {
  GUI_SetSize(sgui, newwid, GUI_GetHeight(sgui));
}

void GUI_SetHeight(ScriptGUI *sgui, int newhit) {
  GUI_SetSize(sgui, GUI_GetWidth(sgui), newhit);
}

void GUI_SetZOrder(ScriptGUI *tehgui, int z) {
  guis[tehgui->id].ZOrder = z;
  update_gui_zorder();
}

int GUI_GetZOrder(ScriptGUI *tehgui) {
  return guis[tehgui->id].ZOrder;
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
    return CreateNewScriptString(guis[tehgui->id].Name);
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
    return guis[tehgui->id].PopupAtMouseY;
}

void GUI_SetPopupYPos(ScriptGUI *tehgui, int newpos)
{
    if (!guis[tehgui->id].IsTextWindow())
        guis[tehgui->id].PopupAtMouseY = newpos;
}

void GUI_SetTransparency(ScriptGUI *tehgui, int trans) {
  if ((trans < 0) | (trans > 100))
    quit("!SetGUITransparency: transparency value must be between 0 and 100");

  guis[tehgui->id].SetTransparencyAsPercentage(trans);
}

int GUI_GetTransparency(ScriptGUI *tehgui) {
  return GfxDef::LegacyTrans255ToTrans100(guis[tehgui->id].Transparency);
}

void GUI_Centre(ScriptGUI *sgui) {
  GUIMain *tehgui = &guis[sgui->id];
  tehgui->X = play.GetUIViewport().GetWidth() / 2 - tehgui->Width / 2;
  tehgui->Y = play.GetUIViewport().GetHeight() / 2 - tehgui->Height / 2;
}

void GUI_SetBackgroundGraphic(ScriptGUI *tehgui, int slotn) {
  if (guis[tehgui->id].BgImage != slotn) {
    guis[tehgui->id].BgImage = slotn;
    guis[tehgui->id].MarkChanged();
  }
}

int GUI_GetBackgroundGraphic(ScriptGUI *tehgui) {
  if (guis[tehgui->id].BgImage < 1)
    return 0;
  return guis[tehgui->id].BgImage;
}

void GUI_SetBackgroundColor(ScriptGUI *tehgui, int newcol)
{
    if (guis[tehgui->id].BgColor != newcol)
    {
        guis[tehgui->id].BgColor = newcol;
        guis[tehgui->id].MarkChanged();
    }
}

int GUI_GetBackgroundColor(ScriptGUI *tehgui)
{
    return guis[tehgui->id].BgColor;
}

void GUI_SetBorderColor(ScriptGUI *tehgui, int newcol)
{
    if (guis[tehgui->id].IsTextWindow())
        return;
    if (guis[tehgui->id].FgColor != newcol)
    {
        guis[tehgui->id].FgColor = newcol;
        guis[tehgui->id].MarkChanged();
    }
}

int GUI_GetBorderColor(ScriptGUI *tehgui)
{
    if (guis[tehgui->id].IsTextWindow())
        return 0;
    return guis[tehgui->id].FgColor;
}

void GUI_SetTextColor(ScriptGUI *tehgui, int newcol)
{
    if (!guis[tehgui->id].IsTextWindow())
        return;
    if (guis[tehgui->id].FgColor != newcol)
    {
        guis[tehgui->id].FgColor = newcol;
        guis[tehgui->id].MarkChanged();
    }
}

int GUI_GetTextColor(ScriptGUI *tehgui)
{
    if (!guis[tehgui->id].IsTextWindow())
        return 0;
    return guis[tehgui->id].FgColor;
}

int GUI_GetTextPadding(ScriptGUI *tehgui)
{
    return guis[tehgui->id].Padding;
}

void GUI_SetTextPadding(ScriptGUI *tehgui, int newpos)
{
    if (guis[tehgui->id].IsTextWindow())
        guis[tehgui->id].Padding = newpos;
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
    ifacepopped=-1; UnPauseGame();
    guis[ifacenum].SetConceal(true);
    if (mousey<=guis[ifacenum].PopupAtMouseY)
        Mouse::SetPosition(Point(mousex, guis[ifacenum].PopupAtMouseY+2));
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
        QueueScriptFunction(kScTypeGame, ScriptFunctionRef(guis[ifce].ScriptModule, guis[ifce].OnClickHandler), 2, params);
        return;
    }

    int btype = guis[ifce].GetControlType(btn);
    int rtype=kGUIAction_None,rdata=0;
    if (btype==kGUIButton) {
        GUIButton*gbuto=(GUIButton*)guis[ifce].GetControl(btn);
        rtype=gbuto->ClickAction[kGUIClickLeft];
        rdata=gbuto->ClickData[kGUIClickLeft];
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
        if ((theObj->GetEventCount() > 0) &&
            (!theObj->EventHandlers[0].IsEmpty()) &&
            DoesScriptFunctionExistInModules(theObj->EventHandlers[0]))
        {
            // control-specific event handler
            const ScriptFunctionRef fn_ref(guis[ifce].ScriptModule, theObj->EventHandlers[0]);
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

// FIXME: rewrite this awful code, use ready GUILabelMacro (?)
void replace_macro_tokens(const char *text, String &fixed_text) {
    const char*curptr=&text[0];
    char tmpm[3];
    const char*endat = curptr + strlen(text);
    fixed_text.Empty();
    char tempo[STD_BUFFER_SIZE];

    while (1) {
        if (curptr[0]==0) break;
        if (curptr>=endat) break;
        if (curptr[0]=='@') {
            const char *curptrWasAt = curptr;
            char macroname[21]; int idd=0; curptr++;
            for (idd=0;idd<20;idd++) {
                if (curptr[0]=='@') {
                    macroname[idd]=0;
                    curptr++;
                    break;
                }
                // unterminated macro (eg. "@SCORETEXT"), so abort
                if (curptr[0] == 0)
                    break;
                macroname[idd]=curptr[0];
                curptr++;
            }
            macroname[idd]=0; 
            tempo[0]=0;
            if (ags_stricmp(macroname,"score")==0)
                snprintf(tempo, sizeof(tempo), "%d",play.score);
            else if (ags_stricmp(macroname,"totalscore")==0)
                snprintf(tempo, sizeof(tempo), "%d",MAXSCORE);
            else if (ags_stricmp(macroname,"scoretext")==0)
                snprintf(tempo, sizeof(tempo), "%d of %d",play.score,MAXSCORE);
            else if (ags_stricmp(macroname,"gamename")==0)
                snprintf(tempo, sizeof(tempo), "%s", play.game_name.GetCStr());
            else if (ags_stricmp(macroname,"overhotspot")==0) {
                // While game is in Wait mode, or in room transition: no overhotspot text
                if (!IsInterfaceEnabled() || in_room_transition)
                    tempo[0] = 0;
                else
                    GetLocationNameInBuf(game_to_data_coord(mousex), game_to_data_coord(mousey), tempo);
            }
            else { // not a macro, there's just a @ in the message
                curptr = curptrWasAt + 1;
                snprintf(tempo, sizeof(tempo), "%s", "@");
            }

            fixed_text.Append(tempo);
        }
        else {
            tmpm[0]=curptr[0]; tmpm[1]=0;
            fixed_text.Append(tmpm);
            curptr++;
        }
    }
}

bool sort_gui_less(const int g1, const int g2)
{
    return (guis[g1].ZOrder < guis[g2].ZOrder) ||
        ((guis[g1].ZOrder == guis[g2].ZOrder) && (g1 < g2));
}

void update_gui_zorder()
{
    std::sort(play.gui_draw_order.begin(), play.gui_draw_order.end(), sort_gui_less);
}

void prepare_gui_runtime(bool startup)
{
    // Trigger all guis and controls to recalculate their dynamic state;
    // here we achieve this by sending "On Resize" event, although there could
    // be a better way for this.
    for (auto &gui : guis)
    {
        for (int i = 0; i < gui.GetControlCount(); ++i)
        {
            GUIObject *guio = gui.GetControl(i);
            guio->IsActivated = false;
            guio->OnResized();
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
        if (!guio->Name.IsEmpty())
            ccAddExternalScriptObject(guio->Name, guio, &ccDynamicGUIObject);
        ccRegisterManagedObject(guio, &ccDynamicGUIObject);
    }
}

void unexport_gui_controls(int ee)
{
    for (int ff = 0; ff < guis[ee].GetControlCount(); ff++)
    {
        GUIObject *guio = guis[ee].GetControl(ff);
        if (!guio->Name.IsEmpty())
            ccRemoveExternalSymbol(guio->Name);
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
        GUIE::MarkAllGUIForUpdate(GUI::Options.DisabledStyle != kGuiDis_Unchanged, true);
        if (GUI::Options.DisabledStyle != kGuiDis_Unchanged)
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
        !IsRectInsideRect(play.GetUIViewport(), RectWH(gui.X, gui.Y, gui.Width, gui.Height)) ||
        // fully transparent (? FIXME: this only checks background, but not controls)
        ((gui.BgColor == 0) && (gui.BgImage < 1)) || (gui.Transparency == 255);
}

int adjust_x_for_guis(int x, int y, bool assume_blocking)
{
    if ((game.options[OPT_DISABLEOFF] == kGuiDis_Off) &&
        ((GUI::Context.DisabledState != kGuiDis_Undefined) || assume_blocking))
        return x; // All GUI off (or will be when the message is displayed)
    // If it's covered by a GUI, move it right a bit
    // FIXME: should not we also account for the text's width here?
    //        and then, maybe we should also able to move it left if rightmost part of text touches GUI?
    for (const auto &gui : guis)
    {
        if (should_skip_adjust_for_gui(gui))
            continue;
        // lower or higher than the message
        if ((gui.Y > y) || (gui.Y + gui.Height < y))
            continue;
        // to the left or to the right from the message
        if ((gui.X > x) || (gui.X + gui.Width < x))
            continue;
        // try to deal with very wide GUIs
        const float gui_width_cap = 0.75f; // NOTE: originally was 280 pixels in 320-wide game
        if (gui.Width >= game.GetGameRes().Width * gui_width_cap)
            continue;
        // fix coordinates if x is inside the gui
        x = gui.X + gui.Width + 2;
    }
    return x;
}

int adjust_y_for_guis(int y, bool assume_blocking)
{
    if ((game.options[OPT_DISABLEOFF] == kGuiDis_Off) &&
        ((GUI::Context.DisabledState >= 0) || assume_blocking))
        return y; // All GUI off (or will be when the message is displayed)
    // If it's covered by a GUI, move it down a bit
    // FIXME: should not we also account for the text's height here?
    //        and then, maybe we should also able to move it up if lower part of text touches GUI below?
    for (const auto &gui : guis)
    {
        if (should_skip_adjust_for_gui(gui))
            continue;
        // lower or higher than the message
        if ((gui.Y > y) || (gui.Y + gui.Height < y))
            continue;
        // try to deal with tall GUIs; CHECKME this later
        const float gui_height_cap = 0.25f; // NOTE: originally was 50 pixels in 200-height game
        if (gui.Height >= game.GetGameRes().Height * gui_height_cap)
            continue;
        // fix coordinates if y is inside the gui
        y = gui.Y + gui.Height + 2;
    }
    return y;
}

int gui_get_interactable(int x,int y)
{
    if ((game.options[OPT_DISABLEOFF] == kGuiDis_Off) && (GUI::Context.DisabledState >= 0))
        return -1;
    return GetGUIAt(x, y);
}

int gui_on_mouse_move(const int mx, const int my)
{
    int mouse_over_gui = -1;
    // If all GUIs are off, skip the loop
    if ((game.options[OPT_DISABLEOFF] == kGuiDis_Off) && (GUI::Context.DisabledState >= 0)) ;
    else {
        // Scan for mouse-y-pos GUIs, and pop one up if appropriate
        // Also work out the mouse-over GUI while we're at it
        // CHECKME: not sure why, but we're testing forward draw order here -
        // from farthest to nearest (this was in original code?)
        for (int guin : play.gui_draw_order) {
            if (guis[guin].IsInteractableAt(mx, my)) mouse_over_gui=guin;

            if (guis[guin].PopupStyle!=kGUIPopupMouseY) continue;
            if (play.complete_overlay_on > 0) break;  // interfaces disabled
            if (ifacepopped==guin) continue;
            if (!guis[guin].IsVisible()) continue;
            // Don't allow it to be popped up while skipping cutscene
            if (play.fast_forward) continue;

            if (mousey < guis[guin].PopupAtMouseY) {
                set_mouse_cursor(CURS_ARROW);
                guis[guin].SetConceal(false);
                ifacepopped=guin; PauseGame();
                break;
            }
        }
    }
    return mouse_over_gui;
}

void gui_on_mouse_hold(const int wasongui, const int wasbutdown)
{
    for (int i=0;i<guis[wasongui].GetControlCount();i++) {
        GUIObject *guio = guis[wasongui].GetControl(i);
        if (!guio->IsActivated) continue;
        if (guis[wasongui].GetControlType(i)!=kGUISlider) continue;
        // GUI Slider repeatedly activates while being dragged
        guio->IsActivated = false;
        force_event(AGSEvent_GUI(wasongui, i, static_cast<eAGSMouseButton>(wasbutdown)));
        break;
    }
}

void gui_on_mouse_up(const int wasongui, const int wasbutdown, const int mx, const int my)
{
    guis[wasongui].OnMouseButtonUp();

    for (int i=0;i<guis[wasongui].GetControlCount();i++) {
        GUIObject *guio = guis[wasongui].GetControl(i);
        if (!guio->IsActivated) continue;
        guio->IsActivated = false;
        if (!IsInterfaceEnabled()) break;

        int cttype=guis[wasongui].GetControlType(i);
        if ((cttype == kGUIButton) || (cttype == kGUISlider) || (cttype == kGUIListBox)) {
            force_event(AGSEvent_GUI(wasongui, i, static_cast<eAGSMouseButton>(wasbutdown)));
        }
        else if (cttype == kGUIInvWindow) {
            mouse_ifacebut_xoffs=mx-(guio->X)-guis[wasongui].X;
            mouse_ifacebut_yoffs=my-(guio->Y)-guis[wasongui].Y;
            int iit=offset_over_inv((GUIInvWindow*)guio);
            if (iit>=0) {
                play.used_inv_on = iit;
                if (game.options[OPT_HANDLEINVCLICKS]) {
                    // Let the script handle the click
                    // LEFTINV is 5, RIGHTINV is 6
                    force_event(AGSEvent_Script(kTS_MouseClick, wasbutdown + 4, mx, my));
                }
                else if (wasbutdown == kMouseRight) // right-click is always Look
                    RunInventoryInteraction(iit, MODE_LOOK);
                else if (cur_mode == MODE_HAND)
                    SetActiveInventory(iit);
                else
                    RunInventoryInteraction (iit, cur_mode);
            }
        }
        else quit("clicked on unknown control type");
        if (guis[wasongui].PopupStyle==kGUIPopupMouseY)
            remove_popup_interface(wasongui);
        break;
    }

    run_on_event(kScriptEvent_GUIMouseUp, wasongui, wasbutdown, mx - guis[wasongui].X, my - guis[wasongui].Y);
}

void gui_on_mouse_down(const int guin, const int mbut, const int mx, const int my)
{
    debug_script_log("Mouse click over GUI %d", guin);
    guis[guin].OnMouseButtonDown(mx, my);
    // run GUI click handler if not on any control
    if ((guis[guin].MouseDownCtrl < 0) && (!guis[guin].OnClickHandler.IsEmpty()))
        force_event(AGSEvent_GUI(guin, -1, static_cast<eAGSMouseButton>(mbut)));

    run_on_event(kScriptEvent_GUIMouseDown, guin, mbut, mx - guis[guin].X, my - guis[guin].Y);
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
        { "GUI::get_PopupYPos",           API_FN_PAIR(GUI_GetPopupYPos) },
        { "GUI::set_PopupYPos",           API_FN_PAIR(GUI_SetPopupYPos) },
        { "GUI::get_ScriptName",          API_FN_PAIR(GUI_GetScriptName) },
        { "TextWindowGUI::get_TextColor", API_FN_PAIR(GUI_GetTextColor) },
        { "TextWindowGUI::set_TextColor", API_FN_PAIR(GUI_SetTextColor) },
        { "TextWindowGUI::get_TextPadding", API_FN_PAIR(GUI_GetTextPadding) },
        { "TextWindowGUI::set_TextPadding", API_FN_PAIR(GUI_SetTextPadding) },
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
    };

    ccAddExternalFunctions(gui_api);
}
