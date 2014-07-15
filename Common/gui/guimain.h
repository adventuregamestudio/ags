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

#ifndef __AC_GUIMAIN_H
#define __AC_GUIMAIN_H

#include "gui/guiobject.h"
#include "ac/common_defines.h"       // AGS_INLINE

// Forward declaration
namespace AGS { namespace Common { class Stream; } }
using namespace AGS; // FIXME later

// There were issues when including header caused conflicts
struct GameSetupStruct;

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
  int padding;                  // padding surrounding a GUI text window
  int reserved[5];
  int on;
  GUIObject *objs[MAX_OBJS_ON_GUI];
  int objrefptr[MAX_OBJS_ON_GUI];       // for re-building objs array
  short drawOrder[MAX_OBJS_ON_GUI];

  static char oNameBuffer[20];

  GUIMain();
  void init();
  void rebuild_array();
  void resort_zorder();
  int  get_control_type(int);
  int  is_mouse_on_gui();
  void draw_blob(Common::Bitmap *ds, int xp, int yp, color_t draw_color);
  void draw_at(Common::Bitmap *ds, int xx, int yy);
  void draw(Common::Bitmap *ds);
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

  void FixupGuiName(char* name);
  void SetTransparencyAsPercentage(int percent);
  void ReadFromFile(Common::Stream *in, GuiVersion gui_version);
  void WriteToFile(Common::Stream *out);

};

extern GuiVersion GameGuiVersion;
extern int guis_need_update;
extern int all_buttons_disabled, gui_inv_pic;
extern int gui_disabled_style;
extern char lines[MAXLINE][200];
extern int  numlines;

extern void read_gui(Common::Stream *in, GUIMain * guiread, GameSetupStruct * gss, GUIMain** allocate = NULL);
extern void write_gui(Common::Stream *out, GUIMain * guiwrite, GameSetupStruct * gss, bool savedgame);

extern int mousex, mousey;

extern int get_adjusted_spritewidth(int spr);
extern int get_adjusted_spriteheight(int spr);
extern bool is_sprite_alpha(int spr);
extern int final_col_dep;

// This function has distinct implementations in Engine and Editor
extern void draw_gui_sprite(Common::Bitmap *ds, int spr, int x, int y, bool use_alpha);

extern AGS_INLINE int divide_down_coordinate(int coord);
extern AGS_INLINE int multiply_up_coordinate(int coord);
extern AGS_INLINE void multiply_up_coordinates(int *x, int *y);
extern AGS_INLINE int get_fixed_pixel_size(int pixels);

// Those function have distinct implementations in Engine and Editor
extern void wouttext_outline(Common::Bitmap *ds, int xxp, int yyp, int usingfont, color_t text_color, char *texx);
extern int wgettextwidth_compensate(Common::Bitmap *ds, const char *tex, int font) ;
extern void check_font(int *fontnum);

extern void set_our_eip(int eip);
#define SET_EIP(x) set_our_eip(x);
extern void set_eip_guiobj(int eip);
extern int get_eip_guiobj();

extern bool outlineGuiObjects;

#endif // __AC_GUIMAIN_H
