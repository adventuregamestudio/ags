
#define USE_CLIB
#include "util/wgt2allg.h"
#include "util/file.h"
#include "util/datastream.h"
#include "gfx/bitmap.h"
#include "gfx/allegrobitmap.h"

using AGS::Common::IBitmap;
using AGS::Common::CAllegroBitmap;
namespace Bitmap = AGS::Common::Bitmap;

using AGS::Common::CDataStream;

#define fopen clibfopen+++do_not_use!!!

#ifdef __cplusplus
extern "C"
{
#endif


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
IBitmap *abuf;

/*
  GCC from the Android NDK doesn't like allegro_init()

  void vga256()
  {
    allegro_init();
    set_gfx_mode(GFX_VGA, 320, 200, 320, 200);
    abuf = screen;
    vesa_xres = 320;
    vesa_yres = 200;
  }
  */

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


  void wsetscreen(IBitmap *nss)
  {
    if (nss == NULL)
      abuf = Bitmap::GetScreenBitmap();
    else
      abuf = nss;
  }

  // [IKM] A very, very dangerous stuff!
  CAllegroBitmap wsetscreen_wrapper;
  void wsetscreen_raw(BITMAP *nss)
  {
    wsetscreen_wrapper.WrapBitmapObject(nss);

    if (nss == NULL) {
      abuf = Bitmap::GetScreenBitmap();
	}
	else {
      abuf = &wsetscreen_wrapper;
	}
  }

  void wsetrgb(int coll, int r, int g, int b, color * pall)
  {
    pall[coll].r = r;
    pall[coll].g = g;
    pall[coll].b = b;
  }

  int wloadpalette(char *filnam, color * pall)
  {
    int kk;

    CDataStream *in = clibfopen(filnam);
    if (in == NULL)
      return -1;

    for (kk = 0; kk < 256; kk++)        // there's a filler byte
      in->ReadArray(&pall[kk], 3, 1);

    delete in;
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

  IBitmap *tempbitm;

  IBitmap *wnewblock(int x1, int y1, int x2, int y2)
  {
    int twid = (x2 - x1) + 1, thit = (y2 - y1) + 1;

    if (twid < 1)
      twid = 1;

    if (thit < 1)
      thit = 1;

    tempbitm = Bitmap::CreateBitmap(twid, thit);

    if (tempbitm == NULL)
      return NULL;

    tempbitm->Blit(abuf, x1, y1, 0, 0, tempbitm->GetWidth(), tempbitm->GetHeight());
    return tempbitm;
  }

  // [IKM] recreated these in platform/file unit
  /*
  short getshort(FILE * fff)
  {
    short sss;
    in->ReadArray(&sss, 2, 1, fff);
    return sss;
  }

  void putshort(short num, FILE *fff)
  {
    ->WriteArray(&num, 2, 1, fff);
  }
  */

  IBitmap *wloadblock(char *fill)
  {
    short widd, hitt;
    CDataStream *in = clibfopen(fill);
    int ff;

    if (in == NULL)
      return NULL;

    widd = in->ReadInt16();
    hitt = in->ReadInt16();
    tempbitm = Bitmap::CreateBitmap(widd, hitt);

    for (ff = 0; ff < hitt; ff++)
      in->ReadArray(&tempbitm->GetScanLineForWriting(ff)[0], widd, 1);

    delete in;
    return tempbitm;
  }

  int wloadsprites(color * pall, char *filnam, IBitmap ** sarray, int strt, int eend)
  {
    int vers;
    char buff[20];
    int numspri = 0, vv, hh, wdd, htt;

    CDataStream *in = clibfopen(filnam);
    if (in == NULL)
      return -1;

    vers = in->ReadInt16();
    in->ReadArray(&buff[0], 13, 1);
    for (vv = 0; vv < 256; vv++)        // there's a filler byte
      in->ReadArray(&pall[vv], 3, 1);

    if (vers > 4)
      return -1;

    if (vers == 4)
      numspri = in->ReadInt16();
    else {
      numspri = in->ReadInt16();
      if ((numspri < 2) || (numspri > 200))
        numspri = 200;
    }

    for (vv = strt; vv <= eend; vv++)
      sarray[vv] = NULL;

    for (vv = 0; vv <= numspri; vv++) {
      int coldep = in->ReadInt16();

      if (coldep == 0) {
        sarray[vv] = NULL;
        if (in->EOS())
          break;

        continue;
      }

      if (in->EOS())
        break;

      if (vv > eend)
        break;

      wdd = in->ReadInt16();
      htt = in->ReadInt16();
      if (vv < strt) {
          in->Seek(Common::kSeekCurrent, wdd * htt);
        continue;
      }
      sarray[vv] = Bitmap::CreateBitmap(wdd, htt, coldep * 8);

      if (sarray[vv] == NULL) {
        delete in;
        return -1;
      }

      for (hh = 0; hh < htt; hh++)
        in->ReadArray(&sarray[vv]->GetScanLineForWriting(hh)[0], wdd * coldep, 1);
    }
    delete in;
    return 0;
  }


  void wfreesprites(IBitmap ** blar, int stt, int end)
  {
    int hh;

    for (hh = stt; hh <= end; hh++) {
      delete blar[hh];
      blar[hh] = NULL;
    }
  }


  /*
  void wsavesprites_ex(color * pll, char *fnm, IBitmap ** spre, int strt, int eend, unsigned char *arry)
  {
    FILE *ooo = fopen(fnm, "wb");
    short topu = 4;
    int aa, lastsp = 0;
    char *spsig = " Sprite File ";

    ->WriteArray(&topu, 2, 1, ooo);
    ->WriteArray(spsig, 13, 1, ooo);

    for (aa = 0; aa < 256; aa++)
      ->WriteArray(&pll[aa], 3, 1, ooo);

    for (aa = strt; aa <= eend; aa++) {
      if (spre[aa] != NULL)
        lastsp = aa;
    }

    if (lastsp > 32000) {
      // don't overflow the short
      lastsp = 32000;
    }

    topu = lastsp;
    ->WriteArray(&topu, 2, 1, ooo);

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
        ->WriteArray(&topu, 2, 1, ooo);
        // sprite does not exist, zero out its entry
        spritewidths[spidx] = 0;
        spriteheights[spidx] = 0;
        spriteoffs[spidx] = 0;
        continue;
      }

      spritewidths[spidx] = spre[aa]->GetWidth();
      spriteheights[spidx] = spre[aa]->GetHeight();
      spriteoffs[spidx] = GetPosition(ooo);

      bpss = ->GetColorDepth(spre[aa]) / 8;
      ->WriteArray(&bpss, 2, 1, ooo);

      topu = spre[aa]->GetWidth();
      ->WriteArray(&topu, 2, 1, ooo);

      topu = spre[aa]->GetHeight();
      ->WriteArray(&topu, 2, 1, ooo);

      ->WriteArray(&spre[aa]->line[0][0], spre[aa]->w * bpss, spre[aa]->h, ooo);
    }
    fclose(ooo);

    // write the sprite index file
    ooo = fopen((char*)spindexfilename, "wb");
    // write "SPRINDEX" id
    ->WriteArray(&spindexid[0], strlen(spindexid), 1, ooo);
    // write version (1)
    ->WriteInt32(1, ooo);
    // write last sprite number and num sprites, to verify that
    // it matches the spr file
    ->WriteInt32(lastsp, ooo);
    ->WriteInt32(numsprits, ooo);
    ->WriteArray(&spritewidths[0], sizeof(short), numsprits, ooo);
    ->WriteArray(&spriteheights[0], sizeof(short), numsprits, ooo);
    ->WriteArray(&spriteoffs[0], sizeof(long), numsprits, ooo);
    fclose(ooo);

    free(spritewidths);
    free(spriteheights);
    free(spriteoffs);
  }

  void wsavesprites(color * pll, char *fnm, IBitmap ** spre, int strt, int eend)
  {
    wsavesprites_ex(pll, fnm, spre, strt, eend, NULL);
  }
*/

  void wputblock(int xx, int yy, IBitmap *bll, int xray)
  {
    if (xray)
		abuf->Blit(bll, xx, yy, Common::kBitmap_Transparency);
    else
      abuf->Blit(bll, 0, 0, xx, yy, bll->GetWidth(), bll->GetHeight());
  }

  CAllegroBitmap wputblock_wrapper; // [IKM] argh! :[
  void wputblock_raw(int xx, int yy, BITMAP *bll, int xray)
  {
	wputblock_wrapper.WrapBitmapObject(bll);
    if (xray)
      abuf->Blit(&wputblock_wrapper, xx, yy, Common::kBitmap_Transparency);
    else
      abuf->Blit(&wputblock_wrapper, 0, 0, xx, yy, wputblock_wrapper.GetWidth(), wputblock_wrapper.GetHeight());
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
    int wantColDep = abuf->GetColorDepth();
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
#if defined(PSP_VERSION)
// PSP: Swap colors only here.
#define SWAP_RB_HICOL
#endif
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

  void wremap(color * pal1, IBitmap *picc, color * pal2)
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

    for (jj = 0; jj < (picc->GetWidth()) * (picc->GetHeight()); jj++) {
      int xxl = jj % (picc->GetWidth()), yyl = jj / (picc->GetWidth());
      int rr = picc->GetPixel(xxl, yyl);
      picc->PutPixel(xxl, yyl, color_mapped_table[rr]);
    }
  }

  void wremapall(color * pal1, IBitmap *picc, color * pal2)
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

  void wcopyscreen(int x1, int y1, int x2, int y2, IBitmap *src, int dx, int dy, IBitmap *dest)
  {
    if (src == NULL)
      src = Bitmap::GetScreenBitmap();

    if (dest == NULL)
      dest = Bitmap::GetScreenBitmap();

    dest->Blit(src, x1, y1, dx, dy, (x2 - x1) + 1, (y2 - y1) + 1);
  }


  void wbutt(int x1, int y1, int x2, int y2)
  {
    wsetcolor(254);
    abuf->FillRect(CRect(x1, y1, x2, y2), currentcolor);
    wsetcolor(253);
    abuf->DrawLine(HLine(x1 - 1, x2 + 1, y1 - 1), currentcolor);
    abuf->DrawLine(CLine(x1 - 1, y1 - 1, x1 - 1, y2 + 1), currentcolor);
    wsetcolor(255);
    abuf->DrawLine(HLine(x1 - 1, x2 + 1, y2 + 1), currentcolor);
    abuf->DrawLine(CLine(x2 + 1, y1 - 1, x2 + 1, y2 + 1), currentcolor);
  }


#ifdef __cplusplus
}
#endif