
#include <stdio.h>
#include "wgt2allg.h"
#include "ac/ac_compress.h"
#include "ac/ac_common.h"	// quit()
#include "compress.h"
#include "misc.h"




#if defined(LINUX_VERSION) || defined(MAC_VERSION) || defined(DJGPP) || defined(_MSC_VER)

extern void lzwcompress(FILE * f, FILE * out);
extern unsigned char *lzwexpand_to_mem(FILE * ii);
extern long outbytes, maxsize, putbytes;

extern int _acroom_bpp;  // bytes per pixel of currently loading room


char *lztempfnm = "~aclzw.tmp";
BITMAP *recalced = NULL;

// returns bytes per pixel for bitmap's color depth
int bmp_bpp(BITMAP*bmpt) {
  if (bitmap_color_depth(bmpt) == 15)
    return 2;

  return bitmap_color_depth(bmpt) / 8;
}

long save_lzw(char *fnn, BITMAP *bmpp, color *pall, long offe) {
  FILE  *ooo, *iii;
  long  fll, toret, gobacto;

  ooo = ci_fopen(lztempfnm, "wb");
  putw(bmpp->w * bmp_bpp(bmpp), ooo);
  putw(bmpp->h, ooo);
  fwrite(&bmpp->line[0][0], bmpp->w * bmp_bpp(bmpp), bmpp->h, ooo);
  fclose(ooo);

  iii = ci_fopen(fnn, "r+b");
  fseek(iii, offe, SEEK_SET);

  ooo = ci_fopen(lztempfnm, "rb");
  fll = filelength(fileno(ooo));
  fwrite(&pall[0], sizeof(color), 256, iii);
  fwrite(&fll, 4, 1, iii);
  gobacto = ftell(iii);

  // reserve space for compressed size
  fwrite(&fll, 4, 1, iii);
  lzwcompress(ooo, iii);
  toret = ftell(iii);
  fseek(iii, gobacto, SEEK_SET);
  fll = (toret - gobacto) - 4;
  fwrite(&fll, 4, 1, iii);      // write compressed size
  fclose(ooo);
  fclose(iii);
  unlink(lztempfnm);

  return toret;
}

/*long load_lzw(char*fnn,BITMAP*bmm,color*pall,long ooff) {
  recalced=bmm;
  FILE*iii=clibfopen(fnn,"rb");
  fseek(iii,ooff,SEEK_SET);*/

long load_lzw(FILE *iii, BITMAP *bmm, color *pall) {
  long          uncompsiz, *loptr;
  unsigned char *membuffer;
  int           arin;

  recalced = bmm;
  // MACPORT FIX (HACK REALLY)
  fread(&pall[0], 1, sizeof(color)*256, iii);
  fread(&maxsize, 4, 1, iii);
  fread(&uncompsiz,4,1,iii);

  uncompsiz += ftell(iii);
  outbytes = 0; putbytes = 0;

  update_polled_stuff_if_runtime();
  membuffer = lzwexpand_to_mem(iii);
  update_polled_stuff_if_runtime();

  loptr = (long *)&membuffer[0];
  membuffer += 8;
#ifdef ALLEGRO_BIG_ENDIAN
  loptr[0] = __int_swap_endian(loptr[0]);
  loptr[1] = __int_swap_endian(loptr[1]);
  int bitmapNumPixels = loptr[0]*loptr[1]/_acroom_bpp;
  switch (_acroom_bpp) // bytes per pixel!
  {
    case 1:
    {
      // all done
      break;
    }
    case 2:
    {
      short *sp = (short *)membuffer;
      for (int i = 0; i < bitmapNumPixels; ++i)
      {
        sp[i] = __short_swap_endian(sp[i]);
      }
      // all done
      break;
    }
    case 4:
    {
      int *ip = (int *)membuffer;
      for (int i = 0; i < bitmapNumPixels; ++i)
      {
        ip[i] = __int_swap_endian(ip[i]);
      }
      // all done
      break;
    }
  }
#endif // ALLEGRO_BIG_ENDIAN

  if (bmm!=NULL)
    destroy_bitmap(bmm);

  update_polled_stuff_if_runtime();

  bmm = create_bitmap_ex(_acroom_bpp * 8, (loptr[0] / _acroom_bpp), loptr[1]);
  if (bmm == NULL)
    quit("!load_room: not enough memory to load room background");

  update_polled_stuff_if_runtime();

  acquire_bitmap (bmm);
  recalced = bmm;

  for (arin = 0; arin < loptr[1]; arin++)
    memcpy(&bmm->line[arin][0], &membuffer[arin * loptr[0]], loptr[0]);

  release_bitmap (bmm);

  update_polled_stuff_if_runtime();

  free(membuffer-8);

  if (ftell(iii) != uncompsiz)
    fseek(iii, uncompsiz, SEEK_SET);

  update_polled_stuff_if_runtime();

  return uncompsiz;
}

long savecompressed_allegro(char *fnn, BITMAP *bmpp, color *pall, long ooo) {
  unsigned char *wgtbl = (unsigned char *)malloc(bmpp->w * bmpp->h + 4);
  short         *sss = (short *)wgtbl;
  long          toret;

  sss[0] = bmpp->w;
  sss[1] = bmpp->h;

  memcpy(&wgtbl[4], &bmpp->line[0][0], bmpp->w * bmpp->h);

  toret = csavecompressed(fnn, wgtbl, pall, ooo);
  free(wgtbl);
  return toret;
}

long loadcompressed_allegro(FILE *fpp, BITMAP **bimpp, color *pall, long ooo) {
  short widd,hitt;
  int   ii;

  BITMAP *bim = *bimpp;
  if (bim != NULL)
    destroy_bitmap(bim);

  fread(&widd,2,1,fpp);
  fread(&hitt,2,1,fpp);
  bim = create_bitmap_ex(8, widd, hitt);
  if (bim == NULL)
    quit("!load_room: not enough memory to decompress masks");
  *bimpp = bim;

  for (ii = 0; ii < hitt; ii++) {
    cunpackbitl(&bim->line[ii][0], widd, fpp);
    if (ii % 20 == 0)
      update_polled_stuff_if_runtime();
  }

  fseek(fpp, 768, SEEK_CUR);  // skip palette

  return ftell(fpp);
}


#endif // LINUX_VERSION || MAC_VERSION || DJGPP || _MSC_VER