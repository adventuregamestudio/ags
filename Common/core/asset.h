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
// AssetInfo and AssetLibInfo - classes describing generic asset library.
//
//=============================================================================
#ifndef __AGS_CN_CORE__ASSET_H
#define __AGS_CN_CORE__ASSET_H

#include <vector>
#include "util/string.h"

namespace AGS
{
namespace Common
{

// Information on single asset
struct AssetInfo
{
    // A pair of filename and libuid is assumed to be unique in game scope
    String      FileName;   // filename associated with asset
    int32_t     LibUid;     // index of library partition (separate file)
    soff_t      Offset;     // asset's position in library file (in bytes)
    soff_t      Size;       // asset's size (in bytes)

    AssetInfo();
};

// Information on multifile asset library
struct AssetLibInfo
{
    String BasePath;                   // full path to the base filename
    String BaseDir;                    // library's directory
    String BaseFileName;               // library's base (head) filename
    std::vector<String> LibFileNames;  // filename for each library part

    // Library contents
    std::vector<AssetInfo> AssetInfos; // information on contained assets
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_CORE__ASSET_H
