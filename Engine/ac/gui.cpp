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

#include <stdio.h>
#include "ac/gui.h"
#include "ac/common.h"
#include "ac/draw.h"
#include "ac/event.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/global_character.h"
#include "ac/global_game.h"
#include "ac/global_gui.h"
#include "ac/global_inventoryitem.h"
#include "ac/global_screen.h"
#include "ac/guicontrol.h"
#include "ac/interfacebutton.h"
#include "ac/invwindow.h"
#include "ac/mouse.h"
#include "ac/roomstruct.h"
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


void GUI_SetVisible(ScriptGUI *tehgui, int isvisible) {
  if (isvisible)
    InterfaceOn(tehgui->id);
  else
    InterfaceOff(tehgui->id);
}

int GUI_GetVisible(ScriptGUI *tehgui) {
  // GUI_GetVisible is slightly different from IsGUIOn, because
  // with a mouse ypos gui it returns 1 if the GUI is enabled,
  // whereas IsGUIOn actually checks if it is displayed
  if (!guis[tehgui->id].IsOff())
    return 1;
  return 0;
}

int GUI_GetX(ScriptGUI *tehgui) {
  return guis[tehgui->id].X;
}

void GUI_SetX(ScriptGUI *tehgui, int xx) {
  guis[tehgui->id].X = xx;
}

int GUI_GetY(ScriptGUI *tehgui) {
  return guis[tehgui->id].Y;
}

void GUI_SetY(ScriptGUI *tehgui, int yy) {
  guis[tehgui->id].Y = yy;
}

void GUI_SetPosition(ScriptGUI *tehgui, int xx, int yy) {
  GUI_SetX(tehgui, xx);
  GUI_SetY(tehgui, yy);
}

void GUI_SetSize(ScriptGUI *sgui, int widd, int hitt) {
  if ((widd < 1) || (hitt < 1) || (widd > BASEWIDTH) || (hitt > BASEHEIGHT))
    quitprintf("!SetGUISize: invalid dimensions (tried to set to %d x %d)", widd, hitt);

  GUIMain *tehgui = &guis[sgui->id];

  if ((tehgui->Width == widd) && (tehgui->Height == hitt))
    return;
  
  tehgui->Width = widd;
  tehgui->Height = hitt;
  
  recreate_guibg_image(tehgui);

  guis_need_update = 1;
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
  guis[tehgui->id].Flags &= ~kGUIMain_NoClick;
  if (clickable == 0)
    guis[tehgui->id].Flags |= kGUIMain_NoClick;
}

int GUI_GetClickable(ScriptGUI *tehgui) {
  if (guis[tehgui->id].Flags & kGUIMain_NoClick)
    return 0;
  return 1;
}

int GUI_GetID(ScriptGUI *tehgui) {
  return tehgui->id;
}

GUIObject* GUI_GetiControls(ScriptGUI *tehgui, int idx) {
  if ((idx < 0) || (idx >= guis[tehgui->id].ControlCount))
    return NULL;
  return guis[tehgui->id].Controls[idx];
}

int GUI_GetControlCount(ScriptGUI *tehgui) {
  return guis[tehgui->id].ControlCount;
}

void GUI_SetTransparency(ScriptGUI *tehgui, int trans) {
  if ((trans < 0) | (trans > 100))
    quit("!SetGUITransparency: transparency value must be between 0 and 100");

  guis[tehgui->id].SetTransparencyAsPercentage(trans);
}

int GUI_GetTransparency(ScriptGUI *tehgui) {
  if (guis[tehgui->id].Transparency == 0)
    return 0;
  if (guis[tehgui->id].Transparency == 255)
    return 100;

  return 100 - ((guis[tehgui->id].Transparency * 10) / 25);
}

void GUI_Centre(ScriptGUI *sgui) {
  GUIMain *tehgui = &guis[sgui->id];
  tehgui->X = play.viewport.GetWidth() / 2 - tehgui->Width / 2;
  tehgui->Y = play.viewport.GetHeight() / 2 - tehgui->Height / 2;
}

void GUI_SetBackgroundGraphic(ScriptGUI *tehgui, int slotn) {
  if (guis[tehgui->id].BgImage != slotn) {
    guis[tehgui->id].BgImage = slotn;
    guis_need_update = 1;
  }
}

int GUI_GetBackgroundGraphic(ScriptGUI *tehgui) {
  if (guis[tehgui->id].BgImage < 1)
    return 0;
  return guis[tehgui->id].BgImage;
}

ScriptGUI *GetGUIAtLocation(int xx, int yy) {
    int guiid = GetGUIAt(xx, yy);
    if (guiid < 0)
        return NULL;
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
        const int real_mousex = mousex;
        const int real_mousey = mousey;
        mousex = x;
        mousey = y;
        guis[guiid].Poll();
        gui_on_mouse_down(guiid, mbut);
        gui_on_mouse_up(guiid, mbut);
        mousex = real_mousex;
        mousey = real_mousey;
    }
}

//=============================================================================

void remove_popup_interface(int ifacenum) {
    if (ifacepopped != ifacenum) return;
    ifacepopped=-1; UnPauseGame();
    guis[ifacenum].SetVisibility(kGUIVisibility_Off);
    if (mousey<=guis[ifacenum].PopupAtMouseY)
        Mouse::SetPosition(Point(mousex, guis[ifacenum].PopupAtMouseY+2));
    if ((!IsInterfaceEnabled()) && (cur_cursor == cur_mode))
        // Only change the mouse cursor if it hasn't been specifically changed first
        set_mouse_cursor(CURS_WAIT);
    else if (IsInterfaceEnabled())
        set_default_cursor();

    if (ifacenum==mouse_on_iface) mouse_on_iface=-1;
    guis_need_update = 1;
}

void process_interface_click(int ifce, int btn, int mbut) {
    if (btn < 0) {
        // click on GUI background
        QueueScriptFunction(kScInstGame, guis[ifce].OnClickHandler, 2,
            RuntimeScriptValue().SetDynamicObject(&scrGui[ifce], &ccDynamicGUI),
            RuntimeScriptValue().SetInt32(mbut));
        return;
    }

    int btype=(guis[ifce].CtrlRefs[btn] >> 16) & 0x000ffff;
    int rtype=kGUIAction_None,rdata;
    if (btype==kGUIButton) {
        GUIButton*gbuto=(GUIButton*)guis[ifce].Controls[btn];
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
        GUIObject *theObj = guis[ifce].Controls[btn];
        // if the object has a special handler script then run it;
        // otherwise, run interface_click
        if ((theObj->GetEventCount() > 0) &&
            (!theObj->EventHandlers[0].IsEmpty()) &&
            (!gameinst->GetSymbolAddress(theObj->EventHandlers[0]).IsNull())) {
                // control-specific event handler
                if (strchr(theObj->GetEventArgs(0), ',') != NULL)
                    QueueScriptFunction(kScInstGame, theObj->EventHandlers[0], 2,
                        RuntimeScriptValue().SetDynamicObject(theObj, &ccDynamicGUIObject),
                        RuntimeScriptValue().SetInt32(mbut));
                else
                    QueueScriptFunction(kScInstGame, theObj->EventHandlers[0], 1,
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
            if (stricmp(macroname,"score")==0)
                sprintf(tempo,"%d",play.score);
            else if (stricmp(macroname,"totalscore")==0)
                sprintf(tempo,"%d",MAXSCORE);
            else if (stricmp(macroname,"scoretext")==0)
                sprintf(tempo,"%d of %d",play.score,MAXSCORE);
            else if (stricmp(macroname,"gamename")==0)
                strcpy(tempo, play.game_name);
            else if (stricmp(macroname,"overhotspot")==0) {
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


void export_gui_controls(int ee) {

    for (int ff = 0; ff < guis[ee].ControlCount; ff++) {
        if (!guis[ee].Controls[ff]->Name.IsEmpty())
            ccAddExternalDynamicObject(guis[ee].Controls[ff]->Name, guis[ee].Controls[ff], &ccDynamicGUIObject);

        ccRegisterManagedObject(guis[ee].Controls[ff], &ccDynamicGUIObject);
    }
}

void unexport_gui_controls(int ee) {

    for (int ff = 0; ff < guis[ee].ControlCount; ff++) {
        if (!guis[ee].Controls[ff]->Name.IsEmpty())
            ccRemoveExternalSymbol(guis[ee].Controls[ff]->Name);

        if (!ccUnRegisterManagedObject(guis[ee].Controls[ff]))
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
        // GUIs might have been removed/added
        for (int aa = 0; aa < game.numgui; aa++) {
            guis[aa].OnControlPositionChanged();
        }
        guis_need_update = 1;
        invalidate_screen();
    }
}


int adjust_x_for_guis (int xx, int yy) {
    if ((game.options[OPT_DISABLEOFF]==3) && (all_buttons_disabled > 0))
        return xx;
    // If it's covered by a GUI, move it right a bit
    for (int aa=0;aa < game.numgui; aa++) {
        if (!guis[aa].IsVisible())
            continue;
        if ((guis[aa].X > xx) || (guis[aa].Y > yy) || (guis[aa].Y + guis[aa].Height < yy))
            continue;
        // totally transparent GUI, ignore
        if ((guis[aa].BgColor == 0) && (guis[aa].BgImage < 1))
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
        if (!guis[aa].IsVisible())
            continue;
        if (guis[aa].Y > yy)
            continue;
        // totally transparent GUI, ignore
        if ((guis[aa].BgColor == 0) && (guis[aa].BgImage < 1))
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
  int ifn = tehgui->Id;
  delete guibg[ifn];
  guibg[ifn] = BitmapHelper::CreateBitmap(tehgui->Width, tehgui->Height, System_GetColorDepth());
  if (guibg[ifn] == NULL)
    quit("SetGUISize: internal error: unable to reallocate gui cache");
  guibg[ifn] = ReplaceBitmapWithSupportedFormat(guibg[ifn]);

  if (guibgbmp[ifn] != NULL)
  {
    gfxDriver->DestroyDDB(guibgbmp[ifn]);
    guibgbmp[ifn] = NULL;
  }
}

extern int is_complete_overlay;

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
            if (is_complete_overlay>0) break;  // interfaces disabled
            //    if (play.disabled_user_interface>0) break;
            if (ifacepopped==guin) continue;
            if (guis[guin].IsConcealed()) continue;
            // Don't allow it to be popped up while skipping cutscene
            if (play.fast_forward) continue;

            if (mousey < guis[guin].PopupAtMouseY) {
                set_mouse_cursor(CURS_ARROW);
                guis[guin].SetVisibility(kGUIVisibility_On); guis_need_update = 1;
                ifacepopped=guin; PauseGame();
                break;
            }
        }
    }
    return mouse_over_gui;
}

void gui_on_mouse_hold(const int wasongui, const int wasbutdown)
{
    for (int i=0;i<guis[wasongui].ControlCount;i++) {
        if (!guis[wasongui].Controls[i]->IsActivated) continue;
        if (guis[wasongui].GetControlType(i)!=kGUISlider) continue;
        // GUI Slider repeatedly activates while being dragged
        guis[wasongui].Controls[i]->IsActivated = false;
        force_event(EV_IFACECLICK, wasongui, i, wasbutdown);
        break;
    }
}

void gui_on_mouse_up(const int wasongui, const int wasbutdown)
{
    guis[wasongui].OnMouseButtonUp();

    for (int i=0;i<guis[wasongui].ControlCount;i++) {
        if (!guis[wasongui].Controls[i]->IsActivated) continue;
        guis[wasongui].Controls[i]->IsActivated = false;
        if (!IsInterfaceEnabled()) break;

        int cttype=guis[wasongui].GetControlType(i);
        if ((cttype == kGUIButton) || (cttype == kGUISlider) || (cttype == kGUIListBox)) {
            force_event(EV_IFACECLICK, wasongui, i, wasbutdown);
        }
        else if (cttype == kGUIInvWindow) {
            mouse_ifacebut_xoffs=mousex-(guis[wasongui].Controls[i]->X)-guis[wasongui].X;
            mouse_ifacebut_yoffs=mousey-(guis[wasongui].Controls[i]->Y)-guis[wasongui].Y;
            int iit=offset_over_inv((GUIInvWindow*)guis[wasongui].Controls[i]);
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
    guis[guin].OnMouseButtonDown();
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

RuntimeScriptValue Sc_GUI_Click(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_Click);
}

RuntimeScriptValue Sc_GUI_ProcessClick(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(GUI_ProcessClick);
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
    ccAddExternalObjectFunction("GUI::get_Clickable",           Sc_GUI_GetClickable);
    ccAddExternalObjectFunction("GUI::set_Clickable",           Sc_GUI_SetClickable);
    ccAddExternalObjectFunction("GUI::get_ControlCount",        Sc_GUI_GetControlCount);
    ccAddExternalObjectFunction("GUI::geti_Controls",           Sc_GUI_GetiControls);
    ccAddExternalObjectFunction("GUI::get_Height",              Sc_GUI_GetHeight);
    ccAddExternalObjectFunction("GUI::set_Height",              Sc_GUI_SetHeight);
    ccAddExternalObjectFunction("GUI::get_ID",                  Sc_GUI_GetID);
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
