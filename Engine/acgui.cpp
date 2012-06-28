/*
  Adventure Game Studio source code Copyright 1999-2011 Chris Jones.
  All rights reserved.

  The AGS Editor Source Code is provided under the Artistic License 2.0
  http://www.opensource.org/licenses/artistic-license-2.0.php

  You MAY NOT compile your own builds of the engine without making it EXPLICITLY
  CLEAR that the code has been altered from the Standard Version.

*/
#pragma unmanaged
#define WGT2ALLEGRO_NOFUNCTIONS
#include "wgt2allg.h"
#define CROOM_NOFUNCTIONS
#include "acroom.h"
#include "acruntim.h"
#include "acgui.h"
#include <ctype.h>

#include "bigend.h"

#undef CROOM_NOFUNCTIONS

#ifdef THIS_IS_THE_ENGINE
#define get_adjusted_spritewidth(x) wgetblockwidth(spriteset[x])
#define get_adjusted_spriteheight(x) wgetblockheight(spriteset[x])
#define is_sprite_alpha(x) ((game.spriteflags[x] & SPF_ALPHACHANNEL) != 0)
#else
extern int get_adjusted_spritewidth(int spr);
extern int get_adjusted_spriteheight(int spr);
#define final_col_dep 32
#define is_sprite_alpha(x) false
#endif

int guis_need_update = 1;
int all_buttons_disabled = 0, gui_inv_pic = -1;
int gui_disabled_style = 0;

extern SpriteCache spriteset;
extern void draw_sprite_compensate(int spr, int x, int y, int xray);
extern inline int divide_down_coordinate(int coord);
extern inline int multiply_up_coordinate(int coord);
extern inline void multiply_up_coordinates(int *x, int *y);
extern inline int get_fixed_pixel_size(int pixels);

char lines[MAXLINE][200];
int  numlines;

#ifdef THIS_IS_THE_ENGINE

extern void ensure_text_valid_for_font(char *, int);
extern void replace_macro_tokens(char*,char*);
extern void break_up_text_into_lines(int wii,int fonnt,char*todis);
extern int eip_guinum, eip_guiobj;
#define SET_EIP(x) our_eip=x;
#define OUTLINE_ALL_OBJECTS 0

#else  // this is the editor

extern bool outlineGuiObjects;
#define SET_EIP(x)
#define OUTLINE_ALL_OBJECTS outlineGuiObjects
#define wgettextwidth_compensate wgettextwidth

#endif

void GUIObject::init() {
  int jj;
  scriptName[0] = 0;
  for (jj = 0; jj < MAX_GUIOBJ_EVENTS; jj++)
    eventHandlers[jj][0] = 0;
}

int GUIObject::IsDisabled() {
  if (flags & GUIF_DISABLED)
    return 1;
  if (all_buttons_disabled)
    return 1;
  return 0;
}

void GUIObject::WriteToFile(FILE * ooo)
{
  // MACPORT FIX: swap
  fwrite(&flags, sizeof(int), BASEGOBJ_SIZE, ooo);
  fputstring(scriptName, ooo);

  putw(GetNumEvents(), ooo);
  for (int kk = 0; kk < GetNumEvents(); kk++)
    fputstring(eventHandlers[kk], ooo);
}

void GUIObject::ReadFromFile(FILE * ooo, int version)
{
  // MACPORT FIX: swap
  fread(&flags, sizeof(int), BASEGOBJ_SIZE, ooo);
  if (version >= 106)
    fgetstring_limit(scriptName, ooo, MAX_GUIOBJ_SCRIPTNAME_LEN);
  else
    scriptName[0] = 0;

  int kk;
  for (kk = 0; kk < GetNumEvents(); kk++)
    eventHandlers[kk][0] = 0;

  if (version >= 108) {
    int numev = getw(ooo);
    if (numev > GetNumEvents())
      quit("Error: too many control events, need newer version");

    // read in the event handler names
    for (kk = 0; kk < numev; kk++)
      fgetstring_limit(eventHandlers[kk], ooo, MAX_GUIOBJ_EVENTHANDLER_LEN + 1);
  }
}

void GUISlider::WriteToFile(FILE * ooo)
{
  GUIObject::WriteToFile(ooo);
  // MACPORT FIX: swap
  fwrite(&min, sizeof(int), 7, ooo);
}

void GUISlider::ReadFromFile(FILE * ooo, int version)
{
  int sizeToRead = 4;

  if (version >= 104)
    sizeToRead = 7;
  else {
    handlepic = -1;
    handleoffset = 0;
    bgimage = 0;
  }

  GUIObject::ReadFromFile(ooo, version);
  fread(&min, sizeof(int), sizeToRead, ooo);
}

void GUISlider::Draw()
{
  int bartlx, bartly, barbrx, barbry;
  int handtlx, handtly, handbrx, handbry, thickness;

  if (min >= max)
    max = min + 1;

  if (value > max)
    value = max;

  if (value < min)
    value = min;

  // it's a horizontal slider
  if (wid > hit) {
    thickness = hit / 3;
    bartlx = x + 1;
    bartly = y + hit / 2 - thickness;
    barbrx = x + wid - 1;
    barbry = y + hit / 2 + thickness + 1;
    handtlx = (int)(((float)(value - min) / (float)(max - min)) * (float)(wid - 4) - 2) + bartlx + 1;
    handtly = bartly - (thickness - 1);
    handbrx = handtlx + get_fixed_pixel_size(4);
    handbry = barbry + (thickness - 1);

    if (handlepic > 0) {
      // store the centre of the pic rather than the top
      handtly = bartly + (barbry - bartly) / 2 + get_fixed_pixel_size(1);
      handtlx += get_fixed_pixel_size(2);
    }
    handtly += multiply_up_coordinate(handleoffset);
    handbry += multiply_up_coordinate(handleoffset);
  }
  // vertical slider
  else {
    thickness = wid / 3;
    bartlx = x + wid / 2 - thickness;
    bartly = y + 1;
    barbrx = x + wid / 2 + thickness + 1;
    barbry = y + hit - 1;
    handtly = (int)(((float)(max - value) / (float)(max - min)) * (float)(hit - 4) - 2) + bartly + 1;
    handtlx = bartlx - (thickness - 1);
    handbry = handtly + get_fixed_pixel_size(4);
    handbrx = barbrx + (thickness - 1);

    if (handlepic > 0) {
      // store the centre of the pic rather than the left
      handtlx = bartlx + (barbrx - bartlx) / 2 + get_fixed_pixel_size(1);
      handtly += get_fixed_pixel_size(2);
    }
    handtlx += multiply_up_coordinate(handleoffset);
    handbrx += multiply_up_coordinate(handleoffset);
  }

  if (bgimage > 0) {
    // tiled image as slider background
    int xinc = 0, yinc = 0;
    if (wid > hit) {
      // horizontal slider
      xinc = get_adjusted_spritewidth(bgimage);
      // centre the image vertically
      bartly = y + (hit / 2) - get_adjusted_spriteheight(bgimage) / 2;
    }
    else {
      // vertical slider
      yinc = get_adjusted_spriteheight(bgimage);
      // centre the image horizontally
      bartlx = x + (wid / 2) - get_adjusted_spritewidth(bgimage) / 2;
    }

    int cx = bartlx, cy = bartly;
    // draw the tiled background image
    do {
      draw_sprite_compensate(bgimage, cx, cy, 1);
      cx += xinc;
      cy += yinc;
      // done as a do..while so that at least one of the image is drawn
    } while ((cx + xinc <= barbrx) && (cy + yinc <= barbry));

  }
  else {
    // normal grey background
    wsetcolor(16);
    wbar(bartlx + 1, bartly + 1, barbrx - 1, barbry - 1);

    wsetcolor(8);
    wline(bartlx, bartly, bartlx, barbry);
    wline(bartlx, bartly, barbrx, bartly);

    wsetcolor(15);
    wline(barbrx, bartly + 1, barbrx, barbry);
    wline(bartlx, barbry, barbrx, barbry);
  }

  if (handlepic > 0) {
    // an image for the slider handle
    if (spriteset[handlepic] == NULL)
      handlepic = 0;
    handtlx -= get_adjusted_spritewidth(handlepic) / 2;
    handtly -= get_adjusted_spriteheight(handlepic) / 2;
    draw_sprite_compensate(handlepic, handtlx, handtly, 1);
    handbrx = handtlx + get_adjusted_spritewidth(handlepic);
    handbry = handtly + get_adjusted_spriteheight(handlepic);
  }
  else {
    // normal grey tracker handle
    wsetcolor(7);
    wbar(handtlx, handtly, handbrx, handbry);

    wsetcolor(15);
    wline(handtlx, handtly, handbrx, handtly);
    wline(handtlx, handtly, handtlx, handbry);

    wsetcolor(16);
    wline(handbrx, handtly + 1, handbrx, handbry);
    wline(handtlx + 1, handbry, handbrx, handbry);
  }

  cached_handtlx = handtlx;
  cached_handtly = handtly;
  cached_handbrx = handbrx;
  cached_handbry = handbry;
}

void GUISlider::MouseMove(int xp, int yp)
{
  if (mpressed == 0)
    return;

  if (wid > hit)                // horizontal slider
    value = (int)(((float)((xp - x) - 2) / (float)(wid - 4)) * (float)(max - min)) + min;
  else                          // vertical slider
    value = (int)(((float)(((y + hit) - yp) - 2) / (float)(hit - 4)) * (float)(max - min)) + min;

  if (value > max)
    value = max;

  if (value < min)
    value = min;

  guis_need_update = 1;
  activated = 1;
}

void GUILabel::WriteToFile(FILE * ooo)
{
  GUIObject::WriteToFile(ooo);
  // MACPORT FIXES: swap
  //fwrite(&text[0], sizeof(char), 200, ooo);
  putw((int)strlen(text) + 1, ooo);
  fwrite(&text[0], sizeof(char), strlen(text) + 1, ooo);
  fwrite(&font, sizeof(int), 3, ooo);
}

void GUILabel::ReadFromFile(FILE * ooo, int version)
{
  GUIObject::ReadFromFile(ooo, version);

  if (textBufferLen > 0)
    free(text);

  if (version < 113) {
    textBufferLen = 200;
  }
  else {
    textBufferLen = getw(ooo);
  }

  text = (char*)malloc(textBufferLen);
  fread(&text[0], sizeof(char), textBufferLen, ooo);

  fread(&font, sizeof(int), 3, ooo);
  if (textcol == 0)
    textcol = 16;
}

void GUILabel::SetText(const char *newText) {

  if ((int)strlen(newText) < textBufferLen) {
    strcpy(this->text, newText);
    return;
  }

  if (textBufferLen > 0)
    free(this->text);

  textBufferLen = (int)strlen(newText) + 1;

  this->text = (char*)malloc(textBufferLen);
  strcpy(this->text, newText);

  // restrict to this length
  if (textBufferLen >= MAX_GUILABEL_TEXT_LEN)
    this->text[MAX_GUILABEL_TEXT_LEN - 1] = 0;
}

const char *GUILabel::GetText() {
  return text;
}

void GUILabel::printtext_align(int yy, char *teptr)
{
  int outxp = x;
  if (align == GALIGN_CENTRE)
    outxp += wid / 2 - wgettextwidth(teptr, font) / 2;
  else if (align == GALIGN_RIGHT)
    outxp += wid - wgettextwidth(teptr, font);

  wouttext_outline(outxp, yy, font, teptr);
}

void GUILabel::Draw()
{
  int cyp = y, TEXT_HT;
  char oritext[MAX_GUILABEL_TEXT_LEN], *teptr;

  check_font(&font);
#ifdef THIS_IS_THE_ENGINE
  replace_macro_tokens(get_translation(text), oritext);
  ensure_text_valid_for_font(oritext, font);
#else
  strcpy(oritext, text);
#endif
  teptr = &oritext[0];
  TEXT_HT = wgettextheight("ZhypjIHQFb", font) + 1;

  wtextcolor(textcol);

#ifdef THIS_IS_THE_ENGINE
  // Use the engine's word wrap tool, to have hebrew-style writing
  // and other features

  break_up_text_into_lines (wid, font, teptr);

#else

  numlines=0;
  split_lines_leftright(teptr, wid, font);

#endif // CUSTOM_WORDWRAP

  for (int aa = 0; aa < numlines; aa++) {
    printtext_align(cyp, lines[aa]);
    cyp += TEXT_HT;
    if (cyp > y + hit)
      break;
  }

}

void GUITextBox::WriteToFile(FILE * ooo)
{
  GUIObject::WriteToFile(ooo);
  // MACPORT FIXES: swap
  fwrite(&text[0], sizeof(char), 200, ooo);
  fwrite(&font, sizeof(int), 3, ooo);
}

void GUITextBox::ReadFromFile(FILE * ooo, int version)
{
  GUIObject::ReadFromFile(ooo, version);
  // MACPORT FIXES: swap
  fread(&text[0], sizeof(char), 200, ooo);
  fread(&font, sizeof(int), 3, ooo);
  if (textcol == 0)
    textcol = 16;
}

void GUITextBox::Draw()
{

  check_font(&font);
  wtextcolor(textcol);
  wsetcolor(textcol);
  if ((exflags & GTF_NOBORDER) == 0) {
    wrectangle(x, y, x + wid - 1, y + hit - 1);
    if (get_fixed_pixel_size(1) > 1)
      wrectangle(x + 1, y + 1, x + wid - get_fixed_pixel_size(1), y + hit - get_fixed_pixel_size(1));
  }

#ifdef THIS_IS_THE_ENGINE
  int startx, starty;

  wouttext_outline(x + 1 + get_fixed_pixel_size(1), y + 1 + get_fixed_pixel_size(1), font, text);
  
  if (!IsDisabled()) {
    // draw a cursor
    startx = wgettextwidth(text, font) + x + 3;
    starty = y + 1 + wgettextheight("BigyjTEXT", font);
    wrectangle(startx, starty, startx + get_fixed_pixel_size(5), starty + (get_fixed_pixel_size(1) - 1));
  }
#else
  // print something fake so we can see what it looks like
  wouttext_outline(x + 2, y + 2, font, "Text Box Contents");
#endif
}

void GUITextBox::KeyPress(int kp)
{
  guis_need_update = 1;
  // backspace, remove character
  if ((kp == 8) && (strlen(text) > 0)) {
    text[strlen(text) - 1] = 0;
    return;
  } else if (kp == 8)
    return;

  // other key, continue
  if ((kp >= 128) && (!fontRenderers[font]->SupportsExtendedCharacters(font)))
    return;

  if (kp == 13) {
    activated++;
    return;
  }

  if (strlen(text) >= 199)
    return;

  text[strlen(text) + 1] = 0;
  text[strlen(text)] = kp;

  // if the new string is too long, remove the new character
  if (wgettextwidth(text, font) > (wid - (6 + get_fixed_pixel_size(5))))
    text[strlen(text) - 1] = 0;

}


void GUIListBox::ChangeFont(int newfont) {
  rowheight = wgettextheight("YpyjIHgMNWQ", font) + get_fixed_pixel_size(2);
  num_items_fit = hit / rowheight;
  font = newfont;
}

void GUIListBox::WriteToFile(FILE * ooo)
{
  int a;

  GUIObject::WriteToFile(ooo);
  // MACPORT FIXES: swap
  fwrite(&numItems, sizeof(int), 11, ooo);
  putw(alignment, ooo);
  putw(reserved1, ooo);
  putw(selectedbgcol, ooo);
  for (a = 0; a < numItems; a++)
    fwrite(&items[a][0], sizeof(char), strlen(items[a]) + 1, ooo);

  if (exflags & GLF_SGINDEXVALID)
    fwrite(&saveGameIndex[0], sizeof(short), numItems, ooo);
}

void GUIListBox::ReadFromFile(FILE * ooo, int version)
{
  int a, i;
  char tempbuf[300];

  GUIObject::ReadFromFile(ooo, version);
  // MACPORT FIXES: swap
  fread(&numItems, sizeof(int), 11, ooo);

  if (version >= 112) {
    alignment = getw(ooo);
    reserved1 = getw(ooo);
  }
  else {
    alignment = GALIGN_LEFT;
    reserved1 = 0;
  }

  if (version >= 107) {
    selectedbgcol = getw(ooo);
  }
  else {
    selectedbgcol = textcol;
    if (selectedbgcol == 0)
      selectedbgcol = 16;
  }

  for (a = 0; a < numItems; a++) {
    i = 0;
    while ((tempbuf[i] = fgetc(ooo)) != 0)
      i++;

    items[a] = (char *)malloc(strlen(tempbuf) + 5);
    strcpy(items[a], tempbuf);
    saveGameIndex[a] = -1;
  }

  if ((version >= 114) && (exflags & GLF_SGINDEXVALID)) {
    fread(&saveGameIndex[0], sizeof(short), numItems, ooo);
  }

  if (textcol == 0)
    textcol = 16;
}

int GUIListBox::AddItem(const char *toadd)
{
  if (numItems >= MAX_LISTBOX_ITEMS)
    return -1;

  guis_need_update = 1;
  items[numItems] = (char *)malloc(strlen(toadd) + 5);
  strcpy(items[numItems], toadd);
  saveGameIndex[numItems] = -1;
  numItems++;
  return numItems - 1;
}

int GUIListBox::InsertItem(int index, const char *toadd)
{
  int aa;

  if (numItems >= MAX_LISTBOX_ITEMS)
    return -1;

  if ((index < 0) || (index > numItems))
    return -1;

  guis_need_update = 1;

  for (aa = numItems; aa > index; aa--) {
    items[aa] = items[aa - 1];
    saveGameIndex[aa] = saveGameIndex[aa - 1];
  }

  items[index] = (char *)malloc(strlen(toadd) + 5);
  strcpy(items[index], toadd);
  saveGameIndex[index] = -1;
  numItems++;

  if (selected >= index)
    selected++;

  return numItems - 1;
}

void GUIListBox::SetItemText(int item, const char *newtext)
{
  if ((item >= numItems) || (item < 0))
    return;

  guis_need_update = 1;
  free(items[item]);
  items[item] = (char *)malloc(strlen(newtext) + 5);
  strcpy(items[item], newtext);
}

void GUIListBox::Clear()
{
  int aa;
  for (aa = 0; aa < numItems; aa++)
    free(items[aa]);

  numItems = 0;
  selected = 0;
  topItem = 0;
  guis_need_update = 1;
}

void GUIListBox::RemoveItem(int index)
{
  int aa;

  if ((index < 0) || (index >= numItems))
    return;

  free(items[index]);
  numItems--;
  for (aa = index; aa < numItems; aa++) {
    items[aa] = items[aa + 1];
    saveGameIndex[aa] = saveGameIndex[aa + 1];
  }

  if (selected > index)
    selected--;
  if (selected >= numItems)
    selected = -1;

  guis_need_update = 1;
}

void GUIListBox::Draw()
{
  wid--;
  hit--;
  int pixel_size = get_fixed_pixel_size(1);

  check_font(&font);
  wtextcolor(textcol);
  wsetcolor(textcol);
  if ((exflags & GLF_NOBORDER) == 0) {
    wrectangle(x, y, x + wid + (pixel_size - 1), y + hit + (pixel_size - 1));
    if (pixel_size > 1)
      wrectangle(x + 1, y + 1, x + wid, y + hit);
  }

  int rightHandEdge = (x + wid) - pixel_size - 1;

  // use ChangeFont to update the rowheight and num_items_fit
  ChangeFont(font);

  // draw the scroll bar in if necessary
  if ((numItems > num_items_fit) && ((exflags & GLF_NOBORDER) == 0) && ((exflags & GLF_NOARROWS) == 0)) {
    int xstrt, ystrt;
    wrectangle(x + wid - get_fixed_pixel_size(7), y, (x + (pixel_size - 1) + wid) - get_fixed_pixel_size(7), y + hit);
    wrectangle(x + wid - get_fixed_pixel_size(7), y + hit / 2, x + wid, y + hit / 2 + (pixel_size - 1));

    xstrt = (x + wid - get_fixed_pixel_size(6)) + (pixel_size - 1);
    ystrt = (y + hit - 3) - get_fixed_pixel_size(5);

    triangle(abuf, xstrt, ystrt, xstrt + get_fixed_pixel_size(4), ystrt, 
             xstrt + get_fixed_pixel_size(2),
             ystrt + get_fixed_pixel_size(5), get_col8_lookup(textcol));

    ystrt = y + 3;
    triangle(abuf, xstrt, ystrt + get_fixed_pixel_size(5), 
             xstrt + get_fixed_pixel_size(4), 
             ystrt + get_fixed_pixel_size(5),
             xstrt + get_fixed_pixel_size(2), ystrt, get_col8_lookup(textcol));

    rightHandEdge -= get_fixed_pixel_size(7);
  }

#ifndef THIS_IS_THE_ENGINE
#define numItems 2
  items[0] = "Sample selected";
  items[1] = "Sample item";
#endif

  for (int a = 0; a < num_items_fit; a++) {
    int thisyp;
    if (a + topItem >= numItems)
      break;

    thisyp = y + pixel_size + a * rowheight;
    if (a + topItem == selected) {
      int stretchto = (x + wid) - pixel_size;

      wtextcolor(backcol);

      if (selectedbgcol > 0) {
        // draw the selected item bar (if colour not transparent)
        wsetcolor(selectedbgcol);
        if ((num_items_fit < numItems) && ((exflags & GLF_NOBORDER) == 0) && ((exflags & GLF_NOARROWS) == 0))
          stretchto -= get_fixed_pixel_size(7);

        wbar(x + pixel_size, thisyp, stretchto, thisyp + rowheight - pixel_size);
      }
    }
    else
      wtextcolor(textcol);

    if (alignment == GALIGN_LEFT)
      wouttext_outline(x + 1 + pixel_size, thisyp + 1, font, items[a + topItem]);
    else {
      int textWidth = wgettextwidth(items[a + topItem], font);

      if (alignment == GALIGN_RIGHT)
        wouttext_outline(rightHandEdge - textWidth, thisyp + 1, font, items[a + topItem]);
      else
        wouttext_outline(((rightHandEdge - x) / 2) + x - (textWidth / 2), thisyp + 1, font, items[a + topItem]);
    }

  }
  wid++;
  hit++;
#ifndef THIS_IS_THE_ENGINE
#undef numItems
#endif
}

int GUIListBox::IsInRightMargin(int xx) {

  if ((xx >= (wid - get_fixed_pixel_size(6))) && ((exflags & GLF_NOBORDER) == 0) && ((exflags & GLF_NOARROWS) == 0)) {
    return 1;
  }
  return 0;
}

int GUIListBox::GetIndexFromCoordinates(int xx, int yy) {
  if (IsInRightMargin(xx))
    return -1;

  int onindex = yy / rowheight + topItem;
  if ((onindex < 0) || (onindex >= numItems))
    return -1;

  return onindex;
}

int GUIListBox::MouseDown()
{
  if (IsInRightMargin(mousexp)) {
    if ((mouseyp < hit / 2) && (topItem > 0))
      topItem--;

    if ((mouseyp >= hit / 2) && (numItems > topItem + num_items_fit))
      topItem++;

    return 0;
  }

  int newsel = GetIndexFromCoordinates(mousexp, mouseyp);
  if (newsel < 0)
    return 0;

  selected = newsel;
  activated = 1;
  return 0;
}

void GUIInv::CalculateNumCells() {
  if (loaded_game_file_version >= 31) // 2.70
  {
    itemsPerLine = wid / multiply_up_coordinate(itemWidth);
    numLines = hit / multiply_up_coordinate(itemHeight);
  }
  else
  {
    itemsPerLine = floor((float)wid / (float)multiply_up_coordinate(itemWidth) + 0.5f);
    numLines = floor((float)hit / (float)multiply_up_coordinate(itemHeight) + 0.5f);
  }
}

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

#ifdef THIS_IS_THE_ENGINE
    // Allow it to change the string to unicode if it's TTF
    char oritext[200];
    strcpy(oritext, get_translation(text));
    ensure_text_valid_for_font(oritext, font);
#else
    char *oritext = text;
#endif

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

DynamicArray<GUIButton> guibuts;
//GUIButton guibuts[MAX_OBJ_EACH_TYPE];
int numguibuts = 0;

DynamicArray<GUILabel> guilabels;
int numguilabels = 0;

DynamicArray<GUIInv> guiinv;
int numguiinv = 0;

DynamicArray<GUISlider> guislider;
int numguislider = 0;

DynamicArray<GUITextBox> guitext;
int numguitext = 0;

DynamicArray<GUIListBox> guilist;
int numguilist = 0;

char GUIMain::oNameBuffer[20];

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
    
#ifdef THIS_IS_THE_ENGINE
    eip_guiobj = drawOrder[aa];
#endif

    GUIObject *objToDraw = objs[drawOrder[aa]];

    if ((objToDraw->IsDisabled()) && (gui_disabled_style == GUIDIS_BLACKOUT))
      continue;
    if (!objToDraw->IsVisible())
      continue;

    objToDraw->Draw();

    int selectedColour = 14;

    if (highlightobj == drawOrder[aa]) {
      if (OUTLINE_ALL_OBJECTS)
        selectedColour = 13;
      wsetcolor(selectedColour);
      draw_blob(objToDraw->x + objToDraw->wid - get_fixed_pixel_size(1) - 1, objToDraw->y);
      draw_blob(objToDraw->x, objToDraw->y + objToDraw->hit - get_fixed_pixel_size(1) - 1);
      draw_blob(objToDraw->x, objToDraw->y);
      draw_blob(objToDraw->x + objToDraw->wid - get_fixed_pixel_size(1) - 1, 
                objToDraw->y + objToDraw->hit - get_fixed_pixel_size(1) - 1);
    }
    if (OUTLINE_ALL_OBJECTS) {
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

void removeBackslashBracket(char *lbuffer) {
  char *slashoffs;
  while ((slashoffs = strstr(lbuffer, "\\[")) != NULL) {
    // remove the backslash
    memmove(slashoffs, slashoffs + 1, strlen(slashoffs));
  }
}

// Break up the text into lines, using normal Western left-right style
void split_lines_leftright(const char *todis, int wii, int fonnt) {
  // v2.56.636: rewrote this function because the old version
  // was crap and buggy
  int i = 0;
  int nextCharWas;
  int splitAt;
  char *theline;
  // make a copy, since we change characters in the original string
  // and this might be in a read-only bit of memory
  char textCopyBuffer[STD_BUFFER_SIZE];
  strcpy(textCopyBuffer, todis);
  theline = textCopyBuffer;

  while (1) {
    splitAt = -1;

    if (theline[i] == 0) {
      // end of the text, add the last line if necessary
      if (i > 0) {
        strcpy(lines[numlines], theline);
        removeBackslashBracket(lines[numlines]);
        numlines++;
      }
      break;
    }

    // temporarily terminate the line here and test its width
    nextCharWas = theline[i + 1];
    theline[i + 1] = 0;

    // force end of line with the [ character (except if \[ )
    if ((theline[i] == '[') && ((i == 0) || (theline[i - 1] != '\\')))
      splitAt = i;
    // otherwise, see if we are too wide
    else if (wgettextwidth_compensate(theline, fonnt) >= wii) {
      int endline = i;
      while ((theline[endline] != ' ') && (endline > 0))
        endline--;

      // single very wide word, display as much as possible
      if (endline == 0)
        endline = i - 1;

      splitAt = endline;
    }

    // restore the character that was there before
    theline[i + 1] = nextCharWas;

    if (splitAt >= 0) {
      // add this line
      nextCharWas = theline[splitAt];
      theline[splitAt] = 0;
      strcpy(lines[numlines], theline);
      removeBackslashBracket(lines[numlines]);
      numlines++;
      theline[splitAt] = nextCharWas;
      if (numlines >= MAXLINE) {
        strcat(lines[numlines-1], "...");
        break;
      }
      // the next line starts from here
      theline += splitAt;
      // skip the space or bracket that caused the line break
      if ((theline[0] == ' ') || (theline[0] == '['))
        theline++;
      i = -1;
    }
    
    i++;
  }

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

#ifdef THIS_IS_THE_ENGINE
#undef get_adjusted_spritewidth
#undef get_adjusted_spriteheight
#endif
