#ifndef __AC_CSCIDIALOG_H
#define __AC_CSCIDIALOG_H

#include "acdialog/ac_dialogdefines.h"

//
// Most of these are defined in ac.cpp;
// A huge split is needed before these could be put into separate headers
//

extern int load_game(int,char*, int*);
extern void break_up_text_into_lines(int wii,int fonnt,char*todis);
extern char lines[][200];
extern int numlines;
extern void wouttext_outline(int xxp,int yyp,int usingfont,char*texx);
extern IGraphicsDriver *gfxDriver;
extern inline int get_fixed_pixel_size(int pixels);

extern void quit(char *);
extern int mousex, mousey;
extern void rec_domouse(int);
extern int rec_misbuttondown(int);
extern int rec_mgetbutton();
extern void next_iteration();
extern void render_graphics(IDriverDependantBitmap *extraBitmap, int extraX, int extraY);
extern void update_polled_stuff_if_runtime();
extern char *get_language_text(int);
extern int sgsiglen;
extern void update_polled_stuff_and_crossfade();
extern char *get_global_message(int);
extern int GetBaseWidth();
extern GameSetup usetup;
extern char ignore_bounds;      // Ignore mouse bounding rectangle while in dialog
extern volatile int timerloop;

extern char saveGameSuffix[21];

//
// These are defined in ac_cscidialog
//

const int NONE = -1, LEFT = 0, RIGHT = 1;

void refresh_screen();

extern int windowbackgroundcolor, pushbuttondarkcolor;
extern int pushbuttonlightcolor;
extern int topwindowhandle;
extern int cbuttfont;
extern int acdialog_font;

extern int smcode;

#endif // __AC_CSCIDIALOG_H