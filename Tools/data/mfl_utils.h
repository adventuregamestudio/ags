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
#include "util/multifilelib.h"
#include "util/stream.h"

namespace AGS
{
namespace DataUtil
{
    using AGS::Common::AssetInfo;
    using AGS::Common::AssetLibInfo;
    using AGS::Common::HError;
    using AGS::Common::Stream;
    using AGS::Common::String;
    namespace MFLUtil = AGS::Common::MFLUtil;

    // Unpacks the library by reading its parts and writing assets into files.
    // lib_dir - tells the directory where the library parts are located,
    // lib_basefile - tells the actual file name of the first library part,
    //     because it may sometimes be different from the name saved in lib.
    // The output files will be written into dst_dir directory;
    // if the asset name contains directories, they will be created as sub-
    // directories inside dst_dir.
    HError UnpackLibrary(const AssetLibInfo &lib, const String &lib_dir,
        const String &lib_basefile, const String &dst_dir);
    // Gather a list of files from a given directory
    HError MakeAssetList(std::vector<AssetInfo> &assets, const String &asset_dir,
        bool do_subdirs, const String &lib_basefile);
    // Generate AssetLibInfo based on a list of assets, optionally limiting each
    // library partition by part_size bytes
    HError MakeAssetLib(AssetLibInfo &lib, const String &lib_basefile,
        std::vector<AssetInfo> &assets, soff_t part_size = 0);
    // Writes the library partition into the file lib_filename;
    // recalculates asset offsets and stores in lib as it goes.
    HError WriteLibraryFile(AssetLibInfo &lib, const String &src_dir,
        const String &lib_filename, MFLUtil::MFLVersion lib_version, int lib_index);
    // Writes the potentially multi-file library into the dst_dir directory;
    // recalculates asset offsets and stores in lib as it goes.
    HError WriteLibrary(AssetLibInfo &lib, const String &asset_dir,
        const String &dst_dir, MFLUtil::MFLVersion lib_version);

} // namespace DataUtil
} // namespace AGS

#endif // __AGS_TOOL_DATA__MFLUTIL_H
