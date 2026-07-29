//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#ifndef __AGS_TOOL_DATA__SPRITEFILEUTILS_H
#define __AGS_TOOL_DATA__SPRITEFILEUTILS_H

#include <regex>
#include <vector>
#include "ac/spritefile.h"
#include "util/error.h"
#include "util/string.h"

namespace AGS
{
namespace DataUtil
{
    using HError = AGS::Common::HError;
    using String = AGS::Common::String;

    String GetCompressionName(Common::SpriteCompression compress);
    Common::SpriteCompression CompressionFromName(const String &compress_name);
    bool ResolveImageFilePattern(const String &pattern, String &res_pattern);
    bool ResolveImageFilePattern(const String &pattern, String &res_pattern, String &regex_pattern);
    // TODO: move elsewhere, more generic utils?
    HError MakeListOfFiles(std::vector<String> &files, const String &asset_dir, const std::regex &regex);

} // namespace DataUtil
} // namespace AGS

#endif // __AGS_TOOL_DATA__SPRITEFILEUTILS_H
