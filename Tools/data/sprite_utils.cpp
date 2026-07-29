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
#include "data/sprite_utils.h"
#include "util/directory.h"
#include "util/path.h"
#include "util/string_types.h"
#include "util/string_utils.h"

using namespace AGS::Common;

namespace AGS
{
namespace DataUtil
{

static const String DefaultPattern = "spr%06d";
static const String DefaultRegexPattern = "spr\\d{6}";
static const String DefaultExtension = "png";
static const CstrArr<kNumSprCompressTypes> CompressionNames = {{"none", "rle", "lzw", "deflate"}};

String GetCompressionName(SpriteCompression compress)
{
    return String::Wrapper(StrUtil::SelectCStr<kNumSprCompressTypes>(
        CompressionNames, compress, "unknown"));
}

SpriteCompression CompressionFromName(const String &compress_name)
{
    return StrUtil::ParseEnum<SpriteCompression, kNumSprCompressTypes>(
        compress_name, CompressionNames, kSprCompress_None);
}

bool ResolveImageFilePattern(const String &pattern, String &res_pattern)
{
    String regex_pattern;
    return ResolveImageFilePattern(pattern, res_pattern, regex_pattern);
}

bool ResolveImageFilePattern(const String &pattern, String &res_pattern, String &regex_pattern)
{
    if (pattern.IsNullOrSpace())
    {
        res_pattern = String::FromFormat("%s.%s", DefaultPattern.GetCStr(), DefaultExtension.GetCStr());
        regex_pattern = String::FromFormat("%s.%s", DefaultRegexPattern.GetCStr(), DefaultExtension.GetCStr());
        return true;
    }

    res_pattern = "";
    regex_pattern = "";
    bool found_spritenum = false;
    const auto parts = pattern.Split('%');
    // Each second part is assumed a placeholder between two '%'
    for (size_t i = 0; i < parts.size(); ++i)
    {
        if (i % 2 == 0)
        {
            res_pattern.Append(parts[i]);
            regex_pattern.Append(parts[i]);
        }
        else if (i < parts.size() - 1)
        {
            // Resolve placeholder
            if ((parts[i].GetLast() == 'N' || parts[i].GetLast() == 'n')
                && (parts[i].GetLength() == 2) && std::isdigit(parts[i][0]))
            {
                res_pattern.AppendFmt("%%0%dd", parts[i][0] - '0');
                regex_pattern.AppendFmt("\\d{%d}", parts[i][0] - '0');
                found_spritenum = true;
            }
            else
            {
                res_pattern.Append("_");
                regex_pattern.Append("_");
            }
        }
    }

    // We cannot export multiple sprites without a sprite number placeholder
    if (!found_spritenum)
        return false;

    if (Path::GetFileExtension(res_pattern).IsEmpty())
    {
        res_pattern = String::FromFormat("%s.%s", res_pattern.GetCStr(), DefaultExtension.GetCStr());
        regex_pattern = String::FromFormat("%s.%s", regex_pattern.GetCStr(), DefaultExtension.GetCStr());
    }
    return true;
}

HError MakeListOfFiles(std::vector<String> &files, const String &asset_dir, const std::regex &regex)
{
    for (FindFile ff = FindFile::Open(asset_dir, regex, true, false, 0);
        !ff.AtEnd(); ff.Next())
    {
        String filename = ff.Current();
        files.push_back(filename);
    }
    return HError::None();
}

} // namespace DataUtil
} // namespace AGS
