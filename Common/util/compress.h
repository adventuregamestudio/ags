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

#ifndef __AC_COMPRESS_H
#define __AC_COMPRESS_H

#include <stdio.h>

namespace AGS { namespace Common { class DataStream; } }
namespace AGS { namespace Common { class Bitmap; }}
using namespace AGS; // FIXME later

// MACPORT FIX 9/6/05: removed far and put space after *
#ifdef __block
#undef __block
#endif
typedef unsigned char * __block;

long csavecompressed(char *finam, __block tobesaved, color pala[256], long exto);

void cpackbitl(unsigned char *line, int size, Common::DataStream *out);
void cpackbitl16(unsigned short *line, int size, Common::DataStream *out);
void cpackbitl32(unsigned int *line, int size, Common::DataStream *out);
int  cunpackbitl(unsigned char *line, int size, Common::DataStream *in);
int  cunpackbitl16(unsigned short *line, int size, Common::DataStream *in);
int  cunpackbitl32(unsigned int *line, int size, Common::DataStream *in);

//=============================================================================

long save_lzw(char *fnn, Common::Bitmap *bmpp, color *pall, long offe);

/*long load_lzw(char*fnn,Common::Bitmap*bmm,color*pall,long ooff);*/
long load_lzw(Common::DataStream *in, Common::Bitmap *bmm, color *pall);
long savecompressed_allegro(char *fnn, Common::Bitmap *bmpp, color *pall, long write_at);
long loadcompressed_allegro(Common::DataStream *in, Common::Bitmap **bimpp, color *pall, long read_at);

//extern char *lztempfnm;
//extern Common::Bitmap *recalced;

// returns bytes per pixel for bitmap's color depth
int bmp_bpp(Common::Bitmap*bmpt);

#endif // __AC_COMPRESS_H
