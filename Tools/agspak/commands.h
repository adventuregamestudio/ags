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
#ifndef __AGS_TOOL_AGSPAK__COMMANDS_H
#define __AGS_TOOL_AGSPAK__COMMANDS_H

#include <regex>
#include "util/string.h"

namespace AGSPak
{
    using String = AGS::Common::String;

    int Command_Create(const String &src_dir, const String &dst_pak, const std::vector<String> &pattern_list,
        const String &pattern_file, bool do_subdirs, size_t part_size_mb, bool verbose);
    int Command_Export(const String &src_pak, const String &dst_dir, const std::vector<String> &pattern_list);
    int Command_List(const String &src_pak);
}

#endif // __AGS_TOOL_AGSPAK__COMMANDS_H
