
#include <stdio.h>
#include "acgui/ac_guibutton.h"
#include "acgui/ac_guimain.h"
#include "wgt2allg.h"
#include "sprcache.h"

extern SpriteCache spriteset;



DynamicArray<GUIButton> guibuts;
//GUIButton guibuts[MAX_OBJ_EACH_TYPE];
int numguibuts = 0;

void GUIButton::WriteToFile(FILE * ooo)
{
  GUIObject::WriteToFile(ooo);
  // MACPORT FIXES: swap
  fwrite(&pic, sizeof(int), 12, ooo);
  fwrite(&text[0], sizeof(char), 50, ooo);
  putw(textAlignment, ooo);
  putw(reserved1, ooo);
}

void GUIButton::ReadFromFile(FILE * ooo, int version)
{
  GUIObject::ReadFromFile(ooo, version);
  // MACPORT FIXES: swap
  fread(&pic, sizeof(int), 12, ooo);
  fread(&text[0], sizeof(char), 50, ooo);
  if (textcol == 0)
    textcol = 16;
  usepic = pic;

  if (version >= 111) {
    textAlignment = getw(ooo);
    reserved1 = getw(ooo);
  }
  else {
    textAlignment = GBUT_ALIGN_TOPMIDDLE;
    reserved1 = 0;
  }
}

void GUIButton::Draw()
{
  int drawDisabled = IsDisabled();

  check_font(&font);
  // if it's "Unchanged when disabled" or "GUI Off", don't grey out
  if ((gui_disabled_style == GUIDIS_UNCHANGED) ||
      (gui_disabled_style == GUIDIS_GUIOFF))
    drawDisabled = 0;

  if (usepic <= 0)
    usepic = pic;

  if (drawDisabled)
    usepic = pic;

  if ((drawDisabled) && (gui_disabled_style == GUIDIS_BLACKOUT))
    // buttons off when disabled - no point carrying on
    return;

  // draw it!!
  if ((usepic > 0) && (pic > 0)) {

    if (flags & GUIF_CLIP)
      set_clip_rect(abuf, x, y, x + wid - 1, y + hit - 1);

    if (spriteset[usepic] != NULL)
      draw_sprite_compensate(usepic, x, y, 1);

    if (gui_inv_pic >= 0) {
      int drawInv = 0;

      // Stretch to fit button
      if (stricmp(text, "(INV)") == 0)
        drawInv = 1;
      // Draw at actual size
      if (stricmp(text, "(INVNS)") == 0)
        drawInv = 2;

      // Stretch if too big, actual size if not
      if (stricmp(text, "(INVSHR)") == 0) {
        if ((get_adjusted_spritewidth(gui_inv_pic) > wid - 6) ||
            (get_adjusted_spriteheight(gui_inv_pic) > hit - 6))
          drawInv = 1;
        else
          drawInv = 2;
      }

      if (drawInv == 1)
        stretch_sprite(abuf, spriteset[gui_inv_pic], x + 3, y + 3, wid - 6, hit - 6);
      else if (drawInv == 2)
        draw_sprite_compensate(gui_inv_pic,
                               x + wid / 2 - get_adjusted_spritewidth(gui_inv_pic) / 2,
                               y + hit / 2 - get_adjusted_spriteheight(gui_inv_pic) / 2, 1);
    }

    if ((drawDisabled) && (gui_disabled_style == GUIDIS_GREYOUT)) {
      int col8 = get_col8_lookup(8);
      int jj, kk;             // darken the button when disabled
      for (jj = 0; jj < spriteset[usepic]->w; jj++) {
        for (kk = jj % 2; kk < spriteset[usepic]->h; kk += 2)
          putpixel(abuf, x + jj, y + kk, col8);
      }
    }

    set_clip(abuf, 0, 0, abuf->w - 1, abuf->h - 1);
  } 
  else if (text[0] != 0) {
    // it's a text button

    wsetcolor(7);
    wbar(x, y, x + wid - 1, y + hit - 1);
    if (flags & GUIF_DEFAULT) {
      wsetcolor(16);
      wrectangle(x - 1, y - 1, x + wid, y + hit);
    }

    if ((isover) && (ispushed))
      wsetcolor(15);
    else
      wsetcolor(8);

    if (drawDisabled)
      wsetcolor(8);

    wline(x, y + hit - 1, x + wid - 1, y + hit - 1);
    wline(x + wid - 1, y, x + wid - 1, y + hit - 1);
    if ((isover) && (ispushed))
      wsetcolor(8);
    else
      wsetcolor(15);

    if (drawDisabled)
      wsetcolor(8);

    wline(x, y, x + wid - 1, y);
    wline(x, y, x, y + hit - 1);
  }                           // end if text

  // Don't print text of (INV) (INVSHR) (INVNS)
  if ((text[0] == '(') && (text[1] == 'I') && (text[2] == 'N')) ; 
  // Don't print the text if there's a graphic and it hasn't been named
  else if ((usepic > 0) && (pic > 0) && (strcmp(text, "New Button") == 0)) ;
  // if there is some text, print it
  else if (text[0] != 0) {
    int usex = x, usey = y;

    char oritext[200]; // text[] can be not longer than 50 characters due declaration
    Draw_set_oritext(oritext, text);

    if ((ispushed) && (isover)) {
      // move the text a bit while pushed
      usex++;
      usey++;
    }

    switch (textAlignment) {
    case GBUT_ALIGN_TOPMIDDLE:
      usex += (wid / 2 - wgettextwidth(oritext, font) / 2);
      usey += 2;
      break;
    case GBUT_ALIGN_TOPLEFT:
      usex += 2;
      usey += 2;
      break;
    case GBUT_ALIGN_TOPRIGHT:
      usex += (wid - wgettextwidth(oritext, font)) - 2;
      usey += 2;
      break;
    case GBUT_ALIGN_MIDDLELEFT:
      usex += 2;
      usey += (hit / 2 - (wgettextheight(oritext, font) + 1) / 2);
      break;
    case GBUT_ALIGN_CENTRED:
      usex += (wid / 2 - wgettextwidth(oritext, font) / 2);
      usey += (hit / 2 - (wgettextheight(oritext, font) + 1) / 2);
      break;
    case GBUT_ALIGN_MIDDLERIGHT:
      usex += (wid - wgettextwidth(oritext, font)) - 2;
      usey += (hit / 2 - (wgettextheight(oritext, font) + 1) / 2);
      break;
    case GBUT_ALIGN_BOTTOMLEFT:
      usex += 2;
      usey += (hit - wgettextheight(oritext, font)) - 2;
      break;
    case GBUT_ALIGN_BOTTOMMIDDLE:
      usex += (wid / 2 - wgettextwidth(oritext, font) / 2);
      usey += (hit - wgettextheight(oritext, font)) - 2;
      break;
    case GBUT_ALIGN_BOTTOMRIGHT:
      usex += (wid - wgettextwidth(oritext, font)) - 2;
      usey += (hit - wgettextheight(oritext, font)) - 2;
      break;
    }

    wtextcolor(textcol);
    if (drawDisabled)
      wtextcolor(8);

    WOUTTEXT_REVERSE(usex, usey, font, oritext);
  }
  
}

void GUIButton::MouseUp()
{
  if (isover) {
    usepic = overpic;
    if ((!IsDisabled()) && (IsClickable()))
      activated++;
  }
  else
    usepic = pic;

  ispushed = 0;
}


//=============================================================================


#include <stdio.h>
#include "wgt2allg.h"
#include "acmain/ac_maindefines.h"
#include "acmain/ac_commonheaders.h"
#include "acmain/ac_animatingguibutton.h"
#include "acgui/ac_guibutton.h"

// ** start animating buttons code

// returns 1 if animation finished
int UpdateAnimatingButton(int bu) {
    if (animbuts[bu].wait > 0) {
        animbuts[bu].wait--;
        return 0;
    }
    ViewStruct *tview = &views[animbuts[bu].view];

    animbuts[bu].frame++;

    if (animbuts[bu].frame >= tview->loops[animbuts[bu].loop].numFrames) 
    {
        if (tview->loops[animbuts[bu].loop].RunNextLoop()) {
            // go to next loop
            animbuts[bu].loop++;
            animbuts[bu].frame = 0;
        }
        else if (animbuts[bu].repeat) {
            animbuts[bu].frame = 0;
            // multi-loop anim, go back
            while ((animbuts[bu].loop > 0) && 
                (tview->loops[animbuts[bu].loop - 1].RunNextLoop()))
                animbuts[bu].loop--;
        }
        else
            return 1;
    }

    CheckViewFrame(animbuts[bu].view, animbuts[bu].loop, animbuts[bu].frame);

    // update the button's image
    guibuts[animbuts[bu].buttonid].pic = tview->loops[animbuts[bu].loop].frames[animbuts[bu].frame].pic;
    guibuts[animbuts[bu].buttonid].usepic = guibuts[animbuts[bu].buttonid].pic;
    guibuts[animbuts[bu].buttonid].pushedpic = 0;
    guibuts[animbuts[bu].buttonid].overpic = 0;
    guis_need_update = 1;

    animbuts[bu].wait = animbuts[bu].speed + tview->loops[animbuts[bu].loop].frames[animbuts[bu].frame].speed;
    return 0;
}

void StopButtonAnimation(int idxn) {
    numAnimButs--;
    for (int aa = idxn; aa < numAnimButs; aa++) {
        animbuts[aa] = animbuts[aa + 1];
    }
}

void FindAndRemoveButtonAnimation(int guin, int objn) {

    for (int ii = 0; ii < numAnimButs; ii++) {
        if ((animbuts[ii].ongui == guin) && (animbuts[ii].onguibut == objn)) {
            StopButtonAnimation(ii);
            ii--;
        }

    }
}
// ** end animating buttons code

// *** BUTTON FUNCTIONS


void Button_Animate(GUIButton *butt, int view, int loop, int speed, int repeat) {
    int guin = butt->guin;
    int objn = butt->objn;

    if ((view < 1) || (view > game.numviews))
        quit("!AnimateButton: invalid view specified");
    view--;

    if ((loop < 0) || (loop >= views[view].numLoops))
        quit("!AnimateButton: invalid loop specified for view");

    // if it's already animating, stop it
    FindAndRemoveButtonAnimation(guin, objn);

    if (numAnimButs >= MAX_ANIMATING_BUTTONS)
        quit("!AnimateButton: too many animating GUI buttons at once");

    int buttonId = guis[guin].objrefptr[objn] & 0x000ffff;

    guibuts[buttonId].pushedpic = 0;
    guibuts[buttonId].overpic = 0;

    animbuts[numAnimButs].ongui = guin;
    animbuts[numAnimButs].onguibut = objn;
    animbuts[numAnimButs].buttonid = buttonId;
    animbuts[numAnimButs].view = view;
    animbuts[numAnimButs].loop = loop;
    animbuts[numAnimButs].speed = speed;
    animbuts[numAnimButs].repeat = repeat;
    animbuts[numAnimButs].frame = -1;
    animbuts[numAnimButs].wait = 0;
    numAnimButs++;
    // launch into the first frame
    if (UpdateAnimatingButton(numAnimButs - 1))
        quit("!AnimateButton: no frames to animate");
}

const char* Button_GetText_New(GUIButton *butt) {
    return CreateNewScriptString(butt->text);
}

void Button_GetText(GUIButton *butt, char *buffer) {
    strcpy(buffer, butt->text);
}

void Button_SetText(GUIButton *butt, const char *newtx) {
    newtx = get_translation(newtx);
    if (strlen(newtx) > 49) quit("!SetButtonText: text too long, button has 50 chars max");

    if (strcmp(butt->text, newtx)) {
        guis_need_update = 1;
        strcpy(butt->text,newtx);
    }
}

void Button_SetFont(GUIButton *butt, int newFont) {
    if ((newFont < 0) || (newFont >= game.numfonts))
        quit("!Button.Font: invalid font number.");

    if (butt->font != newFont) {
        butt->font = newFont;
        guis_need_update = 1;
    }
}

int Button_GetFont(GUIButton *butt) {
    return butt->font;
}

int Button_GetClipImage(GUIButton *butt) {
    if (butt->flags & GUIF_CLIP)
        return 1;
    return 0;
}

void Button_SetClipImage(GUIButton *butt, int newval) {
    butt->flags &= ~GUIF_CLIP;
    if (newval)
        butt->flags |= GUIF_CLIP;

    guis_need_update = 1;
}

int Button_GetGraphic(GUIButton *butt) {
    // return currently displayed pic
    if (butt->usepic < 0)
        return butt->pic;
    return butt->usepic;
}

int Button_GetMouseOverGraphic(GUIButton *butt) {
    return butt->overpic;
}

void Button_SetMouseOverGraphic(GUIButton *guil, int slotn) {
    DEBUG_CONSOLE("GUI %d Button %d mouseover set to slot %d", guil->guin, guil->objn, slotn);

    if ((guil->isover != 0) && (guil->ispushed == 0))
        guil->usepic = slotn;
    guil->overpic = slotn;

    guis_need_update = 1;
    FindAndRemoveButtonAnimation(guil->guin, guil->objn);
}

int Button_GetNormalGraphic(GUIButton *butt) {
    return butt->pic;
}

void Button_SetNormalGraphic(GUIButton *guil, int slotn) {
    DEBUG_CONSOLE("GUI %d Button %d normal set to slot %d", guil->guin, guil->objn, slotn);
    // normal pic - update if mouse is not over, or if there's no overpic
    if (((guil->isover == 0) || (guil->overpic < 1)) && (guil->ispushed == 0))
        guil->usepic = slotn;
    guil->pic = slotn;
    // update the clickable area to the same size as the graphic
    guil->wid = spritewidth[slotn];
    guil->hit = spriteheight[slotn];

    guis_need_update = 1;
    FindAndRemoveButtonAnimation(guil->guin, guil->objn);
}

int Button_GetPushedGraphic(GUIButton *butt) {
    return butt->pushedpic;
}

void Button_SetPushedGraphic(GUIButton *guil, int slotn) {
    DEBUG_CONSOLE("GUI %d Button %d pushed set to slot %d", guil->guin, guil->objn, slotn);

    if (guil->ispushed)
        guil->usepic = slotn;
    guil->pushedpic = slotn;

    guis_need_update = 1;
    FindAndRemoveButtonAnimation(guil->guin, guil->objn);
}

int Button_GetTextColor(GUIButton *butt) {
    return butt->textcol;
}

void Button_SetTextColor(GUIButton *butt, int newcol) {
    if (butt->textcol != newcol) {
        butt->textcol = newcol;
        guis_need_update = 1;
    }
}

void SetButtonText(int guin,int objn,char*newtx) {
    VALIDATE_STRING(newtx);
    if ((guin<0) | (guin>=game.numgui))
        quit("!SetButtonText: invalid GUI number");
    if ((objn<0) | (objn>=guis[guin].numobjs))
        quit("!SetButtonText: invalid object number");
    if (guis[guin].get_control_type(objn)!=GOBJ_BUTTON)
        quit("!SetButtonText: specified control is not a button");

    GUIButton*guil=(GUIButton*)guis[guin].objs[objn];
    Button_SetText(guil, newtx);
}


void AnimateButton(int guin, int objn, int view, int loop, int speed, int repeat) {
    if ((guin<0) | (guin>=game.numgui)) quit("!AnimateButton: invalid GUI number");
    if ((objn<0) | (objn>=guis[guin].numobjs)) quit("!AnimateButton: invalid object number");
    if (guis[guin].get_control_type(objn)!=GOBJ_BUTTON)
        quit("!AnimateButton: specified control is not a button");

    Button_Animate((GUIButton*)guis[guin].objs[objn], view, loop, speed, repeat);
}


int GetButtonPic(int guin, int objn, int ptype) {
    if ((guin<0) | (guin>=game.numgui)) quit("!GetButtonPic: invalid GUI number");
    if ((objn<0) | (objn>=guis[guin].numobjs)) quit("!GetButtonPic: invalid object number");
    if (guis[guin].get_control_type(objn)!=GOBJ_BUTTON)
        quit("!GetButtonPic: specified control is not a button");
    if ((ptype < 0) | (ptype > 3)) quit("!GetButtonPic: invalid pic type");

    GUIButton*guil=(GUIButton*)guis[guin].objs[objn];

    if (ptype == 0) {
        // currently displayed pic
        if (guil->usepic < 0)
            return guil->pic;
        return guil->usepic;
    }
    else if (ptype==1) {
        // nomal pic
        return guil->pic;
    }
    else if (ptype==2) {
        // mouseover pic
        return guil->overpic;
    }
    else { // pushed pic
        return guil->pushedpic;
    }

    quit("internal error in getbuttonpic");
}

void SetButtonPic(int guin,int objn,int ptype,int slotn) {
    if ((guin<0) | (guin>=game.numgui)) quit("!SetButtonPic: invalid GUI number");
    if ((objn<0) | (objn>=guis[guin].numobjs)) quit("!SetButtonPic: invalid object number");
    if (guis[guin].get_control_type(objn)!=GOBJ_BUTTON)
        quit("!SetButtonPic: specified control is not a button");
    if ((ptype<1) | (ptype>3)) quit("!SetButtonPic: invalid pic type");

    GUIButton*guil=(GUIButton*)guis[guin].objs[objn];
    if (ptype==1) {
        Button_SetNormalGraphic(guil, slotn);
    }
    else if (ptype==2) {
        // mouseover pic
        Button_SetMouseOverGraphic(guil, slotn);
    }
    else { // pushed pic
        Button_SetPushedGraphic(guil, slotn);
    }
}
