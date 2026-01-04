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
#ifndef __AGS_TOOL_DATA__INCLUDEEUTILS_H
#define __AGS_TOOL_DATA__INCLUDEEUTILS_H

#include "util/string.h"
#include "util/error.h"
#include <vector>

namespace AGS
{
namespace DataUtil
{

using AGS::Common::HError;
using AGS::Common::String;

// Pass a list of files that is filtered in place by include-like patterns in a file
HError IncludeFiles(const std::vector<String> &input_files, std::vector<String> &output_files,
    const String &parent, const String &include_pattern_file, bool verbose);

} // namespace DataUtil
} // namespace AGS

#endif // __AGS_TOOL_DATA__INCLUDEEUTILS_H