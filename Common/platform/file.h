
//=============================================================================
//
// Platform-independent FILE interface
//
// TODO: abstract interface wrapper around file handle
//
//=============================================================================
#ifndef __AGS_CN_PLATFORM__FILE_H
#define __AGS_CN_PLATFORM__FILE_H

#include <stdio.h>
// TODO: other platforms?

#if defined(LINUX_VERSION) || defined(MAC_VERSION)
long int filelength(int fhandle);
#endif

#endif // __AGS_CN_PLATFORM__FILE_H
