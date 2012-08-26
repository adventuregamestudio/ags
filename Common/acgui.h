/*
** Adventure Game Studio GUI routines
** Copyright (C) 2000-2005, Chris Jones
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE;
** the contents of this file may not be disclosed to third parties,
** copied or duplicated in any form, in whole or in part, without
** prior express permission from Chris Jones.
**
*/

#ifndef __ACGUI_H
#define __ACGUI_H

#include "bigend.h"

#include <math.h>
extern int loaded_game_file_version;

#ifndef __WGT4_H
#ifndef CROOM_NOFUNCTIONS
#error Must include wgt2allg.h first
#endif
#endif

#ifndef WOUTTEXT_REVERSE
#define WOUTTEXT_REVERSE wouttext_outline
#endif

#define MAX_LISTBOX_ITEMS 200
#define MAX_GUILABEL_TEXT_LEN 2048
#define GUIMAGIC          0xcafebeef
//#define MAX_OBJ_EACH_TYPE 251



#ifdef THIS_IS_THE_ENGINE
extern void wouttext_outline(int, int, int, char *);
inline void check_font(int *fontnum)
{
}
#else

#define wouttext_outline(a, b, c, d) wouttextxy(a, b, c, d)
extern GameSetupStruct thisgame;
extern void check_font(int *fontnum);
#endif

template <typename T> struct DynamicArray {
private:
  T defaultConstructed;
  T *data;
  int datasize;

public:

  DynamicArray() {
    data = NULL;
    datasize = 0;
  }

  ~DynamicArray() {
    if (data)
      free(data);
  }

  void GrowTo(int newsize);
  void SetSizeTo(int newsize);
  T& operator[] (int index);
};

template <typename T>
void DynamicArray<T>::GrowTo(int newsize) {
  if (datasize < newsize) {
    SetSizeTo(newsize);
  }
}

template <typename T>
void DynamicArray<T>::SetSizeTo(int newsize) {
  int dsWas = datasize;
  datasize = newsize;
  if (data == NULL)
    data = (T*)calloc(sizeof(T), datasize);
  else {
    T *newdata = (T*)calloc(sizeof(T), datasize);
    if (dsWas > datasize)
      dsWas = datasize;
    memcpy(newdata, data, sizeof(T) * dsWas);
    free(data);
    data = newdata;
  }
  // "construct" the new objects by copying the default-constructed
  // object into them
  // this is necessary so that the vtables are set up correctly
  for (int qq = dsWas; qq < datasize; qq++) {
    memcpy(&data[qq], &defaultConstructed, sizeof(T));
  }
}

template <typename T>
T& DynamicArray<T>::operator[] (int index) {
  if (index < 0)
    index = 0;
  if (index >= datasize) {
    // grow it 5 bigger, so we don't have to keep reallocating
    GrowTo(index + 5);
  }
  return data[index];
}


#define GUIF_DEFAULT  1
#define GUIF_CANCEL   2
#define GUIF_DISABLED 4
#define GUIF_TABSTOP  8
#define GUIF_INVISIBLE 0x10
#define GUIF_CLIP     0x20
#define GUIF_NOCLICKS 0x40
#define GUIF_DELETED  0x8000
#define BASEGOBJ_SIZE 7
#define GALIGN_LEFT   0
#define GALIGN_RIGHT  1
#define GALIGN_CENTRE 2
#define MAX_GUIOBJ_SCRIPTNAME_LEN 25
#define MAX_GUIOBJ_EVENTS 10
#define MAX_GUIOBJ_EVENTHANDLER_LEN 30
struct GUIObject
{
  int guin, objn;    // gui and object number of this object
  unsigned int flags;
  int x, y;
  int wid, hit;
  int zorder;
  int activated;
  char scriptName[MAX_GUIOBJ_SCRIPTNAME_LEN + 1];
  char eventHandlers[MAX_GUIOBJ_EVENTS][MAX_GUIOBJ_EVENTHANDLER_LEN + 1];

  virtual void MouseMove(int, int) = 0; // x,y relative to gui
  virtual void MouseOver() = 0; // mouse moves onto object
  virtual void MouseLeave() = 0;        // mouse moves off object
  virtual int  MouseDown() { // button down - return 1 to lock focus
    return 0;
  }
  virtual void MouseUp() = 0;   // button up
  virtual void KeyPress(int) = 0;
  virtual void Draw() = 0;
  // overridable routine to determine whether the mouse is over
  // the control
  virtual int  IsOverControl(int p_x, int p_y, int p_extra) {
    if ((p_x >= x) && (p_y >= y) && (p_x < x + wid + p_extra) && (p_y < y + hit + p_extra))
      return 1;
    return 0;
  }
  // we can't just fread/fwrite inherited objects because of vtbl, so use:
  virtual void WriteToFile(FILE *);
  virtual void ReadFromFile(FILE *, int);
  // called when the control is resized
  virtual void Resized() { }
  virtual int  GetNumEvents() {
    return numSupportedEvents;
  }
  virtual const char *GetEventName(int idx) {
    if ((idx < 0) || (idx >= numSupportedEvents))
      return NULL;
    return supportedEvents[idx];
  }
  virtual const char *GetEventArgs(int idx) {
    if ((idx < 0) || (idx >= numSupportedEvents))
      return NULL;
    return supportedEventArgs[idx];
  }
  void init();

  int IsDeleted() {
    return flags & GUIF_DELETED;
  }
  int IsDisabled();
  void Enable() {
    flags &= ~GUIF_DISABLED;
  }
  void Disable() {
    flags |= GUIF_DISABLED;
  }
  int IsVisible() {
    if (flags & GUIF_INVISIBLE)
      return 0;
    return 1;
  }
  void Show() {
    flags &= ~GUIF_INVISIBLE;
  }
  void Hide() {
    flags |= GUIF_INVISIBLE;
  }
  int IsClickable() {
#ifdef THIS_IS_THE_ENGINE
    return !(flags & GUIF_NOCLICKS);
#else  // make sure the button can be selected in the editor
    return 1;
#endif
  }
  void SetClickable(bool newValue) {
    flags &= ~GUIF_NOCLICKS;
    if (!newValue)
      flags |= GUIF_NOCLICKS;
  }

protected:
  const char *supportedEvents[MAX_GUIOBJ_EVENTS];
  const char *supportedEventArgs[MAX_GUIOBJ_EVENTS];
  int numSupportedEvents;
};


struct GUISlider:public GUIObject
{
  int min, max;
  int value, mpressed;
  int handlepic, handleoffset, bgimage;
  // The following variables are not persisted on disk
  // Cached (x1, x2, y1, y2) co-ordinates of slider handle
  int cached_handtlx, cached_handbrx;
  int cached_handtly, cached_handbry;

  virtual void WriteToFile(FILE *);
  virtual void ReadFromFile(FILE *, int);
  void Draw();
  void MouseMove(int xp, int yp);

  void MouseOver()
  {
  }

  void MouseLeave()
  {
  }

  virtual int MouseDown()
  {
    mpressed = 1;
    // lock focus to ourselves
    return 1;
  }

  void MouseUp()
  {
    mpressed = 0;
  }

  void KeyPress(int kp)
  {
  }

  virtual int IsOverControl(int p_x, int p_y, int p_extra) {
    // check the overall boundary
    if (GUIObject::IsOverControl(p_x, p_y, p_extra))
      return 1;
    // now check the handle too
    if ((p_x >= cached_handtlx) && (p_y >= cached_handtly) &&
        (p_x < cached_handbrx) && (p_y < cached_handbry))
      return 1;
    return 0;
  }

  void reset()
  {
    GUIObject::init();
    min = 0;
    max = 10;
    value = 0;
    activated = 0;
    cached_handtlx = cached_handbrx = 0;
    cached_handtly = cached_handbry = 0;
    numSupportedEvents = 1;
    supportedEvents[0] = "Change";
    supportedEventArgs[0] = "GUIControl *control";
  }

  GUISlider() {
    reset();
  }
};


struct GUILabel:public GUIObject
{
private:
  char *text;
  int textBufferLen;
public:
  int font, textcol, align;

  virtual void WriteToFile(FILE *);
  virtual void ReadFromFile(FILE *, int);
  void Draw();
  void printtext_align(int yy, char *teptr);
  void SetText(const char *newText);
  const char *GetText();

  void MouseMove(int x, int y)
  {
  }

  void MouseOver()
  {
  }

  void MouseLeave()
  {
  }

  void MouseUp()
  {
  }

  void KeyPress(int kp)
  {
  }

  void reset()
  {
    GUIObject::init();
    align = GALIGN_LEFT;
    font = 0;
    textcol = 0;
    numSupportedEvents = 0;
    text = "";
    textBufferLen = 0;
  }

  GUILabel() {
    reset();
  }
};


#define GTF_NOBORDER  1
struct GUITextBox:public GUIObject
{
  char text[200];
  int font, textcol, exflags;

  virtual void WriteToFile(FILE *);
  virtual void ReadFromFile(FILE *, int);
  void Draw();
  void KeyPress(int);

  void MouseMove(int x, int y)
  {
  }

  void MouseOver()
  {
  }

  void MouseLeave()
  {
  }

  void MouseUp()
  {
  }

  void reset()
  {
    GUIObject::init();
    font = 0;
    textcol = 0;
    text[0] = 0;
    numSupportedEvents = 1;
    supportedEvents[0] = "Activate";
    supportedEventArgs[0] = "GUIControl *control";
  }

  GUITextBox() {
    reset();
  }
};


#define GLF_NOBORDER     1
#define GLF_NOARROWS     2
#define GLF_SGINDEXVALID 4

struct GUIListBox:public GUIObject
{
  char *items[MAX_LISTBOX_ITEMS];
  short saveGameIndex[MAX_LISTBOX_ITEMS];
  int numItems, selected, topItem, mousexp, mouseyp;
  int rowheight, num_items_fit;
  int font, textcol, backcol, exflags;
  int selectedbgcol;
  int alignment, reserved1;
  virtual void WriteToFile(FILE *);
  virtual void ReadFromFile(FILE *, int);
  int  AddItem(const char *toadd);
  int  InsertItem(int index, const char *toadd);
  void SetItemText(int index, const char *newtext);
  void RemoveItem(int index);
  void Clear();
  void Draw();
  int  IsInRightMargin(int x);
  int  GetIndexFromCoordinates(int x, int y);
  void ChangeFont(int newFont);
  virtual int MouseDown();
  
  void MouseMove(int nx, int ny)
  {
    mousexp = nx - x;
    mouseyp = ny - y;
  }

  void MouseOver()
  {
  }

  void MouseLeave()
  {
  }

  void MouseUp()
  {
  }

  void KeyPress(int kp)
  {
  }

  virtual void Resized() 
  {
    if (rowheight == 0)
    {
      check_font(&font);
      ChangeFont(font);
    }

    if (rowheight > 0)
      num_items_fit = hit / rowheight;
  }

  void reset()
  {
    GUIObject::init();
    mousexp = 0;
    mouseyp = 0;
    activated = 0;
    numItems = 0;
    topItem = 0;
    selected = 0;
    font = 0;
    textcol = 0;
    selectedbgcol = 16;
    backcol = 7;
    exflags = 0;
    numSupportedEvents = 1;
    supportedEvents[0] = "SelectionChanged";
    supportedEventArgs[0] = "GUIControl *control";
  }

  GUIListBox() {
    reset();
  }
};


struct GUIInv:public GUIObject
{
  int isover;
  int charId;   // whose inventory (-1 = current player)
  int itemWidth, itemHeight;
  int topIndex;

  int itemsPerLine, numLines;  // not persisted

  virtual void WriteToFile(FILE * ooo)
  {
    GUIObject::WriteToFile(ooo);
    putw(charId, ooo);
    putw(itemWidth, ooo);
    putw(itemHeight, ooo);
    putw(topIndex, ooo);
  }

  virtual void ReadFromFile(FILE * ooo, int version)
  {
    GUIObject::ReadFromFile(ooo, version);
    if (version >= 109) {
      charId = getw(ooo);
      itemWidth = getw(ooo);
      itemHeight = getw(ooo);
      topIndex = getw(ooo);
    }
    else {
      charId = -1;
      itemWidth = 40;
      itemHeight = 22;
      topIndex = 0;
    }

    if (loaded_game_file_version >= 31) // 2.70
    {
      // ensure that some items are visible
      if (itemWidth > wid)
        itemWidth = wid;
      if (itemHeight > hit)
        itemHeight = hit;
    }

    CalculateNumCells();
  }

  void CalculateNumCells();

  virtual void Resized() {
    CalculateNumCells();
  }

  int CharToDisplay();

  void Draw();

  void MouseMove(int x, int y)
  {
  }

  void MouseOver()
  {
    isover = 1;
  }

  void MouseLeave()
  {
    isover = 0;
  }

  void MouseUp()
  {
    if (isover)
      activated = 1;
  }

  void KeyPress(int kp)
  {
  }

  GUIInv() {
    isover = 0;
    numSupportedEvents = 0;
    charId = -1;
    itemWidth = 40;
    itemHeight = 22;
    topIndex = 0;
    CalculateNumCells();
  }
};


#define GBUT_ALIGN_TOPMIDDLE    0
#define GBUT_ALIGN_TOPLEFT      1
#define GBUT_ALIGN_TOPRIGHT     2
#define GBUT_ALIGN_MIDDLELEFT   3 
#define GBUT_ALIGN_CENTRED      4
#define GBUT_ALIGN_MIDDLERIGHT  5
#define GBUT_ALIGN_BOTTOMLEFT   6
#define GBUT_ALIGN_BOTTOMMIDDLE 7
#define GBUT_ALIGN_BOTTOMRIGHT  8

struct GUIButton:public GUIObject
{
  char text[50];
  int pic, overpic, pushedpic;
  int usepic, ispushed, isover;
  int font, textcol;
  int leftclick, rightclick;
  int lclickdata, rclickdata;
  int textAlignment, reserved1;

  virtual void WriteToFile(FILE * ooo);
  virtual void ReadFromFile(FILE * ooo, int);
  void Draw();
  void MouseUp();

  void MouseMove(int x, int y)
  {
  }

  void MouseOver()
  {
    if (ispushed)
      usepic = pushedpic;
    else
      usepic = overpic;

    isover = 1;
  }

  void MouseLeave()
  {
    usepic = pic;
    isover = 0;
  }

  virtual int MouseDown()
  {
    if (pushedpic > 0)
      usepic = pushedpic;

    ispushed = 1;
    return 0;
  }

  void KeyPress(int keycode)
  {
  }

  void reset()
  {
    GUIObject::init();
    usepic = -1;
    pic = -1;
    overpic = -1;
    pushedpic = -1;
    ispushed = 0;
    isover = 0;
    text[0] = 0;
    font = 0;
    textcol = 0;
    leftclick = 2;
    rightclick = 0;
    activated = 0;
    numSupportedEvents = 1;
    supportedEvents[0] = "Click";
    supportedEventArgs[0] = "GUIControl *control, MouseButton button";
  }

  GUIButton() {
    reset();
  }
};


extern DynamicArray<GUILabel> guilabels;
extern int numguilabels;

extern DynamicArray<GUISlider> guislider;
extern int numguislider;

extern DynamicArray<GUITextBox> guitext;
extern int numguitext;

extern DynamicArray<GUIButton> guibuts;
extern int numguibuts;

extern DynamicArray<GUIInv> guiinv;
extern int numguiinv;

extern DynamicArray<GUIListBox> guilist;
extern int numguilist;

#define MAX_OBJS_ON_GUI 30
#define GOBJ_BUTTON     1
#define GOBJ_LABEL      2
#define GOBJ_INVENTORY  3
#define GOBJ_SLIDER     4
#define GOBJ_TEXTBOX    5
#define GOBJ_LISTBOX    6
#define GUI_TEXTWINDOW  0x05    // set vtext[0] to this to signify text window
#define GUIF_NOCLICK    1
#define MOVER_MOUSEDOWNLOCKED -4000
struct GUIMain
{
  char vtext[4];                // for compatibility
  char name[16];                // the name of the GUI
  char clickEventHandler[20];
  int x, y, wid, hit;
  int focus;                    // which object has the focus
  int numobjs;                  // number of objects on gui
  int popup;                    // when it pops up (POPUP_NONE, POPUP_MOUSEY, POPUP_SCRIPT)
  int popupyp;                  // popup when mousey < this
  int bgcol, bgpic, fgcol;
  int mouseover, mousewasx, mousewasy;
  int mousedownon;
  int highlightobj;
  int flags;
  int transparency;
  int zorder;
  int guiId;
  int reserved[6];
  int on;
  GUIObject *objs[MAX_OBJS_ON_GUI];
  int objrefptr[MAX_OBJS_ON_GUI];       // for re-building objs array
  short drawOrder[MAX_OBJS_ON_GUI];

  static char oNameBuffer[20];

  GUIMain();
  void init();
  const char *get_objscript_name(const char *basedOn);
  void rebuild_array();
  void resort_zorder();
  int  get_control_type(int);
  int  is_mouse_on_gui();
  void draw_blob(int xp, int yp);
  void draw_at(int xx, int yy);
  void draw();
  int  find_object_under_mouse();
  // this version allows some extra leeway in the Editor so that
  // the user can grab tiny controls
  int  find_object_under_mouse(int);
  int  find_object_under_mouse(int leeway, bool mustBeClickable);
  void poll();
  void mouse_but_down();
  void mouse_but_up();
  int  is_textwindow();
  bool send_to_back(int objNum);
  bool bring_to_front(int objNum);
  void control_positions_changed();
  bool is_alpha();

  void FixupGuiName(char* name)
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

  void SetTransparencyAsPercentage(int percent)
  {
    // convert from % transparent to Opacity from 0-255
    if (percent == 0)
      this->transparency = 0;
    else if (percent == 100)
      this->transparency = 255;
    else
      this->transparency = ((100 - percent) * 25) / 10;
  }
  
  void ReadFromFile(FILE *fp, int version)
  {
    // read/write everything except drawOrder since
    // it will be regenerated
    fread(vtext, sizeof(char), 40, fp);
    fread(&x, sizeof(int), 27, fp);

    // 64 bit fix: Read 4 byte int values into array of 8 byte long ints
    int i;
    for (i = 0; i < MAX_OBJS_ON_GUI; i++)
      objs[i] = (GUIObject*)getw(fp);

    fread(&objrefptr, sizeof(int), MAX_OBJS_ON_GUI, fp);
  }

  void WriteToFile(FILE *fp)
  {
    fwrite(vtext, sizeof(char), 40, fp);
    fwrite(&x, sizeof(int), 27, fp);

    // 64 bit fix: Write 4 byte int values from array of 8 byte long ints
    int i;
    for (i = 0; i < MAX_OBJS_ON_GUI; i++)
      fwrite(&objs[i], 4, 1, fp);

    fwrite(&objrefptr, sizeof(int), MAX_OBJS_ON_GUI, fp);
  }

};

#define GUIDIS_GREYOUT   1
#define GUIDIS_BLACKOUT  2
#define GUIDIS_UNCHANGED 4
#define GUIDIS_GUIOFF  0x80

extern int guis_need_update;
extern int all_buttons_disabled, gui_inv_pic;
extern int gui_disabled_style;

extern void read_gui(FILE * iii, GUIMain * guiread, GameSetupStruct * gss, GUIMain** allocate = NULL);
extern void write_gui(FILE * ooo, GUIMain * guiwrite, GameSetupStruct * gss);

extern char lines[][200];
extern int numlines;
extern void removeBackslashBracket(char *lbuffer);
extern void split_lines_leftright(const char *todis, int wii, int fonnt);

#endif // __ACGUI_H
