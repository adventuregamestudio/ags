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
//
//  Android-specific implementation of some of the File routines;
//  mainly purposed to work with the NDK AAssetManager object.
//
//=============================================================================
#ifndef __AGS_CN_UTIL__ANDROID_FILE_H
#define __AGS_CN_UTIL__ANDROID_FILE_H

#include "core/platform.h"
#if AGS_PLATFORM_OS_ANDROID
#include "core/types.h"
#include "util/string.h"

struct AAssetManager;

namespace AGS
{
namespace Common
{

void           InitAndroidFile();
void           ShutdownAndroidFile();
AAssetManager *GetAAssetManager();
// Tells if the Android Asset of the given name exists
bool           GetAAssetExists(const String &filename);
// Gets the Android Asset's size, returns -1 if such asset was not found
soff_t         GetAAssetSize(const String &filename);

} // namespace Common
} // namespace AGS

#endif // AGS_PLATFORM_OS_ANDROID

#endif // __AGS_CN_UTIL__ANDROID_FILE_H
