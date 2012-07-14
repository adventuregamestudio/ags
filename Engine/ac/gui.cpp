
#include "ac/gui.h"
#include "util/wgt2allg.h"
#include "ali3d.h"
#include "ac/common.h"
#include "ac/draw.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/global_game.h"
#include "ac/global_gui.h"
#include "ac/global_screen.h"
#include "ac/interfacebutton.h"
#include "ac/mouse.h"
#include "ac/roomstruct.h"
#include "ac/runtime_defines.h"
#include "ac/dynobj/cc_guiobject.h"
#include "ac/dynobj/scriptgui.h"
#include "script/cc_instance.h"
#include "debug/debug.h"
#include "gfx/gfxfilter.h"
#include "gui/guibutton.h"
#include "gui/guimain.h"
#include "script/script.h"
#include "script/script_runtime.h"


extern GameSetup usetup;
extern roomstruct thisroom;
extern GUIMain*guis;
extern GFXFilter *filter;
extern int cur_mode,cur_cursor;
extern ccInstance *gameinst;
extern ScriptGUI *scrGui;
extern GameSetupStruct game;
extern CCGUIObject ccDynamicGUIObject;
extern int scrnwid,scrnhit;
extern block *guibg;
extern IDriverDependantBitmap **guibgbmp;
extern IGraphicsDriver *gfxDriver;


int ifacepopped=-1;  // currently displayed pop-up GUI (-1 if none)
int mouse_on_iface=-1;   // mouse cursor is over this interface
int mouse_on_iface_button=-1;
int mouse_pushed_iface=-1;  // this BUTTON on interface MOUSE_ON_IFACE is pushed
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
  if (tehgui->gui->on != 0)
    return 1;
  return 0;
}

int GUI_GetX(ScriptGUI *tehgui) {
  return divide_down_coordinate(tehgui->gui->x);
}

void GUI_SetX(ScriptGUI *tehgui, int xx) {
  if (xx >= thisroom.width)
    quit("!GUI.X: co-ordinates specified are out of range.");

  tehgui->gui->x = multiply_up_coordinate(xx);
}

int GUI_GetY(ScriptGUI *tehgui) {
  return divide_down_coordinate(tehgui->gui->y);
}

void GUI_SetY(ScriptGUI *tehgui, int yy) {
  if (yy >= thisroom.height)
    quit("!GUI.Y: co-ordinates specified are out of range.");

  tehgui->gui->y = multiply_up_coordinate(yy);
}

void GUI_SetPosition(ScriptGUI *tehgui, int xx, int yy) {
  GUI_SetX(tehgui, xx);
  GUI_SetY(tehgui, yy);
}

void GUI_SetSize(ScriptGUI *sgui, int widd, int hitt) {
  if ((widd < 1) || (hitt < 1) || (widd > BASEWIDTH) || (hitt > GetMaxScreenHeight()))
    quitprintf("!SetGUISize: invalid dimensions (tried to set to %d x %d)", widd, hitt);

  GUIMain *tehgui = sgui->gui;
  multiply_up_coordinates(&widd, &hitt);

  if ((tehgui->wid == widd) && (tehgui->hit == hitt))
    return;
  
  tehgui->wid = widd;
  tehgui->hit = hitt;
  
  recreate_guibg_image(tehgui);

  guis_need_update = 1;
}

int GUI_GetWidth(ScriptGUI *sgui) {
  return divide_down_coordinate(sgui->gui->wid);
}

int GUI_GetHeight(ScriptGUI *sgui) {
  return divide_down_coordinate(sgui->gui->hit);
}

void GUI_SetWidth(ScriptGUI *sgui, int newwid) {
  GUI_SetSize(sgui, newwid, GUI_GetHeight(sgui));
}

void GUI_SetHeight(ScriptGUI *sgui, int newhit) {
  GUI_SetSize(sgui, GUI_GetWidth(sgui), newhit);
}

void GUI_SetZOrder(ScriptGUI *tehgui, int z) {
  tehgui->gui->zorder = z;
  update_gui_zorder();
}

int GUI_GetZOrder(ScriptGUI *tehgui) {
  return tehgui->gui->zorder;
}

void GUI_SetClickable(ScriptGUI *tehgui, int clickable) {
  tehgui->gui->flags &= ~GUIF_NOCLICK;
  if (clickable == 0)
    tehgui->gui->flags |= GUIF_NOCLICK;
}

int GUI_GetClickable(ScriptGUI *tehgui) {
  if (tehgui->gui->flags & GUIF_NOCLICK)
    return 0;
  return 1;
}

int GUI_GetID(ScriptGUI *tehgui) {
  return tehgui->id;
}

GUIObject* GUI_GetiControls(ScriptGUI *tehgui, int idx) {
  if ((idx < 0) || (idx >= tehgui->gui->numobjs))
    return NULL;
  return tehgui->gui->objs[idx];
}

int GUI_GetControlCount(ScriptGUI *tehgui) {
  return tehgui->gui->numobjs;
}

void GUI_SetTransparency(ScriptGUI *tehgui, int trans) {
  if ((trans < 0) | (trans > 100))
    quit("!SetGUITransparency: transparency value must be between 0 and 100");

  tehgui->gui->SetTransparencyAsPercentage(trans);
}

int GUI_GetTransparency(ScriptGUI *tehgui) {
  if (tehgui->gui->transparency == 0)
    return 0;
  if (tehgui->gui->transparency == 255)
    return 100;

  return 100 - ((tehgui->gui->transparency * 10) / 25);
}

void GUI_Centre(ScriptGUI *sgui) {
  GUIMain *tehgui = sgui->gui;
  tehgui->x = scrnwid / 2 - tehgui->wid / 2;
  tehgui->y = scrnhit / 2 - tehgui->hit / 2;
}

void GUI_SetBackgroundGraphic(ScriptGUI *tehgui, int slotn) {
  if (tehgui->gui->bgpic != slotn) {
    tehgui->gui->bgpic = slotn;
    guis_need_update = 1;
  }
}

int GUI_GetBackgroundGraphic(ScriptGUI *tehgui) {
  if (tehgui->gui->bgpic < 1)
    return 0;
  return tehgui->gui->bgpic;
}

ScriptGUI *GetGUIAtLocation(int xx, int yy) {
    int guiid = GetGUIAt(xx, yy);
    if (guiid < 0)
        return NULL;
    return &scrGui[guiid];
}

//=============================================================================

void remove_popup_interface(int ifacenum) {
    if (ifacepopped != ifacenum) return;
    ifacepopped=-1; UnPauseGame();
    guis[ifacenum].on=0;
    if (mousey<=guis[ifacenum].popupyp)
        filter->SetMousePosition(mousex, guis[ifacenum].popupyp+2);
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
        run_text_script_2iparam(gameinst, guis[ifce].clickEventHandler, (int)&scrGui[ifce], mbut);
        return;
    }

    int btype=(guis[ifce].objrefptr[btn] >> 16) & 0x000ffff;
    int rtype=0,rdata;
    if (btype==GOBJ_BUTTON) {
        GUIButton*gbuto=(GUIButton*)guis[ifce].objs[btn];
        rtype=gbuto->leftclick;
        rdata=gbuto->lclickdata;
    }
    else if ((btype==GOBJ_SLIDER) || (btype == GOBJ_TEXTBOX) || (btype == GOBJ_LISTBOX))
        rtype = IBACT_SCRIPT;
    else quit("unknown GUI object triggered process_interface");

    if (rtype==0) ;
    else if (rtype==IBACT_SETMODE)
        set_cursor_mode(rdata);
    else if (rtype==IBACT_SCRIPT) {
        GUIObject *theObj = guis[ifce].objs[btn];
        // if the object has a special handler script then run it;
        // otherwise, run interface_click
        if ((theObj->GetNumEvents() > 0) &&
            (theObj->eventHandlers[0][0] != 0) &&
            (ccGetSymbolAddr(gameinst, theObj->eventHandlers[0]) != NULL)) {
                // control-specific event handler
                if (strchr(theObj->GetEventArgs(0), ',') != NULL)
                    run_text_script_2iparam(gameinst, theObj->eventHandlers[0], (int)theObj, mbut);
                else
                    run_text_script_iparam(gameinst, theObj->eventHandlers[0], (int)theObj);
        }
        else
            run_text_script_2iparam(gameinst,"interface_click",ifce,btn);
    }
}


void replace_macro_tokens(char*statusbarformat,char*cur_stb_text) {
    char*curptr=&statusbarformat[0];
    char tmpm[3];
    char*endat = curptr + strlen(statusbarformat);
    cur_stb_text[0]=0;
    char tempo[STD_BUFFER_SIZE];

    while (1) {
        if (curptr[0]==0) break;
        if (curptr>=endat) break;
        if (curptr[0]=='@') {
            char *curptrWasAt = curptr;
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
                    GetLocationName(divide_down_coordinate(mousex), divide_down_coordinate(mousey), tempo);
            }
            else { // not a macro, there's just a @ in the message
                curptr = curptrWasAt + 1;
                strcpy(tempo, "@");
            }

            strcat(cur_stb_text,tempo);
        }
        else {
            tmpm[0]=curptr[0]; tmpm[1]=0;
            strcat(cur_stb_text,tmpm);
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
            if (guis[a].zorder < guis[play.gui_draw_order[b]].zorder) {
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

    for (int ff = 0; ff < guis[ee].numobjs; ff++) {
        if (guis[ee].objs[ff]->scriptName[0] != 0)
            ccAddExternalSymbol(guis[ee].objs[ff]->scriptName, guis[ee].objs[ff]);

        ccRegisterManagedObject(guis[ee].objs[ff], &ccDynamicGUIObject);
    }
}

void unexport_gui_controls(int ee) {

    for (int ff = 0; ff < guis[ee].numobjs; ff++) {
        if (guis[ee].objs[ff]->scriptName[0] != 0)
            ccRemoveExternalSymbol(guis[ee].objs[ff]->scriptName);

        if (!ccUnRegisterManagedObject(guis[ee].objs[ff]))
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
            guis[aa].control_positions_changed();
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
        if (guis[aa].on < 1)
            continue;
        if ((guis[aa].x > xx) || (guis[aa].y > yy) || (guis[aa].y + guis[aa].hit < yy))
            continue;
        // totally transparent GUI, ignore
        if ((guis[aa].bgcol == 0) && (guis[aa].bgpic < 1))
            continue;

        // try to deal with full-width GUIs across the top
        if (guis[aa].x + guis[aa].wid >= get_fixed_pixel_size(280))
            continue;

        if (xx < guis[aa].x + guis[aa].wid) 
            xx = guis[aa].x + guis[aa].wid + 2;        
    }
    return xx;
}

int adjust_y_for_guis ( int yy) {
    if ((game.options[OPT_DISABLEOFF]==3) && (all_buttons_disabled > 0))
        return yy;
    // If it's covered by a GUI, move it down a bit
    for (int aa=0;aa < game.numgui; aa++) {
        if (guis[aa].on < 1)
            continue;
        if (guis[aa].y > yy)
            continue;
        // totally transparent GUI, ignore
        if ((guis[aa].bgcol == 0) && (guis[aa].bgpic < 1))
            continue;

        // try to deal with full-height GUIs down the left or right
        if (guis[aa].hit > get_fixed_pixel_size(50))
            continue;

        if (yy < guis[aa].y + guis[aa].hit) 
            yy = guis[aa].y + guis[aa].hit + 2;        
    }
    return yy;
}

void recreate_guibg_image(GUIMain *tehgui)
{
  int ifn = tehgui->guiId;
  destroy_bitmap(guibg[ifn]);
  guibg[ifn] = create_bitmap_ex (final_col_dep, tehgui->wid, tehgui->hit);
  if (guibg[ifn] == NULL)
    quit("SetGUISize: internal error: unable to reallocate gui cache");
  guibg[ifn] = gfxDriver->ConvertBitmapToSupportedColourDepth(guibg[ifn]);

  if (guibgbmp[ifn] != NULL)
  {
    gfxDriver->DestroyDDB(guibgbmp[ifn]);
    guibgbmp[ifn] = NULL;
  }
}

