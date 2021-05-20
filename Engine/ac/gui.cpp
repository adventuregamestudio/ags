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
#include <cstdio>
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
#include "ac/system.h"
#include "ac/dynobj/cc_guiobject.h"
#include "ac/dynobj/scriptgui.h"
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
#include "util/geometry.h"
#include "util/string_compat.h"


using namespace AGS::Common;
using namespace AGS::Engine;


extern GameSetup usetup;
extern RoomStruct thisroom;
extern int cur_mode,cur_cursor;
extern ccInstance *gameinst;
extern ScriptGUI *scrGui;
extern GameSetupStruct game;
extern CCGUIObject ccDynamicGUIObject;
extern Bitmap **guibg;
extern IDriverDependantBitmap **guibgbmp;
extern IGraphicsDriver *gfxDriver;

extern CCGUI ccDynamicGUI;
extern CCGUIObject ccDynamicGUIObject;


int ifacepopped=-1;  // currently displayed pop-up GUI (-1 if none)
int mouse_on_iface=-1;   // mouse cursor is over this interface
int mouse_ifacebut_xoffs=-1,mouse_ifacebut_yoffs=-1;

int eip_guinum, eip_guiobj;


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
  return guis[tehgui->id].X;
}

void GUI_SetX(ScriptGUI *tehgui, int x) {
  guis[tehgui->id].SetAt(x, guis[tehgui->id].Y);
}

int GUI_GetY(ScriptGUI *tehgui) {
  return guis[tehgui->id].Y;
}

void GUI_SetY(ScriptGUI *tehgui, int y) {
    guis[tehgui->id].SetAt(guis[tehgui->id].X, y);
}

void GUI_SetPosition(ScriptGUI *tehgui, int x, int y) {
    guis[tehgui->id].SetAt(x, y);
}

void GUI_SetSize(ScriptGUI *sgui, int w, int h) {
  if ((w < 1) || (h < 1))
    quitprintf("!SetGUISize: invalid dimensions (tried to set to %d x %d)", w, h);

  GUIMain *tehgui = &guis[sgui->id];

  if ((tehgui->Width == w) && (tehgui->Height == h))
    return;
  
  tehgui->SetSize(w, h);
  tehgui->MarkChanged();
}

int GUI_GetWidth(ScriptGUI *sgui) {
  return guis[sgui->id].Width;
}

int GUI_GetHeight(ScriptGUI *sgui) {
  return guis[sgui->id].Height;
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
  int x = play.GetUIViewport().GetWidth() / 2 - tehgui->Width / 2;
  int y = play.GetUIViewport().GetHeight() / 2 - tehgui->Height / 2;
  tehgui->SetAt(x, y);
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
    {
        guis[guiid].Poll(x, y);
        gui_on_mouse_down(guiid, mbut);
        gui_on_mouse_up(guiid, mbut);
    }
}

int GUI_GetBlendMode(ScriptGUI *gui) {
    return guis[gui->id].BlendMode;
}

void GUI_SetBlendMode(ScriptGUI *gui, int blendMode) {
    if ((blendMode < 0) || (blendMode >= kNumBlendModes))
        quitprintf("!SetBlendMode: invalid blend mode %d, supported modes are %d - %d", blendMode, 0, kNumBlendModes - 1);
    guis[gui->id].BlendMode = (BlendMode)blendMode;
}

float GUI_GetRotation(ScriptGUI *gui) {
    return guis[gui->id].Rotation;
}

void GUI_SetRotation(ScriptGUI *gui, float rotation) {
    guis[gui->id].SetRotation(rotation);
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
        QueueScriptFunction(kScInstGame, guis[ifce].OnClickHandler.GetCStr(), 2,
            RuntimeScriptValue().SetDynamicObject(&scrGui[ifce], &ccDynamicGUI),
            RuntimeScriptValue().SetInt32(mbut));
        return;
    }

    int btype = guis[ifce].GetControlType(btn);
    int rtype=kGUIAction_None,rdata;
    if (btype==kGUIButton) {
        GUIButton*gbuto=(GUIButton*)guis[ifce].GetControl(btn);
        rtype=gbuto->ClickAction[kMouseLeft];
        rdata=gbuto->ClickData[kMouseLeft];
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
            (!gameinst->GetSymbolAddress(theObj->EventHandlers[0].GetCStr()).IsNull())) {
                // control-specific event handler
                if (theObj->GetEventArgs(0).FindChar(',') != -1)
                    QueueScriptFunction(kScInstGame, theObj->EventHandlers[0].GetCStr(), 2,
                        RuntimeScriptValue().SetDynamicObject(theObj, &ccDynamicGUIObject),
                        RuntimeScriptValue().SetInt32(mbut));
                else
                    QueueScriptFunction(kScInstGame, theObj->EventHandlers[0].GetCStr(), 1,
                        RuntimeScriptValue().SetDynamicObject(theObj, &ccDynamicGUIObject));
        }
        else
            QueueScriptFunction(kScInstGame, "interface_click", 2,
                RuntimeScriptValue().SetInt32(ifce),
                RuntimeScriptValue().SetInt32(btn));
    }
}


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
                sprintf(tempo,"%d",play.score);
            else if (ags_stricmp(macroname,"totalscore")==0)
                sprintf(tempo,"%d",MAXSCORE);
            else if (ags_stricmp(macroname,"scoretext")==0)
                sprintf(tempo,"%d of %d",play.score,MAXSCORE);
            else if (ags_stricmp(macroname,"gamename")==0)
                strcpy(tempo, play.game_name);
            else if (ags_stricmp(macroname,"overhotspot")==0) {
                // While game is in Wait mode, no overhotspot text
                if (!IsInterfaceEnabled())
                    tempo[0] = 0;
                else
                    GetLocationName(mousex, mousey, tempo);
            }
            else { // not a macro, there's just a @ in the message
                curptr = curptrWasAt + 1;
                strcpy(tempo, "@");
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


void update_gui_zorder() {
    int numdone = 0, b;

    // for each GUI
    for (int a = 0; a < game.numgui; a++) {
        // find the right place in the draw order array
        int insertAt = numdone;
        for (b = 0; b < numdone; b++) {
            if (guis[a].ZOrder < guis[play.gui_draw_order[b]].ZOrder) {
                insertAt = b;
                break;
            }
        }
        // insert the new item
        for (b = numdone - 1; b >= insertAt; b--)
            play.gui_draw_order[b + 1] = play.gui_draw_order[b];
        play.gui_draw_order[insertAt] = a;
        numdone++;
    }

}


void export_gui_controls(int ee)
{
    for (int ff = 0; ff < guis[ee].GetControlCount(); ff++)
    {
        GUIObject *guio = guis[ee].GetControl(ff);
        if (!guio->Name.IsEmpty())
            ccAddExternalDynamicObject(guio->Name, guio, &ccDynamicGUIObject);
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

int convert_gui_disabled_style(int oldStyle) {
    int toret = GUIDIS_GREYOUT;

    // if GUIs Turn Off is selected, don't grey out buttons for
    // any Persistent GUIs which remain
    // set to 0x80 so that it is still non-zero, but has no effect
    if (oldStyle == 3)
        toret = GUIDIS_GUIOFF;
    // GUIs Go Black
    else if (oldStyle == 1)
        toret = GUIDIS_BLACKOUT;
    // GUIs unchanged
    else if (oldStyle == 2)
        toret = GUIDIS_UNCHANGED;

    return toret;
}

void update_gui_disabled_status() {
    // update GUI display status (perhaps we've gone into
    // an interface disabled state)
    int all_buttons_was = all_buttons_disabled;
    all_buttons_disabled = 0;

    if (!IsInterfaceEnabled()) {
        all_buttons_disabled = gui_disabled_style;
    }

    if (all_buttons_was != all_buttons_disabled) {
        if (gui_disabled_style != GUIDIS_UNCHANGED) {
            for (int aa = 0; aa < game.numgui; aa++) {
                guis[aa].OnControlPositionChanged(); // this marks GUI for update
            }
            invalidate_screen();
        }
    }
}


int adjust_x_for_guis (int xx, int yy) {
    if ((game.options[OPT_DISABLEOFF]==3) && (all_buttons_disabled > 0))
        return xx;
    // If it's covered by a GUI, move it right a bit
    for (int aa=0;aa < game.numgui; aa++) {
        if (!guis[aa].IsDisplayed())
            continue;
        if ((guis[aa].X > xx) || (guis[aa].Y > yy) || (guis[aa].Y + guis[aa].Height < yy))
            continue;
        // totally transparent GUI, ignore
        if (((guis[aa].BgColor == 0) && (guis[aa].BgImage < 1)) || (guis[aa].Transparency == 255))
            continue;

        // try to deal with full-width GUIs across the top
        if (guis[aa].X + guis[aa].Width >= 280)
            continue;

        if (xx < guis[aa].X + guis[aa].Width) 
            xx = guis[aa].X + guis[aa].Width + 2;        
    }
    return xx;
}

int adjust_y_for_guis ( int yy) {
    if ((game.options[OPT_DISABLEOFF]==3) && (all_buttons_disabled > 0))
        return yy;
    // If it's covered by a GUI, move it down a bit
    for (int aa=0;aa < game.numgui; aa++) {
        if (!guis[aa].IsDisplayed())
            continue;
        if (guis[aa].Y > yy)
            continue;
        // totally transparent GUI, ignore
        if (((guis[aa].BgColor == 0) && (guis[aa].BgImage < 1)) || (guis[aa].Transparency == 255))
            continue;

        // try to deal with full-height GUIs down the left or right
        if (guis[aa].Height > 50)
            continue;

        if (yy < guis[aa].Y + guis[aa].Height) 
            yy = guis[aa].Y + guis[aa].Height + 2;        
    }
    return yy;
}

void recreate_guibg_image(GUIMain *tehgui)
{
  // Calculate all supported GUI transforms
  const int ifn = tehgui->ID;
  Size final_sz = gfxDriver->HasAcceleratedTransform() ?
      Size(tehgui->Width, tehgui->Height) :
      RotateSize(Size(tehgui->Width, tehgui->Height), tehgui->Rotation);
  if (guibg[ifn] && guibg[ifn]->GetSize() == final_sz)
    return; // all is fine
      
  delete guibg[ifn];
  guibg[ifn] = new Bitmap(final_sz.Width, final_sz.Height, gfxDriver->GetCompatibleBitmapFormat(game.GetColorDepth()));
  if (guibgbmp[ifn] != nullptr)
  {
    gfxDriver->DestroyDDB(guibgbmp[ifn]);
    guibgbmp[ifn] = nullptr;
  }
}

int gui_get_interactable(int x,int y)
{
    if ((game.options[OPT_DISABLEOFF]==3) && (all_buttons_disabled > 0))
        return -1;
    return GetGUIAt(x, y);
}

int gui_on_mouse_move()
{
    int mouse_over_gui = -1;
    // If all GUIs are off, skip the loop
    if ((game.options[OPT_DISABLEOFF]==3) && (all_buttons_disabled > 0)) ;
    else {
        // Scan for mouse-y-pos GUIs, and pop one up if appropriate
        // Also work out the mouse-over GUI while we're at it
        int ll;
        for (ll = 0; ll < game.numgui;ll++) {
            const int guin = play.gui_draw_order[ll];
            if (guis[guin].IsInteractableAt(mousex, mousey)) mouse_over_gui=guin;

            if (guis[guin].PopupStyle!=kGUIPopupMouseY) continue;
            if (play.complete_overlay_on > 0) break;  // interfaces disabled
            //    if (play.disabled_user_interface>0) break;
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
        force_event(EV_IFACECLICK, wasongui, i, wasbutdown);
        break;
    }
}

void gui_on_mouse_up(const int wasongui, const int wasbutdown)
{
    guis[wasongui].OnMouseButtonUp();

    for (int i=0;i<guis[wasongui].GetControlCount();i++) {
        GUIObject *guio = guis[wasongui].GetControl(i);
        if (!guio->IsActivated) continue;
        guio->IsActivated = false;
        if (!IsInterfaceEnabled()) break;

        int cttype=guis[wasongui].GetControlType(i);
        if ((cttype == kGUIButton) || (cttype == kGUISlider) || (cttype == kGUIListBox)) {
            force_event(EV_IFACECLICK, wasongui, i, wasbutdown);
        }
        else if (cttype == kGUIInvWindow) {
            mouse_ifacebut_xoffs=mousex-(guio->X)-guis[wasongui].X;
            mouse_ifacebut_yoffs=mousey-(guio->Y)-guis[wasongui].Y;
            int iit=offset_over_inv((GUIInvWindow*)guio);
            if (iit>=0) {
                evblocknum=iit;
                play.used_inv_on = iit;
                if (game.options[OPT_HANDLEINVCLICKS]) {
                    // Let the script handle the click
                    // LEFTINV is 5, RIGHTINV is 6
                    force_event(EV_TEXTSCRIPT,TS_MCLICK, wasbutdown + 4);
                }
                else if (wasbutdown==2)  // right-click is always Look
                    run_event_block_inv(iit, 0);
                else if (cur_mode == MODE_HAND)
                    SetActiveInventory(iit);
                else
                    RunInventoryInteraction (iit, cur_mode);
                evblocknum=-1;
            }
        }
        else quit("clicked on unknown control type");
        if (guis[wasongui].PopupStyle==kGUIPopupMouseY)
            remove_popup_interface(wasongui);
        break;
    }

    run_on_event(GE_GUI_MOUSEUP, RuntimeScriptValue().SetInt32(wasongui));
}

void gui_on_mouse_down(const int guin, const int mbut)
{
    debug_script_log("Mouse click over GUI %d", guin);
    guis[guin].OnMouseButtonDown(mousex, mousey);
    // run GUI click handler if not on any control
    if ((guis[guin].MouseDownCtrl < 0) && (!guis[guin].OnClickHandler.IsEmpty()))
        force_event(EV_IFACECLICK, guin, -1, mbut);

    run_on_event(GE_GUI_MOUSEDOWN, RuntimeScriptValue().SetInt32(guin));
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

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

// int (ScriptGUI *gui)
RuntimeScriptValue Sc_GUI_GetBlendMode(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetBlendMode);
}

// void (ScriptGUI *gui, int blendMode)
RuntimeScriptValue Sc_GUI_SetBlendMode(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetBlendMode);
}

RuntimeScriptValue Sc_GUI_GetRotation(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(ScriptGUI, GUI_GetRotation);
}

RuntimeScriptValue Sc_GUI_SetRotation(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(ScriptGUI, GUI_SetRotation);
}

void RegisterGUIAPI()
{
    ccAddExternalObjectFunction("GUI::Centre^0",                Sc_GUI_Centre);
    ccAddExternalObjectFunction("GUI::Click^1",                 Sc_GUI_Click);
    ccAddExternalStaticFunction("GUI::GetAtScreenXY^2",         Sc_GetGUIAtLocation);
    ccAddExternalStaticFunction("GUI::ProcessClick^3",          Sc_GUI_ProcessClick);
    ccAddExternalObjectFunction("GUI::SetPosition^2",           Sc_GUI_SetPosition);
    ccAddExternalObjectFunction("GUI::SetSize^2",               Sc_GUI_SetSize);
    ccAddExternalObjectFunction("GUI::get_BackgroundGraphic",   Sc_GUI_GetBackgroundGraphic);
    ccAddExternalObjectFunction("GUI::set_BackgroundGraphic",   Sc_GUI_SetBackgroundGraphic);
    ccAddExternalObjectFunction("GUI::get_BackgroundColor",     Sc_GUI_GetBackgroundColor);
    ccAddExternalObjectFunction("GUI::set_BackgroundColor",     Sc_GUI_SetBackgroundColor);
    ccAddExternalObjectFunction("GUI::get_BorderColor",         Sc_GUI_GetBorderColor);
    ccAddExternalObjectFunction("GUI::set_BorderColor",         Sc_GUI_SetBorderColor);
    ccAddExternalObjectFunction("GUI::get_Clickable",           Sc_GUI_GetClickable);
    ccAddExternalObjectFunction("GUI::set_Clickable",           Sc_GUI_SetClickable);
    ccAddExternalObjectFunction("GUI::get_ControlCount",        Sc_GUI_GetControlCount);
    ccAddExternalObjectFunction("GUI::geti_Controls",           Sc_GUI_GetiControls);
    ccAddExternalObjectFunction("GUI::get_Height",              Sc_GUI_GetHeight);
    ccAddExternalObjectFunction("GUI::set_Height",              Sc_GUI_SetHeight);
    ccAddExternalObjectFunction("GUI::get_ID",                  Sc_GUI_GetID);
    ccAddExternalObjectFunction("GUI::get_AsTextWindow",        Sc_GUI_AsTextWindow);
    ccAddExternalObjectFunction("GUI::get_PopupStyle",          Sc_GUI_GetPopupStyle);
    ccAddExternalObjectFunction("GUI::get_PopupYPos",           Sc_GUI_GetPopupYPos);
    ccAddExternalObjectFunction("GUI::set_PopupYPos",           Sc_GUI_SetPopupYPos);
    ccAddExternalObjectFunction("TextWindowGUI::get_TextColor", Sc_GUI_GetTextColor);
    ccAddExternalObjectFunction("TextWindowGUI::set_TextColor", Sc_GUI_SetTextColor);
    ccAddExternalObjectFunction("TextWindowGUI::get_TextPadding", Sc_GUI_GetTextPadding);
    ccAddExternalObjectFunction("TextWindowGUI::set_TextPadding", Sc_GUI_SetTextPadding);
    ccAddExternalObjectFunction("GUI::get_Transparency",        Sc_GUI_GetTransparency);
    ccAddExternalObjectFunction("GUI::set_Transparency",        Sc_GUI_SetTransparency);
    ccAddExternalObjectFunction("GUI::get_Visible",             Sc_GUI_GetVisible);
    ccAddExternalObjectFunction("GUI::set_Visible",             Sc_GUI_SetVisible);
    ccAddExternalObjectFunction("GUI::get_Width",               Sc_GUI_GetWidth);
    ccAddExternalObjectFunction("GUI::set_Width",               Sc_GUI_SetWidth);
    ccAddExternalObjectFunction("GUI::get_X",                   Sc_GUI_GetX);
    ccAddExternalObjectFunction("GUI::set_X",                   Sc_GUI_SetX);
    ccAddExternalObjectFunction("GUI::get_Y",                   Sc_GUI_GetY);
    ccAddExternalObjectFunction("GUI::set_Y",                   Sc_GUI_SetY);
    ccAddExternalObjectFunction("GUI::get_ZOrder",              Sc_GUI_GetZOrder);
    ccAddExternalObjectFunction("GUI::set_ZOrder",              Sc_GUI_SetZOrder);
    ccAddExternalObjectFunction("GUI::get_Shown",               Sc_GUI_GetShown);
    ccAddExternalObjectFunction("GUI::get_BlendMode",           Sc_GUI_GetBlendMode);
    ccAddExternalObjectFunction("GUI::set_BlendMode",           Sc_GUI_SetBlendMode);
    ccAddExternalObjectFunction("GUI::get_Rotation",            Sc_GUI_GetRotation);
    ccAddExternalObjectFunction("GUI::set_Rotation",            Sc_GUI_SetRotation);

    /* ----------------------- Registering unsafe exports for plugins -----------------------*/

    ccAddExternalFunctionForPlugin("GUI::Centre^0",                (void*)GUI_Centre);
    ccAddExternalFunctionForPlugin("GUI::GetAtScreenXY^2",         (void*)GetGUIAtLocation);
    ccAddExternalFunctionForPlugin("GUI::SetPosition^2",           (void*)GUI_SetPosition);
    ccAddExternalFunctionForPlugin("GUI::SetSize^2",               (void*)GUI_SetSize);
    ccAddExternalFunctionForPlugin("GUI::get_BackgroundGraphic",   (void*)GUI_GetBackgroundGraphic);
    ccAddExternalFunctionForPlugin("GUI::set_BackgroundGraphic",   (void*)GUI_SetBackgroundGraphic);
    ccAddExternalFunctionForPlugin("GUI::get_Clickable",           (void*)GUI_GetClickable);
    ccAddExternalFunctionForPlugin("GUI::set_Clickable",           (void*)GUI_SetClickable);
    ccAddExternalFunctionForPlugin("GUI::get_ControlCount",        (void*)GUI_GetControlCount);
    ccAddExternalFunctionForPlugin("GUI::geti_Controls",           (void*)GUI_GetiControls);
    ccAddExternalFunctionForPlugin("GUI::get_Height",              (void*)GUI_GetHeight);
    ccAddExternalFunctionForPlugin("GUI::set_Height",              (void*)GUI_SetHeight);
    ccAddExternalFunctionForPlugin("GUI::get_ID",                  (void*)GUI_GetID);
    ccAddExternalFunctionForPlugin("GUI::get_Transparency",        (void*)GUI_GetTransparency);
    ccAddExternalFunctionForPlugin("GUI::set_Transparency",        (void*)GUI_SetTransparency);
    ccAddExternalFunctionForPlugin("GUI::get_Visible",             (void*)GUI_GetVisible);
    ccAddExternalFunctionForPlugin("GUI::set_Visible",             (void*)GUI_SetVisible);
    ccAddExternalFunctionForPlugin("GUI::get_Width",               (void*)GUI_GetWidth);
    ccAddExternalFunctionForPlugin("GUI::set_Width",               (void*)GUI_SetWidth);
    ccAddExternalFunctionForPlugin("GUI::get_X",                   (void*)GUI_GetX);
    ccAddExternalFunctionForPlugin("GUI::set_X",                   (void*)GUI_SetX);
    ccAddExternalFunctionForPlugin("GUI::get_Y",                   (void*)GUI_GetY);
    ccAddExternalFunctionForPlugin("GUI::set_Y",                   (void*)GUI_SetY);
    ccAddExternalFunctionForPlugin("GUI::get_ZOrder",              (void*)GUI_GetZOrder);
    ccAddExternalFunctionForPlugin("GUI::set_ZOrder",              (void*)GUI_SetZOrder);
}
