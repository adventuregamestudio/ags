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

#include <functional>
#include <regex>
#include <vector>
#include "util/string.h"
#include "util/error.h"

namespace AGS
{
namespace DataUtil
{

using AGS::Common::HError;
using AGS::Common::String;

enum PatternType
{
    eInclude = 0,
    eExclude
};

struct Pattern
{
    PatternType Type;
    std::regex Regex;
    String OriginalPattern; // for debug purposes
    String RegexPattern; // for debug purposes
};

// Creates match pattern list based on textual descriptions
HError CreatePatternList(const std::vector<String> pattern_desc, std::vector<Pattern> &patterns);

// Pass a list of files that is filtered in place by include-like patterns in a file
HError IncludeFiles(const std::vector<String> &input_files, std::vector<String> &output_files,
    const String &include_pattern_file, bool verbose);
// Pass a list of files that is filtered in place by provided patterns
HError IncludeFiles(const std::vector<String> &input_files, std::vector<String> &output_files,
    const std::vector<Pattern> &patterns, bool verbose);

// Pass a list of files that is filtered by array of include-like patterns
// All operations are done without file reading (so it's easier to test)
HError MatchPatternPaths(const std::vector<String> &input_files, std::vector<String> &output_matches,
    const std::vector<String> &pattern_desc);
// Pass a list of files that is filtered by provided patterns
HError MatchPatternPaths(const std::vector<String> &input_files, std::vector<String> &output_matches,
    const std::vector<Pattern> &patterns);

// A predicate functor that can be used to filter items, using provided patterns list
class PatternMatch
{
public:
    PatternMatch(const std::vector<Pattern> &patterns)
        : _patterns(patterns) {}

    bool operator()(const String &item);

private:
    const std::vector<Pattern> &_patterns;
};

} // namespace DataUtil
} // namespace AGS

#endif // __AGS_TOOL_DATA__INCLUDEEUTILS_H