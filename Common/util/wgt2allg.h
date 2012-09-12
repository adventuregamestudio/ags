/*
** WGT -> Allegro portability interface
** Copyright (C) 1998, Chris Jones
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE;
** the contents of this file may not be disclosed to third parties,
** copied or duplicated in any form, in whole or in part, without
** prior express permission from Chris Jones.
**
** wsavesprites and wloadsprites are hi-color compliant
*/
#define _WGT45_

#ifndef __WGT4_H
#define __WGT4_H

#ifdef _MSC_VER
#ifndef WINDOWS_VERSION
#define WINDOWS_VERSION
#endif
#endif

#include <stdio.h>
#include <string.h>

#ifdef USE_ALLEGRO3
#include <allegro3.h>
#else
#include "allegro.h"
#endif

#if !defined(LINUX_VERSION) && !defined(MAC_VERSION)
#include <dos.h>
#include <io.h>
#endif

#include <stdarg.h>

#ifdef WINDOWS_VERSION
#include "winalleg.h"
#elif defined(MAC_VERSION) && !defined(IOS_VERSION)
#include <osxalleg.h>
#endif

namespace AGS { namespace Common { class Bitmap; }}
using namespace AGS; // FIXME later


#if defined WGT2ALLEGRO_NOFUNCTIONS
#error WGT2ALLEGRO_NOFUNCTIONS macro is obsolete and should not be defined anymore.
#endif

#if (WGTMAP_SIZE == 1)
typedef unsigned char *wgtmap;
#else
typedef short *wgtmap;
#endif

#define color RGB
#define TEXTFG    0
#define TEXTBG    1

#define fpos_t unsigned long
#ifdef __cplusplus
extern "C"
{
#endif

#if defined(WINDOWS_VERSION) || defined(LINUX_VERSION) || defined(MAC_VERSION)
#include <time.h>
  struct time
  {
    int ti_hund, ti_sec, ti_min, ti_hour;
  };
#endif

#define is_ttf(fontptr)  (fontptr[0] == 'T')

  

#ifdef __cplusplus
}
#endif


//=============================================================================

#ifdef USE_CLIB
#include "util/clib32.h"
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    extern const char *wgt2allgcopyright;
    extern const char *spindexid;
    extern const char *spindexfilename;

    extern fpos_t lfpos;
    extern FILE *libf;
    extern short lresult;
    extern int lsize;
    extern char password[16];
    extern char *wgtlibrary;
    extern int currentcolor;
    extern int vesa_xres, vesa_yres;
    extern Common::Bitmap *abuf;

#ifdef WINDOWS_VERSION
#define GFX_VGA GFX_DIRECTX
#endif

#if defined(LINUX_VERSION) || defined(MAC_VERSION)
#define GFX_VGA GFX_AUTODETECT
#endif

    /*
    GCC from the Android NDK doesn't like allegro_init()

    extern void vga256();
    */

#if (!defined(WINDOWS_VERSION) && !defined(LINUX_VERSION) && !defined(MAC_VERSION))
    extern union REGS r;
    extern void wsetmode(int nnn);
    extern int wgetmode();
#endif

    extern void wsetscreen(Common::Bitmap *nss);
	// CHECKME: temporary solution for plugin system
	extern void wsetscreen_raw(BITMAP *nss);
    extern void wsetrgb(int coll, int r, int g, int b, color * pall);
    extern int wloadpalette(char *filnam, color * pall);

    extern void wcolrotate(unsigned char start, unsigned char finish, int dir, color * pall);

    extern Common::Bitmap *tempbitm;
    extern Common::Bitmap *wnewblock(int x1, int y1, int x2, int y2);

    // [IKM] recreated these in platform/file unit
    /*
    extern short getshort(FILE * fff);
    extern void putshort(short num, FILE *fff);
    */
    extern Common::Bitmap *wloadblock(char *fill);

    extern int wloadsprites(color * pall, char *filnam, Common::Bitmap ** sarray, int strt, int eend);
    extern void wfreesprites(Common::Bitmap ** blar, int stt, int end);
    /*
    extern void wsavesprites_ex(color * pll, char *fnm, Common::Bitmap ** spre, int strt, int eend, unsigned char *arry);
    extern void wsavesprites(color * pll, char *fnm, Common::Bitmap ** spre, int strt, int eend);
    */
	extern void wputblock(int xx, int yy, Common::Bitmap *bll, int xray);
	// CHECKME: temporary solution for plugin system
	extern void wputblock_raw(int xx, int yy, BITMAP *bll, int xray);

    extern const int col_lookups[32];

    extern void __my_setcolor(int *ctset, int newcol);
    extern void wsetcolor(int nval);

    extern int get_col8_lookup(int nval);


    extern int __wremap_keep_transparent;
    extern void wremap(color * pal1, Common::Bitmap *picc, color * pal2);
    extern void wremapall(color * pal1, Common::Bitmap *picc, color * pal2);

    // library file functions
    extern void readheader();

    extern void findfile(char *filnam);

    extern int checkpassword(char *passw); // 0 = incorrect   ! = 0 correct

#if defined(WINDOWS_VERSION) || defined(LINUX_VERSION) || defined(MAC_VERSION)
    extern void gettime(struct time *tpt);
#endif

    extern long wtimer(struct time tt1, struct time tt2);

    extern void wcopyscreen(int x1, int y1, int x2, int y2, Common::Bitmap *src, int dx, int dy, Common::Bitmap *dest);

#ifdef __cplusplus
}
#endif





#define tx    abuf->cl
#define ty    abuf->ct
//#define bx abuf->cr   // can't do this because of REGS.bx
#define by    abuf->cb
#define kbdon key

#define installkbd()          install_keyboard()
#define uninstallkbd()        remove_keyboard()
#define wallocblock(wii,hii)  create_bitmap(wii,hii)
#define wbar(x1, y1, x2, y2)  rectfill(abuf, x1, y1, x2, y2, currentcolor)
#define wclip(x1, y1, x2, y2) set_clip(abuf, x1, y1, x2, y2)
#define wcls(coll)            clear_to_color(abuf,coll)

#define wfade_in(from, to, speed, pal)  fade_in_range(pal, 5 /* 64 - speed * 7 */, from, to)
#define wfade_out(from, to, speed, pal) fade_out_range(5, from, to)
#define wfastputpixel(x1, y1)           _putpixel(abuf, x1, y1, currentcolor)
#define wfreeblock(bll)                 destroy_bitmap(bll)
#define wgetblockheight(bll)            bll->GetHeight()
#define wgetblockwidth(bll)             bll->GetWidth()
#define wgetpixel(xx, yy)               getpixel(abuf, xx, yy)
#define whline(x1, x2, yy)              hline(abuf, x1, yy, x2, currentcolor)
#define wline(x1, y1, x2, y2)           line(abuf,x1,y1,x2,y2,currentcolor)
#define wloadpcx256(fnm,pall)           load_pcx( fnm, pall)
#define wnormscreen()                   abuf = screen
#define wputpixel(x1, y1)               putpixel(abuf, x1, y1, currentcolor)
#define wreadpalette(from, to, dd)      get_palette_range(dd, from, to)
#define wrectangle(x1, y1, x2, y2)      rect(abuf, x1, y1, x2, y2, currentcolor)
#define wregionfill(xx, yy)             floodfill(abuf, xx, yy, currentcolor)
#define wretrace()                      vsync()
#define setlib(lll)                     csetlib(lll, "")
#define wsetpalette(from, to, pall)     set_palette_range(pall, from, to, 0)
#define vgadetected()                   1

// now define the wvesa_xxx to the normal names, since we use SVGA normally
#define wvesa_bar       wbar
#define wvesa_clip      wclip
#define wvesa_cls       wcls
#define wvesa_outtextxy wouttextxy
#define wvesa_rectangle wrectangle

#define XRAY    1
#define NORMAL  0

struct IMouseGetPosCallback {
public:
    virtual void AdjustPosition(int *x, int *y) = 0;
};

// Font/text rendering
extern void init_font_renderer();
extern void shutdown_font_renderer();
extern bool wloadfont_size(int fontNumber, int fontSize);
extern void wfreefont(int fontNumber);
extern void wouttextxy(int, int, int fontNumber, const char *);
extern void wgtprintf(int, int, int fontNumber, char *, ...);
extern int wgettextheight(const char *, int fontNumber);
extern int wgettextwidth(const char *, int fontNumber);
extern void wtextcolor(int);
extern int textcol;
extern int wtext_multiply;
extern void ensure_text_valid_for_font(char *text, int fontnum);
extern void adjust_y_coordinate_for_text(int* ypos, int fontnum);
extern void wtexttransparent(int);
/* Temp code to test for memory leaks
#define destroy_bitmap my_destroy_bitmap
#undef wfreeblock
#define wfreeblock my_destroy_bitmap
extern void my_destroy_bitmap(BITMAP *bitmap);
*/

#ifdef __cplusplus
extern "C"
{
#endif

    extern void wbutt(int x1, int y1, int x2, int y2);

#ifdef __cplusplus
}
#endif


// archive attributes to search for - al_findfirst breaks with 0
#define FA_SEARCH -1


#endif
