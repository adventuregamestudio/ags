#ifndef __CROOM_FUNC_COMPRESS_H
#define __CROOM_FUNC_COMPRESS_H

#if defined(LINUX_VERSION) || defined(MAC_VERSION) || defined(DJGPP) || defined(_MSC_VER)
extern void lzwcompress(FILE *,FILE *);
extern void lzwexpand(FILE *,FILE *);
extern unsigned char *lzwexpand_to_mem(FILE *);
extern long maxsize, outbytes, putbytes;
extern char *lztempfnm;

extern long save_lzw(char *fnn, BITMAP *bmpp, color *pall, long offe);
extern BITMAP *recalced;
/*long load_lzw(char*fnn,BITMAP*bmm,color*pall,long ooff);*/
extern long load_lzw(FILE *iii, BITMAP *bmm, color *pall);
extern long savecompressed_allegro(char *fnn, BITMAP *bmpp, color *pall, long ooo);
extern long loadcompressed_allegro(FILE *fpp, BITMAP **bimpp, color *pall, long ooo);
#endif // LINUX_VERSION || MAC_VERSION || DJGPP || _MSC_VER

#endif // __CROOM_FUNC_COMPRESS_H