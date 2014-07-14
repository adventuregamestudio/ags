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
// Platform-independent Path functions
//
//=============================================================================
#ifndef __AGS_CN_UTIL__PATH_H
#define __AGS_CN_UTIL__PATH_H

#include "util/string.h"

namespace AGS
{
    namespace Common
    {

        namespace Path
        {
            // Tells if the given path is a directory
            bool    IsDirectory(const String &directory);
            // Tells if the given path is a file
            bool    IsFile(const String &filename);

            // Makes a platform-dependant path comparison.
            // This takes into consideration platform's filename case (in)sensivity and
            // DOS-compatible 8.3 filenames;
            // The result value corresponds to stdlib strcmp function.
            int     ComparePaths(const String &path1, const String &path2);
            // Makes a path have only '/' slashes; this is to make it easier to work
            // with path, knowing it contains only one type of directory separators
            void    FixupPath(String &path);
            String  MakeAbsolutePath(const String &path);
        } // namespace Path

    } // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__PATH_H
