/*
This is UNPUBLISHED PROPRIETARY SOURCE CODE;
the contents of this file may not be disclosed to third parties,
copied or duplicated in any form, in whole or in part, without
prior express permission from Chris Jones.
*/

#ifndef __AC_COMPRESS_H
#define __AC_COMPRESS_H

#include <stdio.h>

namespace AGS { namespace Common { class CDataStream; } }
namespace AGS { namespace Common { class IBitmap; }}
using namespace AGS; // FIXME later

// MACPORT FIX 9/6/05: removed far and put space after *
#if !defined(MAC_VERSION)
typedef unsigned char * __block;
#else
#ifdef __block
#undef __block
#define __block unsigned char*
#endif
#endif

long csavecompressed(char *finam, __block tobesaved, color pala[256], long exto);

void cpackbitl(unsigned char *line, int size, Common::CDataStream *out);
void cpackbitl16(unsigned short *line, int size, Common::CDataStream *out);
void cpackbitl32(unsigned int *line, int size, Common::CDataStream *out);
int  cunpackbitl(unsigned char *line, int size, Common::CDataStream *in);
int  cunpackbitl16(unsigned short *line, int size, Common::CDataStream *in);
int  cunpackbitl32(unsigned int *line, int size, Common::CDataStream *in);

//=============================================================================

#if defined(LINUX_VERSION) || defined(MAC_VERSION) || defined(DJGPP) || defined(_MSC_VER)

long save_lzw(char *fnn, Common::IBitmap *bmpp, color *pall, long offe);

/*long load_lzw(char*fnn,Common::IBitmap*bmm,color*pall,long ooff);*/
long load_lzw(Common::CDataStream *in, Common::IBitmap *bmm, color *pall);
long savecompressed_allegro(char *fnn, Common::IBitmap *bmpp, color *pall, long write_at);
long loadcompressed_allegro(Common::CDataStream *in, Common::IBitmap **bimpp, color *pall, long read_at);

//extern char *lztempfnm;
//extern Common::IBitmap *recalced;

// returns bytes per pixel for bitmap's color depth
int bmp_bpp(Common::IBitmap*bmpt);

#endif // LINUX_VERSION || MAC_VERSION || DJGPP || _MSC_VER

#endif // __AC_COMPRESS_H