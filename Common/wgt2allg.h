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
#elif defined(MAC_VERSION)
#include <Allegro/osxalleg.h>
#elif defined(LINUX_VERSION)
#include "linalleg.h"
#endif

#include "bigend.h"

typedef BITMAP *block;

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

#ifndef WGT2ALLEGRO_NOFUNCTIONS
  const char *wgt2allgcopyright = "WGT2Allegro (c) 1997,1998 Chris Jones";
  const char *spindexid = "SPRINDEX";
  const char *spindexfilename = "sprindex.dat";
  fpos_t lfpos;
  FILE *libf;
  short lresult;
  int lsize;
  char password[16];
  char *wgtlibrary;
  int currentcolor;
  int vesa_xres, vesa_yres;
  block abuf;

#ifdef WINDOWS_VERSION
#define GFX_VGA GFX_DIRECTX
#endif

#if defined(LINUX_VERSION) || defined(MAC_VERSION)
#define GFX_VGA GFX_AUTODETECT
#endif

  void vga256()
  {
    allegro_init();
    set_gfx_mode(GFX_VGA, 320, 200, 320, 200);
    abuf = screen;
    vesa_xres = 320;
    vesa_yres = 200;
  }

#if (!defined(WINDOWS_VERSION) && !defined(LINUX_VERSION) && !defined(MAC_VERSION))
  union REGS r;
  void wsetmode(int nnn)
  {
    r.x.ax = nnn;
    int86(0x10, &r, &r);
  }

  int wgetmode()
  {
    r.x.ax = 0x0f00;
    int86(0x10, &r, &r);
    return r.h.al;
  }
#endif

#ifdef USE_CLIB
  extern "C"
  {
    extern FILE *clibfopen(char *, char *);
    extern long cliboffset(char *);
    extern long clibfilesize(char *);
    extern long last_opened_size;
  }
#define fopen clibfopen
#endif

  void wsetscreen(block nss)
  {
    if (nss == NULL)
      abuf = screen;
    else
      abuf = nss;
  }

  void wsetrgb(int coll, int r, int g, int b, color * pall)
  {
    pall[coll].r = r;
    pall[coll].g = g;
    pall[coll].b = b;
  }

  int wloadpalette(char *filnam, color * pall)
  {
    FILE *fff;
    int kk;

    fff = fopen(filnam, "rb");
    if (fff == NULL)
      return -1;

    for (kk = 0; kk < 256; kk++)        // there's a filler byte
      fread(&pall[kk], 3, 1, fff);

    fclose(fff);
    return 0;
  }

  void wcolrotate(unsigned char start, unsigned char finish, int dir, color * pall)
  {
    int jj;
    if (dir == 0) {
      // rotate left
      color tempp = pall[start];

      for (jj = start; jj < finish; jj++)
        pall[jj] = pall[jj + 1];

      pall[finish] = tempp;
    }
    else {
      // rotate right
      color tempp = pall[finish];

      for (jj = finish - 1; jj >= start; jj--)
        pall[jj + 1] = pall[jj];

      pall[start] = tempp;
    }
  }

  block tempbitm;
  block wnewblock(int x1, int y1, int x2, int y2)
  {
    int twid = (x2 - x1) + 1, thit = (y2 - y1) + 1;

    if (twid < 1)
      twid = 1;

    if (thit < 1)
      thit = 1;

    tempbitm = create_bitmap(twid, thit);

    if (tempbitm == NULL)
      return NULL;

    blit(abuf, tempbitm, x1, y1, 0, 0, tempbitm->w, tempbitm->h);
    return tempbitm;
  }

  short getshort(FILE * fff)
  {
    short sss;
    fread(&sss, 2, 1, fff);
    return sss;
  }

  void putshort(short num, FILE *fff)
  {
    fwrite(&num, 2, 1, fff);
  }

  block wloadblock(char *fill)
  {
    short widd, hitt;
    FILE *fff = fopen(fill, "rb");
    int ff;

    if (fff == NULL)
      return NULL;

    widd = getshort(fff);
    hitt = getshort(fff);
    tempbitm = create_bitmap(widd, hitt);

    for (ff = 0; ff < hitt; ff++)
      fread(&tempbitm->line[ff][0], widd, 1, fff);

    fclose(fff);
    return tempbitm;
  }

  int wloadsprites(color * pall, char *filnam, block * sarray, int strt, int eend)
  {
    FILE *ff;
    int vers;
    char buff[20];
    int numspri = 0, vv, hh, wdd, htt;

    ff = fopen(filnam, "rb");
    if (ff == NULL)
      return -1;

    vers = getshort(ff);
    fread(&buff[0], 13, 1, ff);
    for (vv = 0; vv < 256; vv++)        // there's a filler byte
      fread(&pall[vv], 3, 1, ff);

    if (vers > 4)
      return -1;

    if (vers == 4)
      numspri = getshort(ff);
    else {
      numspri = getshort(ff);
      if ((numspri < 2) || (numspri > 200))
        numspri = 200;
    }

    for (vv = strt; vv <= eend; vv++)
      sarray[vv] = NULL;

    for (vv = 0; vv <= numspri; vv++) {
      int coldep = getshort(ff);

      if (coldep == 0) {
        sarray[vv] = NULL;
        if (feof(ff))
          break;

        continue;
      }

      if (feof(ff))
        break;

      if (vv > eend)
        break;

      wdd = getshort(ff);
      htt = getshort(ff);
      if (vv < strt) {
        fseek(ff, wdd * htt, SEEK_CUR);
        continue;
      }
      sarray[vv] = create_bitmap_ex(coldep * 8, wdd, htt);

      if (sarray[vv] == NULL) {
        fclose(ff);
        return -1;
      }

      for (hh = 0; hh < htt; hh++)
        fread(&sarray[vv]->line[hh][0], wdd * coldep, 1, ff);
    }
    fclose(ff);
    return 0;
  }

  void wfreesprites(block * blar, int stt, int end)
  {
    int hh;

    for (hh = stt; hh <= end; hh++) {
      if (blar[hh] != NULL)
        destroy_bitmap(blar[hh]);

      blar[hh] = NULL;
    }
  }
/*
  void wsavesprites_ex(color * pll, char *fnm, block * spre, int strt, int eend, unsigned char *arry)
  {
    FILE *ooo = fopen(fnm, "wb");
    short topu = 4;
    int aa, lastsp = 0;
    char *spsig = " Sprite File ";

    fwrite(&topu, 2, 1, ooo);
    fwrite(spsig, 13, 1, ooo);

    for (aa = 0; aa < 256; aa++)
      fwrite(&pll[aa], 3, 1, ooo);

    for (aa = strt; aa <= eend; aa++) {
      if (spre[aa] != NULL)
        lastsp = aa;
    }

    if (lastsp > 32000) {
      // don't overflow the short
      lastsp = 32000;
    }

    topu = lastsp;
    fwrite(&topu, 2, 1, ooo);

    // allocate buffers to store the indexing info
    int numsprits = (lastsp - strt) + 1;
    short *spritewidths = (short*)malloc(numsprits * sizeof(short));
    short *spriteheights = (short*)malloc(numsprits * sizeof(short));
    long *spriteoffs = (long*)malloc(numsprits * sizeof(long));
    int spidx;

    for (aa = strt; aa <= lastsp; aa++) {
      short bpss = 1;
      spidx = aa - strt;

      if (spre[aa] == NULL) {
        topu = 0;
        fwrite(&topu, 2, 1, ooo);
        // sprite does not exist, zero out its entry
        spritewidths[spidx] = 0;
        spriteheights[spidx] = 0;
        spriteoffs[spidx] = 0;
        continue;
      }

      spritewidths[spidx] = spre[aa]->w;
      spriteheights[spidx] = spre[aa]->h;
      spriteoffs[spidx] = ftell(ooo);

      bpss = bitmap_color_depth(spre[aa]) / 8;
      fwrite(&bpss, 2, 1, ooo);

      topu = spre[aa]->w;
      fwrite(&topu, 2, 1, ooo);

      topu = spre[aa]->h;
      fwrite(&topu, 2, 1, ooo);

      fwrite(&spre[aa]->line[0][0], spre[aa]->w * bpss, spre[aa]->h, ooo);
    }
    fclose(ooo);

    // write the sprite index file
    ooo = fopen((char*)spindexfilename, "wb");
    // write "SPRINDEX" id
    fwrite(&spindexid[0], strlen(spindexid), 1, ooo);
    // write version (1)
    putw(1, ooo);
    // write last sprite number and num sprites, to verify that
    // it matches the spr file
    putw(lastsp, ooo);
    putw(numsprits, ooo);
    fwrite(&spritewidths[0], sizeof(short), numsprits, ooo);
    fwrite(&spriteheights[0], sizeof(short), numsprits, ooo);
    fwrite(&spriteoffs[0], sizeof(long), numsprits, ooo);
    fclose(ooo);

    free(spritewidths);
    free(spriteheights);
    free(spriteoffs);
  }

  void wsavesprites(color * pll, char *fnm, block * spre, int strt, int eend)
  {
    wsavesprites_ex(pll, fnm, spre, strt, eend, NULL);
  }
*/
  void wputblock(int xx, int yy, block bll, int xray)
  {
    if (xray)
      draw_sprite(abuf, bll, xx, yy);
    else
      blit(bll, abuf, 0, 0, xx, yy, bll->w, bll->h);
  }

  const int col_lookups[32] = {
    0x000000, 0x0000A0, 0x00A000, 0x00A0A0, 0xA00000,   // 4
    0xA000A0, 0xA05000, 0xA0A0A0, 0x505050, 0x5050FF, 0x50FF50, 0x50FFFF,       // 11
    0xFF5050, 0xFF50FF, 0xFFFF50, 0xFFFFFF, 0x000000, 0x101010, 0x202020,       // 18
    0x303030, 0x404040, 0x505050, 0x606060, 0x707070, 0x808080, 0x909090,       // 25
    0xA0A0A0, 0xB0B0B0, 0xC0C0C0, 0xD0D0D0, 0xE0E0E0, 0xF0F0F0
  };

  void __my_setcolor(int *ctset, int newcol)
  {
    int wantColDep = bitmap_color_depth(abuf);
    if (wantColDep == 8)
      ctset[0] = newcol;
    else if (newcol & 0x40000000) // already calculated it
      ctset[0] = newcol;
    else if ((newcol >= 32) && (wantColDep > 16)) {
      // true-color
#ifdef SWAP_RB_HICOL
      ctset[0] = makeacol32(getb16(newcol), getg16(newcol), getr16(newcol), 255);
#else
      ctset[0] = makeacol32(getr16(newcol), getg16(newcol), getb16(newcol), 255);
#endif
    }
    else if (newcol >= 32) {
#ifdef SWAP_RB_HICOL
      ctset[0] = makecol16(getb16(newcol), getg16(newcol), getr16(newcol));
#else
      // If it's 15-bit, convert the color
      if (wantColDep == 15)
        ctset[0] = (newcol & 0x001f) | ((newcol >> 1) & 0x7fe0);
      else
        ctset[0] = newcol;
#endif
    } 
    else
    {
      ctset[0] = makecol_depth(wantColDep, col_lookups[newcol] >> 16,
                               (col_lookups[newcol] >> 8) & 0x000ff, col_lookups[newcol] & 0x000ff);

      // in case it's used on an alpha-channel sprite, make sure it's visible
      if (wantColDep > 16)
        ctset[0] |= 0xff000000;
    }

    // if it's 32-bit color, signify that the colour has been calculated
    //if (wantColDep >= 24)
//      ctset[0] |= 0x40000000;
  }

  void wsetcolor(int nval)
  {
    __my_setcolor(&currentcolor, nval);
  }

  int get_col8_lookup(int nval)
  {
    int tmpv;
    __my_setcolor(&tmpv, nval);
    return tmpv;
  }

  int __wremap_keep_transparent = 1;
  void wremap(color * pal1, block picc, color * pal2)
  {
    int jj;
    unsigned char color_mapped_table[256];

    for (jj = 0; jj < 256; jj++)
    {
      if ((pal1[jj].r == 0) && (pal1[jj].g == 0) && (pal1[jj].b == 0))
      {
        color_mapped_table[jj] = 0;
      }
      else
      {
        color_mapped_table[jj] = bestfit_color(pal2, pal1[jj].r, pal1[jj].g, pal1[jj].b);
      }
    }

    if (__wremap_keep_transparent > 0) {
      // keep transparency
      color_mapped_table[0] = 0;
      // any other pixels which are being mapped to 0, map to 16 instead
      for (jj = 1; jj < 256; jj++) {
        if (color_mapped_table[jj] == 0)
          color_mapped_table[jj] = 16;
      }
    }

    for (jj = 0; jj < (picc->w) * (picc->h); jj++) {
      int xxl = jj % (picc->w), yyl = jj / (picc->w);
      int rr = getpixel(picc, xxl, yyl);
      putpixel(picc, xxl, yyl, color_mapped_table[rr]);
    }
  }

  void wremapall(color * pal1, block picc, color * pal2)
  {
    __wremap_keep_transparent--;
    wremap(pal1, picc, pal2);
    __wremap_keep_transparent++;
  }

  // library file functions
  void readheader()
  {
  }

  void findfile(char *filnam)
  {
    // should set lfpos = offset of file and set lresult = 1
    // or set lresult = 0 on failure (and exit with message)
  }

  int checkpassword(char *passw)
  {
    return 0;                   // 0 = incorrect   ! = 0 correct
  }

#if defined(WINDOWS_VERSION) || defined(LINUX_VERSION) || defined(MAC_VERSION)
  void gettime(struct time *tpt)
  {
    time_t ltt;
    struct tm *tis;

    time(&ltt);
    tis = localtime(&ltt);
    tpt->ti_sec = tis->tm_sec;
    tpt->ti_min = tis->tm_min;
    tpt->ti_hour = tis->tm_hour;
    tpt->ti_hund = 0;
  }
#endif

  long wtimer(struct time tt1, struct time tt2)
  {
    long timm1 = tt1.ti_hund + (long)tt1.ti_sec * 100 + (long)tt1.ti_min * 6000 + (long)tt1.ti_hour * 6000 * 60;

    long timm2 = tt2.ti_hund + (long)tt2.ti_sec * 100 + (long)tt2.ti_min * 6000 + (long)tt2.ti_hour * 6000 * 60;

    return timm2 - timm1;
  }

  void wcopyscreen(int x1, int y1, int x2, int y2, block src, int dx, int dy, block dest)
  {
    if (src == NULL)
      src = screen;

    if (dest == NULL)
      dest = screen;

    blit(src, dest, x1, y1, dx, dy, (x2 - x1) + 1, (y2 - y1) + 1);
  }

#ifdef USE_CLIB
#undef fopen
#endif
#else

  extern const char *spindexid;
  extern const char *spindexfilename ;

  extern fpos_t lfpos;
  extern FILE *libf;
  extern short lresult;
  extern int lsize;
  extern char password[16];
  extern char *wgtlibrary;

  extern void readheader();
  extern void findfile(char *);
  extern int checkpassword(char *);

  extern int currentcolor;
  extern block abuf;
  extern int vesa_xres, vesa_yres;

  extern short getshort(FILE * fff);
  extern void putshort(short num, FILE *fff);

  extern void vga256();
  extern void wbutt(int, int, int, int);
  extern void wsetmode(int);
  extern int wgetmode();
  extern void wsetscreen(block);
  extern void wnormscreen();
  extern void wcopyscreen(int, int, int, int, block, int, int, block);
  extern void wsetrgb(int, int, int, int, color *);
  extern int wloadpalette(char *, color *);
  extern void wcolrotate(unsigned char, unsigned char, int, color *);
  extern block wnewblock(int, int, int, int);
  extern void wfreesprites(block *, int, int);
  extern block wloadblock(char *);
  extern int wloadsprites(color *, char *, block *, int, int);
  extern void wputblock(int, int, block, int);
  extern void wremap(color *, block, color *);
  extern void wsetcolor(int);
  extern long wtimer(struct time, struct time);
  extern void gettime(struct time *);
  extern void __my_setcolor(int *ctset, int newcol);
  extern int get_col8_lookup(int nval);

#endif
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
#define wgetblockheight(bll)            bll->h
#define wgetblockwidth(bll)             bll->w
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

#ifndef WGT2ALLEGRO_NOFUNCTIONS
#ifdef __cplusplus
extern "C"
{
#endif
  void wbutt(int x1, int y1, int x2, int y2)
  {
    wsetcolor(254);
    wbar(x1, y1, x2, y2);
    wsetcolor(253);
    whline(x1 - 1, x2 + 1, y1 - 1);
    wline(x1 - 1, y1 - 1, x1 - 1, y2 + 1);
    wsetcolor(255);
    whline(x1 - 1, x2 + 1, y2 + 1);
    wline(x2 + 1, y1 - 1, x2 + 1, y2 + 1);
  }
#ifdef __cplusplus
}
#endif
#endif

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
#endif
