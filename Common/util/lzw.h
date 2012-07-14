
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_CN_UTIL__LZW_H
#define __AGS_CN_UTIL__LZW_H

void lzwcompress(FILE * f, FILE * out);
unsigned char *lzwexpand_to_mem(FILE * ii);

#endif // __AGS_CN_UTIL__LZW_H
