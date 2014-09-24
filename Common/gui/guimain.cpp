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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "util/string_utils.h" //strlwr()
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
#include "debug/out.h"
#include "util/math.h"

using namespace AGS::Common;

char GUIMain::oNameBuffer[20];

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

void GUIMain::ReadFromFile(Stream *in, GuiVersion gui_version)
{
  // read/write everything except drawOrder since
  // it will be regenerated
  in->Read(vtext, 40);
  in->ReadArrayOfInt32(&x, 27);

  // array of 32-bit pointers; these values are unused
  in->Seek(MAX_OBJS_ON_GUI * sizeof(int32_t));

  in->ReadArrayOfInt32(objrefptr, MAX_OBJS_ON_GUI);
}

void GUIMain::WriteToFile(Stream *out)
{
  out->Write(vtext, 40);
  out->WriteArrayOfInt32(&x, 27);

  // array of dummy 32-bit pointers
  int32_t dummy_arr[MAX_OBJS_ON_GUI];
  out->WriteArrayOfInt32(dummy_arr, MAX_OBJS_ON_GUI);

  out->WriteArrayOfInt32((int32_t*)&objrefptr, MAX_OBJS_ON_GUI);
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

bool GUIMain::bring_to_front(int objNum)
{
    return set_control_zorder(objNum, numobjs - 1);
}

bool GUIMain::send_to_back(int objNum)
{
    return set_control_zorder(objNum, 0);
}

bool GUIMain::set_control_zorder(int objNum, int zorder)
{
    if (objNum < 0 || objNum >= numobjs)
        return false; // no such control

    Math::Clamp(0, numobjs - 1, zorder);
    const int old_zorder = objs[objNum]->zorder;
    if (old_zorder == zorder)
        return false; // no change

    const bool move_back = zorder < old_zorder; // back is at zero index
    const int  left      = move_back ? zorder : old_zorder;
    const int  right     = move_back ? old_zorder : zorder;
    for (int i = 0; i < numobjs; ++i)
    {
        const int i_zorder = objs[i]->zorder;
        if (i_zorder == old_zorder)
            objs[i]->zorder = zorder; // the control we are moving
        else if (i_zorder >= left && i_zorder <= right)
        {
            // controls in between old and new positions shift towards free place
            if (move_back)
                objs[i]->zorder++; // move to front
            else
                objs[i]->zorder--; // move to back
        }
    }
    resort_zorder();
    control_positions_changed();
    return true;
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

void GUIMain::draw_blob(Common::Bitmap *ds, int xp, int yp, color_t draw_color)
{
  ds->FillRect(Rect(xp, yp, xp + get_fixed_pixel_size(1), yp + get_fixed_pixel_size(1)), draw_color);
}

void GUIMain::draw_at(Common::Bitmap *ds, int xx, int yy)
{
  int aa;

  SET_EIP(375)

  if ((wid < 1) || (hit < 1))
    return;

  //Bitmap *abufwas = g;
  Bitmap *subbmp = BitmapHelper::CreateSubBitmap(ds, RectWH(xx, yy, wid, hit));

  SET_EIP(376)
  // stop border being transparent, if the whole GUI isn't
  if ((fgcol == 0) && (bgcol != 0))
    fgcol = 16;

  //g = subbmp;
  if (bgcol != 0)
    subbmp->Fill(subbmp->GetCompatibleColor(bgcol));

  SET_EIP(377)

  color_t draw_color;
  if (fgcol != bgcol) {
    draw_color = subbmp->GetCompatibleColor(fgcol);
    subbmp->DrawRect(Rect(0, 0, subbmp->GetWidth() - 1, subbmp->GetHeight() - 1), draw_color);
    if (get_fixed_pixel_size(1) > 1)
      subbmp->DrawRect(Rect(1, 1, subbmp->GetWidth() - 2, subbmp->GetHeight() - 2), draw_color);
  }

  SET_EIP(378)

  if ((bgpic > 0) && (spriteset[bgpic] != NULL))
    draw_gui_sprite(subbmp, bgpic, 0, 0, false);

  SET_EIP(379)

  for (aa = 0; aa < numobjs; aa++) {
    
    set_eip_guiobj(drawOrder[aa]);

    GUIObject *objToDraw = objs[drawOrder[aa]];

    if ((objToDraw->IsDisabled()) && (gui_disabled_style == GUIDIS_BLACKOUT))
      continue;
    if (!objToDraw->IsVisible())
      continue;

    objToDraw->Draw(subbmp);

    int selectedColour = 14;

    if (highlightobj == drawOrder[aa]) {
      if (outlineGuiObjects)
        selectedColour = 13;
      draw_color = subbmp->GetCompatibleColor(selectedColour);
      draw_blob(subbmp, objToDraw->x + objToDraw->wid - get_fixed_pixel_size(1) - 1, objToDraw->y, draw_color);
      draw_blob(subbmp, objToDraw->x, objToDraw->y + objToDraw->hit - get_fixed_pixel_size(1) - 1, draw_color);
      draw_blob(subbmp, objToDraw->x, objToDraw->y, draw_color);
      draw_blob(subbmp, objToDraw->x + objToDraw->wid - get_fixed_pixel_size(1) - 1, 
                objToDraw->y + objToDraw->hit - get_fixed_pixel_size(1) - 1, draw_color);
    }
    if (outlineGuiObjects) {
      int oo;  // draw a dotted outline round all objects
      draw_color = subbmp->GetCompatibleColor(selectedColour);
      for (oo = 0; oo < objToDraw->wid; oo+=2) {
        subbmp->PutPixel(oo + objToDraw->x, objToDraw->y, draw_color);
        subbmp->PutPixel(oo + objToDraw->x, objToDraw->y + objToDraw->hit - 1, draw_color);
      }
      for (oo = 0; oo < objToDraw->hit; oo+=2) {
        subbmp->PutPixel(objToDraw->x, oo + objToDraw->y, draw_color);
        subbmp->PutPixel(objToDraw->x + objToDraw->wid - 1, oo + objToDraw->y, draw_color);
      }      
    }
  }

  SET_EIP(380)
  delete subbmp;
//  sub_graphics.GetBitmap() = abufwas;
}

void GUIMain::draw(Common::Bitmap *ds)
{
  draw_at(ds, x, y);
}

int GUIMain::find_object_under_mouse(int extrawid, bool mustBeClickable)
{
  int aa;

  if (loaded_game_file_version <= kGameVersion_262)
  {
    // Ignore draw order on 2.6.2 and lower
    for (aa = 0; aa < numobjs; aa++) {
      int objNum = aa;

      if (!objs[objNum]->IsVisible())
        continue;
      if ((!objs[objNum]->IsClickable()) && (mustBeClickable))
        continue;
      if (objs[objNum]->IsOverControl(mousex, mousey, extrawid))
        return objNum;
    }
  }
  else
  {
    for (aa = numobjs - 1; aa >= 0; aa--) {
      int objNum = drawOrder[aa];

      if (!objs[objNum]->IsVisible())
        continue;
      if ((!objs[objNum]->IsClickable()) && (mustBeClickable))
        continue;
      if (objs[objNum]->IsOverControl(mousex, mousey, extrawid))
        return objNum;
    }
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

GuiVersion GameGuiVersion = kGuiVersion_Initial;
void read_gui(Stream *in, GUIMain * guiread, GameSetupStruct * gss, GUIMain** allocate)
{
  int ee;

  if (in->ReadInt32() != (int)GUIMAGIC)
    quit("read_gui: file is corrupt");

  GameGuiVersion = (GuiVersion)in->ReadInt32();
  Out::FPrint("Game GUI version: %d", GameGuiVersion);
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

  if (allocate != NULL)
  {
    *allocate = (GUIMain*)malloc(sizeof(GUIMain) * gss->numgui);
    guiread = *allocate;
  }

  // import the main GUI elements
  for (int iteratorCount = 0; iteratorCount < gss->numgui; ++iteratorCount)
  {
    guiread[iteratorCount].init();
    guiread[iteratorCount].ReadFromFile(in, GameGuiVersion);
  }

  for (ee = 0; ee < gss->numgui; ee++) {
    if (guiread[ee].hit < 2)
      guiread[ee].hit = 2;

    if (GameGuiVersion < kGuiVersion_unkn_103)
      sprintf(guiread[ee].name, "GUI%d", ee);
    if (GameGuiVersion < kGuiVersion_260)
      guiread[ee].zorder = ee;
    if (GameGuiVersion < kGuiVersion_331)
      guiread[ee].padding = TEXTWINDOW_PADDING_DEFAULT;

    if (loaded_game_file_version <= kGameVersion_272) // Fix names for 2.x: "GUI" -> "gGui"
        guiread->FixupGuiName(guiread[ee].name);

    guiread[ee].guiId = ee;
  }

  // import the buttons
  numguibuts = in->ReadInt32();
  guibuts.SetSizeTo(numguibuts);

  for (ee = 0; ee < numguibuts; ee++)
    guibuts[ee].ReadFromFile(in, GameGuiVersion);

  // labels
  numguilabels = in->ReadInt32();
  guilabels.SetSizeTo(numguilabels);

  for (ee = 0; ee < numguilabels; ee++)
    guilabels[ee].ReadFromFile(in, GameGuiVersion);

  // inv controls
  numguiinv = in->ReadInt32();
  guiinv.SetSizeTo(numguiinv);

  for (ee = 0; ee < numguiinv; ee++)
    guiinv[ee].ReadFromFile(in, GameGuiVersion);

  if (GameGuiVersion >= kGuiVersion_214) {
    // sliders
    numguislider = in->ReadInt32();
    guislider.SetSizeTo(numguislider);

    for (ee = 0; ee < numguislider; ee++)
      guislider[ee].ReadFromFile(in, GameGuiVersion);
  }

  if (GameGuiVersion >= kGuiVersion_222) {
    // text boxes
    numguitext = in->ReadInt32();
    guitext.SetSizeTo(numguitext);

    for (ee = 0; ee < numguitext; ee++)
      guitext[ee].ReadFromFile(in, GameGuiVersion);
  }

  if (GameGuiVersion >= kGuiVersion_230) {
    // list boxes
    numguilist = in->ReadInt32();
    guilist.SetSizeTo(numguilist);

    for (ee = 0; ee < numguilist; ee++)
      guilist[ee].ReadFromFile(in, GameGuiVersion);
  }

  // set up the reverse-lookup array
  for (ee = 0; ee < gss->numgui; ee++) {
    guiread[ee].rebuild_array();

    if (GameGuiVersion < kGuiVersion_270)
      guiread[ee].clickEventHandler[0] = 0;

    for (int ff = 0; ff < guiread[ee].numobjs; ff++) {
      guiread[ee].objs[ff]->guin = ee;
      guiread[ee].objs[ff]->objn = ff;

      if (GameGuiVersion < kGuiVersion_272e)
        guiread[ee].objs[ff]->zorder = ff;
    }

    guiread[ee].resort_zorder();
  }

  guis_need_update = 1;
}

void write_gui(Stream *out, GUIMain * guiwrite, GameSetupStruct * gss, bool savedgame)
{
  int ee;

  out->WriteInt32(GUIMAGIC);

  if (savedgame)
  {
    out->WriteInt32(GameGuiVersion > kGuiVersion_ForwardCompatible ? GameGuiVersion : kGuiVersion_ForwardCompatible);
  }
  else
  {
    out->WriteInt32(kGuiVersion_Current);
  }
  
  out->WriteInt32(gss->numgui);

  for (int iteratorCount = 0; iteratorCount < gss->numgui; ++iteratorCount)
  {
    guiwrite[iteratorCount].WriteToFile(out);
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
