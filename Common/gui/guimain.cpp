
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "gui/guimain.h"
#include "wgt2allg.h"
#include "ac/ac_common.h"	// quit()
#include "ac/gamesetupstruct.h"
#include "gui/guibutton.h"
#include "gui/guilabel.h"
#include "gui/guislider.h"
#include "gui/guiinv.h"
#include "gui/guitextbox.h"
#include "gui/guilistbox.h"
#include "acfont/ac_fonts.h"
#include "sprcache.h"

extern SpriteCache spriteset;


char GUIMain::oNameBuffer[20];
char lines[MAXLINE][200];
int  numlines;

int guis_need_update = 1;
int all_buttons_disabled = 0, gui_inv_pic = -1;
int gui_disabled_style = 0;


// Temporarily copied this from acruntim.h;
// it is unknown if this should be defined for all solution, or only runtime
#define STD_BUFFER_SIZE 3000



GUIMain::GUIMain()
{
  init();
}

void GUIMain::init()
{
  vtext[0] = 0;
  clickEventHandler[0] = 0;
  focus = 0;
  numobjs = 0;
  mouseover = -1;
  mousewasx = -1;
  mousewasy = -1;
  mousedownon = -1;
  highlightobj = -1;
  on = 1;
  fgcol = 1;
  bgcol = 8;
  flags = 0;
}

void GUIMain::FixupGuiName(char* name)
{
	if ((strlen(name) > 0) && (name[0] != 'g'))
	{
	  char tempbuffer[200];

	  memset(tempbuffer, 0, 200);
	  tempbuffer[0] = 'g';
	  tempbuffer[1] = name[0];
	  strcat(&tempbuffer[2], strlwr(&name[1]));
	  strcpy(name, tempbuffer);
	}
}

void GUIMain::SetTransparencyAsPercentage(int percent)
{
	// convert from % transparent to Opacity from 0-255
	if (percent == 0)
	  this->transparency = 0;
	else if (percent == 100)
	  this->transparency = 255;
	else
	  this->transparency = ((100 - percent) * 25) / 10;
}

void GUIMain::ReadFromFile(FILE *fp, int version)
{
    // read/write everything except drawOrder since
    // it will be regenerated
    fread(vtext, sizeof(char), 40, fp);
    fread(&x, sizeof(int), 27 + 2*MAX_OBJS_ON_GUI, fp);
}

void GUIMain::WriteToFile(FILE *fp)
{
	fwrite(vtext, sizeof(char), 40, fp);
	fwrite(&x, sizeof(int), 27 + 2*MAX_OBJS_ON_GUI, fp);
}

const char* GUIMain::get_objscript_name(const char *basedOn) {
  if (basedOn == NULL)
    basedOn = name;

  if (basedOn[0] == 0) {
    oNameBuffer[0] = 0;
  }
  else {
    if (strlen(basedOn) > 18)
      return "";
    sprintf(oNameBuffer, "g%s", basedOn);
    strlwr(oNameBuffer);
    oNameBuffer[1] = toupper(oNameBuffer[1]);
  }
  return &oNameBuffer[0];
}

int GUIMain::is_textwindow() {
  if (vtext[0] == GUI_TEXTWINDOW)
    return 1;
  return 0;
}

extern "C" int compare_guicontrolzorder(const void *elem1, const void *elem2) {
  GUIObject *e1, *e2;
  e1 = *((GUIObject**)elem1);
  e2 = *((GUIObject**)elem2);

  // returns >0 if e1 is lower down, <0 if higher, =0 if the same
  return e1->zorder - e2->zorder;
}

bool GUIMain::is_alpha() 
{
  if (this->bgpic > 0)
  {
    // alpha state depends on background image
    return is_sprite_alpha(this->bgpic);
  }
  if (this->bgcol > 0)
  {
    // not alpha transparent if there is a background color
    return false;
  }
  // transparent background, enable alpha blending
  return (final_col_dep >= 24);
}

void GUIMain::resort_zorder()
{
  int ff;
  GUIObject *controlArray[MAX_OBJS_ON_GUI];
  
  for (ff = 0; ff < numobjs; ff++)
    controlArray[ff] = objs[ff];
  
  qsort(&controlArray, numobjs, sizeof(GUIObject*), compare_guicontrolzorder);

  for (ff = 0; ff < numobjs; ff++)
    drawOrder[ff] = controlArray[ff]->objn;
}

bool GUIMain::bring_to_front(int objNum) {
  if (objNum < 0)
    return false;

  if (objs[objNum]->zorder < numobjs - 1) {
    int oldOrder = objs[objNum]->zorder;
    for (int ii = 0; ii < numobjs; ii++) {
      if (objs[ii]->zorder > oldOrder)
        objs[ii]->zorder--;
    }
    objs[objNum]->zorder = numobjs - 1;
    resort_zorder();
    control_positions_changed();
    return true;
  }

  return false;
}

bool GUIMain::send_to_back(int objNum) {
  if (objNum < 0)
    return false;

  if (objs[objNum]->zorder > 0) {
    int oldOrder = objs[objNum]->zorder;
    for (int ii = 0; ii < numobjs; ii++) {
      if (objs[ii]->zorder < oldOrder)
        objs[ii]->zorder++;
    }
    objs[objNum]->zorder = 0;
    resort_zorder();
    control_positions_changed();
    return true;
  }

  return false;
}

void GUIMain::rebuild_array()
{
  int ff, thistype, thisnum;

  for (ff = 0; ff < numobjs; ff++) {
    thistype = (objrefptr[ff] >> 16) & 0x000ffff;
    thisnum = objrefptr[ff] & 0x0000ffff;

    if ((thisnum < 0) || (thisnum >= 2000))
      quit("GUIMain: rebuild array failed (invalid object index)");

    if (thistype == GOBJ_BUTTON)
      objs[ff] = &guibuts[thisnum];
    else if (thistype == GOBJ_LABEL)
      objs[ff] = &guilabels[thisnum];
    else if (thistype == GOBJ_INVENTORY)
      objs[ff] = &guiinv[thisnum];
    else if (thistype == GOBJ_SLIDER)
      objs[ff] = &guislider[thisnum];
    else if (thistype == GOBJ_TEXTBOX)
      objs[ff] = &guitext[thisnum];
    else if (thistype == GOBJ_LISTBOX)
      objs[ff] = &guilist[thisnum];
    else
      quit("guimain: unknown control type found on gui");

    objs[ff]->guin = this->guiId;
    objs[ff]->objn = ff;
  }

  resort_zorder();
}

int GUIMain::get_control_type(int indx)
{
  if ((indx < 0) | (indx >= numobjs))
    return -1;
  return ((objrefptr[indx] >> 16) & 0x0000ffff);
}

int GUIMain::is_mouse_on_gui()
{
  if (on < 1)
    return 0;

  if (flags & GUIF_NOCLICK)
    return 0;

  if ((mousex >= x) & (mousey >= y) & (mousex <= x + wid) & (mousey <= y + hit))
    return 1;

  return 0;
}

void GUIMain::draw_blob(int xp, int yp)
{
  wbar(xp, yp, xp + get_fixed_pixel_size(1), yp + get_fixed_pixel_size(1));
}

void GUIMain::draw_at(int xx, int yy)
{
  int aa;

  SET_EIP(375)

  wtexttransparent(TEXTFG);

  if ((wid < 1) || (hit < 1))
    return;

  block abufwas = abuf;
  block subbmp = create_sub_bitmap(abuf, xx, yy, wid, hit);

  SET_EIP(376)
  // stop border being transparent, if the whole GUI isn't
  if ((fgcol == 0) && (bgcol != 0))
    fgcol = 16;

  abuf = subbmp;
  if (bgcol != 0)
    clear_to_color(abuf, get_col8_lookup(bgcol));

  SET_EIP(377)

  if (fgcol != bgcol) {
    rect(abuf, 0, 0, abuf->w - 1, abuf->h - 1, get_col8_lookup(fgcol));
    if (get_fixed_pixel_size(1) > 1)
      rect(abuf, 1, 1, abuf->w - 2, abuf->h - 2, get_col8_lookup(fgcol));
  }

  SET_EIP(378)

  if ((bgpic > 0) && (spriteset[bgpic] != NULL))
    draw_sprite_compensate(bgpic, 0, 0, 0);

  SET_EIP(379)

  for (aa = 0; aa < numobjs; aa++) {
    
    set_eip_guiobj(drawOrder[aa]);

    GUIObject *objToDraw = objs[drawOrder[aa]];

    if ((objToDraw->IsDisabled()) && (gui_disabled_style == GUIDIS_BLACKOUT))
      continue;
    if (!objToDraw->IsVisible())
      continue;

    objToDraw->Draw();

    int selectedColour = 14;

    if (highlightobj == drawOrder[aa]) {
      if (outlineGuiObjects)
        selectedColour = 13;
      wsetcolor(selectedColour);
      draw_blob(objToDraw->x + objToDraw->wid - get_fixed_pixel_size(1) - 1, objToDraw->y);
      draw_blob(objToDraw->x, objToDraw->y + objToDraw->hit - get_fixed_pixel_size(1) - 1);
      draw_blob(objToDraw->x, objToDraw->y);
      draw_blob(objToDraw->x + objToDraw->wid - get_fixed_pixel_size(1) - 1, 
                objToDraw->y + objToDraw->hit - get_fixed_pixel_size(1) - 1);
    }
    if (outlineGuiObjects) {
      int oo;  // draw a dotted outline round all objects
      wsetcolor(selectedColour);
      for (oo = 0; oo < objToDraw->wid; oo+=2) {
        wputpixel(oo + objToDraw->x, objToDraw->y);
        wputpixel(oo + objToDraw->x, objToDraw->y + objToDraw->hit - 1);
      }
      for (oo = 0; oo < objToDraw->hit; oo+=2) {
        wputpixel(objToDraw->x, oo + objToDraw->y);
        wputpixel(objToDraw->x + objToDraw->wid - 1, oo + objToDraw->y);
      }      
    }
  }

  SET_EIP(380)
  destroy_bitmap(abuf);
  abuf = abufwas;
}

void GUIMain::draw()
{
  draw_at(x, y);
}

int GUIMain::find_object_under_mouse(int extrawid, bool mustBeClickable)
{
  int aa;

  for (aa = numobjs - 1; aa >= 0; aa--) {
    int objNum = drawOrder[aa];

    if (!objs[objNum]->IsVisible())
      continue;
    if ((!objs[objNum]->IsClickable()) && (mustBeClickable))
      continue;
    if (objs[objNum]->IsOverControl(mousex, mousey, extrawid))
      return objNum;
  }

  return -1;
}

int GUIMain::find_object_under_mouse()
{
  return find_object_under_mouse(0, true);
}

int GUIMain::find_object_under_mouse(int extrawid)
{
  return find_object_under_mouse(extrawid, true);
}

void GUIMain::control_positions_changed()
{
  // force it to re-check for which control is under the mouse
  mousewasx = -1;
  mousewasy = -1;
}

void GUIMain::poll()
{
  int mxwas = mousex, mywas = mousey;

  mousex -= x;
  mousey -= y;
  if ((mousex != mousewasx) | (mousey != mousewasy)) {
    int newum = find_object_under_mouse();
    
    if (mouseover == MOVER_MOUSEDOWNLOCKED)
      objs[mousedownon]->MouseMove(mousex, mousey);
    else if (newum != mouseover) {
      if (mouseover >= 0)
        objs[mouseover]->MouseLeave();

      if ((newum >= 0) && (objs[newum]->IsDisabled()))
        // the control is disabled - ignore it
        mouseover = -1;
      else if ((newum >= 0) && (!objs[newum]->IsClickable()))
        // the control is not clickable - ignore it
        mouseover = -1;
      else {
        // over a different control
        mouseover = newum;
        if (mouseover >= 0) {
          objs[mouseover]->MouseOver();
          objs[mouseover]->MouseMove(mousex, mousey);
        }
      }
      guis_need_update = 1;
    } 
    else if (mouseover >= 0)
      objs[mouseover]->MouseMove(mousex, mousey);
  }
  mousewasx = mousex;
  mousewasy = mousey;
  mousex = mxwas;
  mousey = mywas;
}

void GUIMain::mouse_but_down()
{
  if (mouseover < 0)
    return;

  // don't activate disabled buttons
  if ((objs[mouseover]->IsDisabled()) || (!objs[mouseover]->IsVisible()) ||
      (!objs[mouseover]->IsClickable()))
    return;

  mousedownon = mouseover;
  if (objs[mouseover]->MouseDown())
    mouseover = MOVER_MOUSEDOWNLOCKED;
  objs[mousedownon]->MouseMove(mousex - x, mousey - y);
  guis_need_update = 1;
}

void GUIMain::mouse_but_up()
{
  // focus was locked - reset it back to normal, but on the
  // locked object so that a MouseLeave gets fired if necessary
  if (mouseover == MOVER_MOUSEDOWNLOCKED) {
    mouseover = mousedownon;
    mousewasx = -1;  // force update
  }

  if (mousedownon < 0)
    return;

  objs[mousedownon]->MouseUp();
  mousedownon = -1;
  guis_need_update = 1;
}



#define GUI_VERSION 115

void read_gui(FILE * iii, GUIMain * guiread, GameSetupStruct * gss, GUIMain** allocate)
{
  int gver, ee;

  if (getw(iii) != (int)GUIMAGIC)
    quit("read_gui: file is corrupt");

  gver = getw(iii);
  if (gver < 100) {
    gss->numgui = gver;
    gver = 0;
  }
  else if (gver > GUI_VERSION)
    quit("read_gui: this game requires a newer version of AGS");
  else
    gss->numgui = getw(iii);

  if ((gss->numgui < 0) || (gss->numgui > 1000))
    quit("read_gui: invalid number of GUIs, file corrupt?");

  if (allocate != NULL)
  {
    *allocate = (GUIMain*)malloc(sizeof(GUIMain) * gss->numgui);
    guiread = *allocate;
  }

  // import the main GUI elements
  for (int iteratorCount = 0; iteratorCount < gss->numgui; ++iteratorCount)
  {
    guiread[iteratorCount].init();
    guiread[iteratorCount].ReadFromFile(iii, gver);
  }

  for (ee = 0; ee < gss->numgui; ee++) {
    if (guiread[ee].hit < 2)
      guiread[ee].hit = 2;

    if (gver < 103)
      sprintf(guiread[ee].name, "GUI%d", ee);
    if (gver < 105)
      guiread[ee].zorder = ee;

    if (loaded_game_file_version <= 32) // Fix names for 2.x: "GUI" -> "gGui"
        guiread->FixupGuiName(guiread[ee].name);

    guiread[ee].guiId = ee;
  }

  // import the buttons
  numguibuts = getw(iii);
  guibuts.SetSizeTo(numguibuts);

  for (ee = 0; ee < numguibuts; ee++)
    guibuts[ee].ReadFromFile(iii, gver);

  // labels
  numguilabels = getw(iii);
  guilabels.SetSizeTo(numguilabels);

  for (ee = 0; ee < numguilabels; ee++)
    guilabels[ee].ReadFromFile(iii, gver);

  // inv controls
  numguiinv = getw(iii);
  guiinv.SetSizeTo(numguiinv);

  for (ee = 0; ee < numguiinv; ee++)
    guiinv[ee].ReadFromFile(iii, gver);

  if (gver >= 100) {
    // sliders
    numguislider = getw(iii);
    guislider.SetSizeTo(numguislider);

    for (ee = 0; ee < numguislider; ee++)
      guislider[ee].ReadFromFile(iii, gver);
  }

  if (gver >= 101) {
    // text boxes
    numguitext = getw(iii);
    guitext.SetSizeTo(numguitext);

    for (ee = 0; ee < numguitext; ee++)
      guitext[ee].ReadFromFile(iii, gver);
  }

  if (gver >= 102) {
    // list boxes
    numguilist = getw(iii);
    guilist.SetSizeTo(numguilist);

    for (ee = 0; ee < numguilist; ee++)
      guilist[ee].ReadFromFile(iii, gver);
  }

  // set up the reverse-lookup array
  for (ee = 0; ee < gss->numgui; ee++) {
    guiread[ee].rebuild_array();

    if (gver < 110)
      guiread[ee].clickEventHandler[0] = 0;

    for (int ff = 0; ff < guiread[ee].numobjs; ff++) {
      guiread[ee].objs[ff]->guin = ee;
      guiread[ee].objs[ff]->objn = ff;

      if (gver < 115)
        guiread[ee].objs[ff]->zorder = ff;
    }

    guiread[ee].resort_zorder();
  }

  guis_need_update = 1;
}

void write_gui(FILE * ooo, GUIMain * guiwrite, GameSetupStruct * gss)
{
  int ee;

  putw(GUIMAGIC, ooo);
  putw(GUI_VERSION, ooo);
  putw(gss->numgui, ooo);

  for (int iteratorCount = 0; iteratorCount < gss->numgui; ++iteratorCount)
  {
    guiwrite[iteratorCount].WriteToFile(ooo);
  }

  putw(numguibuts, ooo);
  for (ee = 0; ee < numguibuts; ee++)
    guibuts[ee].WriteToFile(ooo);

  putw(numguilabels, ooo);
  for (ee = 0; ee < numguilabels; ee++)
    guilabels[ee].WriteToFile(ooo);

  putw(numguiinv, ooo);
  for (ee = 0; ee < numguiinv; ee++)
    guiinv[ee].WriteToFile(ooo);

  putw(numguislider, ooo);
  for (ee = 0; ee < numguislider; ee++)
    guislider[ee].WriteToFile(ooo);

  putw(numguitext, ooo);
  for (ee = 0; ee < numguitext; ee++)
    guitext[ee].WriteToFile(ooo);

  putw(numguilist, ooo);
  for (ee = 0; ee < numguilist; ee++)
    guilist[ee].WriteToFile(ooo);
}
