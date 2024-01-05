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
//  Command Line Input Parser
//
//=============================================================================

#ifndef __AGS_CN_UTIL__CMDLINEOPTS_H
#define __AGS_CN_UTIL__CMDLINEOPTS_H

#include <vector>
#include <set>
#include "util/string.h"

namespace AGS
{
namespace Common
{

namespace CmdLineOpts
{

    struct ParseResult {
        bool HelpRequested = false;
        std::vector<std::pair<String, String>> OptWithValue{};
        std::vector<String> PosArgs;
        std::multiset<String> Opt;
    };

    // opt_params_with_values are the options that always take an argument.
    // example: `-l Val` or `--long Val` are passed with {"-l","--long"}.
    ParseResult Parse(int argc, const char *const argv[], const std::set<String> &opt_params_with_values);

} // namespace CmdLineOpts

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__CMDLINEOPTS_H