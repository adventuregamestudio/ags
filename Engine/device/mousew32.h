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
//
// MOUSELIBW32.CPP
//
// Library of mouse functions for graphics and text mode
//
// (c) 1994 Chris Jones
// Win32 (allegro) update (c) 1999 Chris Jones
//
//=============================================================================

#define MAXCURSORS 20

namespace AGS { namespace Common { class Bitmap; } }
using namespace AGS; // FIXME later

struct IMouseGetPosCallback;

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
