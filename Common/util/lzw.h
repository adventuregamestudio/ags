
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_CN_UTIL__LZW_H
#define __AGS_CN_UTIL__LZW_H

namespace AGS { namespace Common { class CDataStream; } }
using namespace AGS; // FIXME later

void lzwcompress(Common::CDataStream *lzw_in, Common::CDataStream *out);
unsigned char *lzwexpand_to_mem(Common::CDataStream *in);

#endif // __AGS_CN_UTIL__LZW_H
