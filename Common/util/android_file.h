//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
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
#include <regex>
#include "core/types.h"
#include "util/string.h"

struct AAssetManager;
struct AAssetDir;
struct AAsset;

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

// AndroidDir wraps Android Asset directory and ensures its disposal
class AndroidADir
{
public:
    AndroidADir(AndroidADir &&aadir);
    AndroidADir(const String &dirname);
    ~AndroidADir();

    // Iterate to the next asset, returns asset's name or empty string
    // if not more assets are found in this directory
    String Next();
    // Iterates through assets until matching the name pattern;
    // returns asset's name or empty string if no matching asset found
    String Next(const std::regex &pattern);

    operator bool() const { return _dir != nullptr; }

private:
    AAssetDir *_dir {nullptr};
};

} // namespace Common
} // namespace AGS

#endif // AGS_PLATFORM_OS_ANDROID

#endif // __AGS_CN_UTIL__ANDROID_FILE_H
