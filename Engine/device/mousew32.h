/* MOUSELIBW32.CPP

    Library of mouse functions for graphics and text mode

    (c) 1994 Chris Jones

    Win32 (allegro) update (c) 1999 Chris Jones

    This is UNPUBLISHED PROPRIETARY SOURCE CODE;
    the contents of this file may not be disclosed to third parties,
    copied or duplicated in any form, in whole or in part, without
    prior express permission from Chris Jones.
*/

#define MAXCURSORS 20


void msetgraphpos(int,int);
void msetcallback(IMouseGetPosCallback *gpCallback);
void mgraphconfine(int x1, int y1, int x2, int y2);
void mgetgraphpos();
void msetcursorlimit(int x1, int y1, int x2, int y2);
void drawCursor();
void domouse(int str);
int ismouseinbox(int lf, int tp, int rt, int bt);
void mfreemem();
void mnewcursor(char cursno);
void mloadwcursor(char *namm);
int mgetbutton();
int misbuttondown(int buno);
void msetgraphpos(int xa, int ya);
void msethotspot(int xx, int yy);
int minstalled();


extern int mousex, mousey;
extern int hotx, hoty;
extern int disable_mgetgraphpos;
extern char currentcursor;

extern Common::Bitmap *mousecurs[MAXCURSORS];
