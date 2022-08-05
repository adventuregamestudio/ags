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
    // Container struct to store command line options
    struct ParseResult {

        // True if the parsing failed
        bool IsBadlyFormed = false;

        // Contains the name of the option that caused IsBadlyFormed to be true
        String BadlyFormedOption;

        //If any of the "help" flags was encountered in the options
        bool HelpRequested = false;

        // Options that don't start with "-" (the option is directly the value)
        std::vector<String> PosArgs;

        // Options that are just flags
        std::multiset<String> Opt;

        // Options that are followed by 1 mandatory value
        std::vector<std::pair<String, String>> OptWith1Value{};

        // Options that are followed by 2 mandatory values
        std::vector<std::pair<String, std::vector<String>>> OptWith2Values{};

        // Options that are followed by 3 mandatory values
        std::vector<std::pair<String, std::vector<String>>> OptWith3Values{};

        // Options that are followed by 4 mandatory values
        std::vector<std::pair<String, std::vector<String>>> OptWith4Values{};

    };

    // Utility class for working with the raw C-style array of command line options.
    class CmdLineOptsParser {

    public:

        // Takes an array of command line parameters and turns them into a standard set of options,
        // where each command line option is matched with its corresponding parameter(s), if any.
        // opt_params_with_1_values are the options that always take an argument.
        // example: `-l Val` or `--long Val` are passed with {"-l","--long"}.
        static ParseResult Parse(
            int argc,
            const char* const argv[],
            const std::set<String>& opt_params_with_1_values,
            const std::set<String>& opt_params_with_2_values,
            const std::set<String>& opt_params_with_3_values,
            const std::set<String>& opt_params_with_4_values
        );
    };


} // namespace CmdLineOpts

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__CMDLINEOPTS_H
