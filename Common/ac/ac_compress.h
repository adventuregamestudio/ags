#ifndef __AC_COMPRESS_H
#define __AC_COMPRESS_H

#if defined(LINUX_VERSION) || defined(MAC_VERSION) || defined(DJGPP) || defined(_MSC_VER)



extern long save_lzw(char *fnn, BITMAP *bmpp, color *pall, long offe);

/*long load_lzw(char*fnn,BITMAP*bmm,color*pall,long ooff);*/
extern long load_lzw(FILE *iii, BITMAP *bmm, color *pall);
extern long savecompressed_allegro(char *fnn, BITMAP *bmpp, color *pall, long ooo);
extern long loadcompressed_allegro(FILE *fpp, BITMAP **bimpp, color *pall, long ooo);

extern char *lztempfnm;
extern BITMAP *recalced;

// returns bytes per pixel for bitmap's color depth
extern int bmp_bpp(BITMAP*bmpt);

#endif // LINUX_VERSION || MAC_VERSION || DJGPP || _MSC_VER

#endif // __AC_COMPRESS_H