//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
/// The "Include Utils" deal with the include/exclude list, containing patterns
/// in the .gitignore format, which may instruct to either include or
/// exclude particular file path.
/// The code is based on the Python's fnmatch, and a gist demonstrating
/// its port to C++, see:
/// https://github.com/python/cpython/blob/main/Lib/fnmatch.py
/// https://gist.github.com/alco/1869512
/// NOTE: the level of support for .gitignore format is not clear,
/// maybe not all of the possible edge cases are handled here.
/// For the reference, .gitignore specs:
/// https://git-scm.com/docs/gitignore
//=============================================================================
#include "data/include_utils.h"
#include "util/file.h"
#include "util/path.h"
#include "util/textstreamreader.h"
#include <regex>

using namespace AGS::Common;

namespace AGS
{
namespace DataUtil
{

using AGS::Common::Stream;

enum PatternType
{
    eInclude = 0,
    eExclude
};

struct Pattern
{
    PatternType Type;
    std::regex Regex;
    String TextualTranslatedPattern; // for debug purposes
    String TextualOriginalPattern; // for debug purposes
};

static bool is_regex_syntax_character(const int c)
{
    // see https://tc39.es/ecma262/#prod-SyntaxCharacter
    return (c == '^') || (c == '$') || (c =='\\') || (c == '.') || (c == '*') || (c == '+') || (c == '?') ||
        (c == '(') || (c == ')') || (c == '[') || (c == ']') || (c == '{') || (c == '}') || (c == '|');
}

static String translate_to_regex_string(const String &pattern)
{
    int i = 0;
    int n = (int)pattern.GetLength();
    String result;

    // The pattern must match the path section either at the beginning or right after the path separator
    if (!pattern.StartsWith("/"))
    {
        result.Append("(^|/)");
    }

    while (i < n) {
        char c = pattern[i];
        ++i;

        if (c == '*') {
            result.Append(".*");
        } else if (c == '?') {
            result.AppendChar('.');
        } else if (c == '[') {
            int j = i;
            /*
             * The following two statements check if the sequence we stumbled
             * upon is '[]' or '[!]' because those are not valid character
             * classes.
             */
            if (j < n && pattern[j] == '!')
                ++j;
            if (j < n && pattern[j] == ']')
                ++j;
            /*
             * Look for the closing ']' right off the bat. If one is not found,
             * escape the opening '[' and continue.  If it is found, process
             * the contents of '[...]'.
             */
            while (j < n && pattern[j] != ']')
                ++j;
            if (j >= n) {
                result.Append("\\[");
            } else {
                String stuff = pattern;
                stuff.ClipMid(i, j-i);
                stuff.Replace("\\"," \\\\");
                char first_char = pattern.GetAt(i);
                i = j + 1;
                result.Append("[");
                if (first_char == '!') {
                    result.Append("^");
                    stuff.ClipLeft(1);
                    result.Append(stuff);
                } else if (first_char == '^') {
                    result.Append("\\");
                    result.Append(stuff);
                } else {
                    result.Append(stuff);
                }
                result.Append("]");
            }
        } else {
            // Escape a regex syntax character if necessary, otherwise print one verbatim
            if (is_regex_syntax_character(c)) {
                result.Append("\\");
                result.AppendChar(c);
            } else {
                result.AppendChar(c);
            }
        }
    }

    // The pattern must match the path section either at the end or right before the path separator
    if (!pattern.EndsWith("/"))
    {
        result.Append("($|/)");
    }

    return result;
}

String normalize_separators_in_text(const String &path)
{
    String result = path;
    result.Replace("\\\\", "/");
    return result;
}

String normalize_separators_in_string(const String &path)
{
    String result = path;
    result.Replace("\\", "/");
    return result;
}

HError description_to_patterns(const std::vector<String> &description, std::vector<Pattern> &patterns)
{
    for (String line : description)
    {
        line.Trim();

        // line is empty or a comment
        if(line.IsEmpty() || line.StartsWith("#"))
            continue;

        Pattern p;
        p.Type = eInclude;
        if (line[0] == '!')
        {
            p.Type = eExclude;
            line.ClipLeft(1);
        }
        line.MakeLowerUTF8(); // for case insensitivity
        String shell_regex_txt = normalize_separators_in_text(line); // for unix paths

        String regex_txt = translate_to_regex_string(shell_regex_txt);

        p.TextualOriginalPattern = shell_regex_txt;
        p.TextualTranslatedPattern = regex_txt;

        // catch exceptions, since they may happen from user input in the pattern file
        try
        {
            p.Regex = regex_txt.GetCStr();
        }
        catch (const std::exception& ex)
        {
            return new Error(String::FromFormat(
            "include_utils: failed at translating pattern at line '%s', error: '%s'.", line.GetCStr(), ex.what()
            ));
        }

        patterns.emplace_back(p);
    }
    return HError::None();
}

std::vector<String> match_files(const std::vector<String>& files, const std::vector<Pattern> &patterns)
{
    std::vector<String> matches;
    // if a file entry matches no pattern of the include type it should not be included in the list
    // if a file entry matches a pattern of the include type it should be included unless a pattern of exclude type will match it after
    // if a pattern of exclude type exists but a later pattern causes it to be included, it should be included.

    for (const auto& file : files) {
        bool include = false;
        String normalized_file_path = file.LowerUTF8(); // for case insensitivity
        normalized_file_path = normalize_separators_in_string(normalized_file_path);

        for (const auto& pattern : patterns) {
            if (std::regex_search(normalized_file_path.GetCStr(), pattern.Regex)) {
                if (pattern.Type == eInclude) {
                    include = true;
                } else if (pattern.Type == eExclude) {
                    include = false;
                }
            }
        }

        if (include) {
            matches.push_back(file);
        }
    }

    return matches;
}

std::vector<String> read_file(const String &filename, bool verbose)
{
    std::vector<String> lines;
    std::unique_ptr<Stream> in (File::OpenFileRead(filename));
    if (!in)
    {
        if(verbose)
        {
            printf("Warning: pattern file '%s' not found.\n", filename.GetCStr());
        }
        return lines;
    }
    printf("Info: found pattern file '%s'.\n", filename.GetCStr());

    TextStreamReader sr(std::move(in));

    if (sr.EOS())
        return lines;

    do {
        String line = sr.ReadLine();
        if(!(line.IsNullOrSpace() || line.IsEmpty()))
            lines.push_back(line);
    } while(!sr.EOS());

    return lines;
}

HError MatchPatternPaths(const std::vector<String> &input_files, std::vector<String> &output_matches,
    const std::vector<String> &patterns_description)
{
    std::vector<Pattern> patterns {};
    HError err = description_to_patterns(patterns_description, patterns);
    const std::vector<String> matches = match_files(input_files, patterns);
    output_matches.insert(output_matches.end(), matches.begin(), matches.end());

    return err;
}

HError IncludeFiles(const std::vector<String> &input_files, std::vector<String> &output_files,
    const String &parent, const String &include_pattern_file, const bool verbose)
{
    const String ignore_filename = Path::ConcatPaths(parent, include_pattern_file);
    std::vector<String> patterns_description = read_file(ignore_filename, verbose);
    std::vector<String> matches{};
    HError err = MatchPatternPaths(input_files, matches, patterns_description);
    output_files.insert(output_files.end(), matches.begin(), matches.end());

    return err;
}

} // namespace DataUtil
} // namespace AGS
