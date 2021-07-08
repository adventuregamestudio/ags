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
#ifndef __AGS_TOOL_DATA__MFLUTIL_H
#define __AGS_TOOL_DATA__MFLUTIL_H

#include "core/asset.h"
#include "util/error.h"

namespace AGS
{
namespace DataUtil
{
    using AGS::Common::AssetLibInfo;
    using AGS::Common::HError;
    using AGS::Common::String;

    // Unpacks the library by reading its parts and writing assets into files.
    // lib_dir - tells the directory where the library parts are located,
    // lib_basefile - tells the actual file name of the first library part,
    //     because it may sometimes be different from the name saved in lib.
    // The output files will be written into dst_dir directory;
    // if the asset name contains directories, they will be created as sub-
    // directories inside dst_dir.
    HError UnpackLibrary(const AssetLibInfo &lib, const String &lib_dir,
        const String &lib_basefile, const String &dst_dir);

} // namespace DataUtil
} // namespace AGS

#endif // __AGS_TOOL_DATA__MFLUTIL_H
