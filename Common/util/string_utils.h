
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_CN_UTIL__STRINGUTILS_H
#define __AGS_CN_UTIL__STRINGUTILS_H

#include <stdio.h>
#include <stdarg.h>

namespace AGS { namespace Common { class CDataStream; } }
using namespace AGS; // FIXME later

#if defined(ANDROID_VERSION) || defined(IOS_VERSION)
// Some Android defines that are needed in multiple files.
#include <wchar.h>
extern "C" {
    char *strupr(char *s);
    char *strlwr(char *s);
}
#endif
#if defined(ANDROID_VERSION) || defined(IOS_VERSION) || defined(LINUX_VERSION)
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif

void removeBackslashBracket(char *lbuffer);
// Break up the text into lines, using normal Western left-right style
void split_lines_leftright(const char *todis, int wii, int fonnt);

//=============================================================================

// FIXME: remove later when arrays of chars are replaced by string class
void fputstring(const char *sss, Common::CDataStream *out);
void fgetstring_limit(char *sss, Common::CDataStream *in, int bufsize);
void fgetstring(char *sss, Common::CDataStream *in);


#endif // __AGS_CN_UTIL__STRINGUTILS_H
