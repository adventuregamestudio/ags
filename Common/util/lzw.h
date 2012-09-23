
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_CN_UTIL__LZW_H
#define __AGS_CN_UTIL__LZW_H

namespace AGS { namespace Common { class DataStream; } }
using namespace AGS; // FIXME later

void lzwcompress(Common::DataStream *lzw_in, Common::DataStream *out);
unsigned char *lzwexpand_to_mem(Common::DataStream *in);

#endif // __AGS_CN_UTIL__LZW_H
