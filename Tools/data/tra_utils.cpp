//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "data/tra_utils.h"
#include "ac/wordsdictionary.h"
#include "util/string_utils.h"
#include "util/textstreamreader.h"

using namespace AGS::Common;

namespace AGS
{
namespace DataUtil
{

//-----------------------------------------------------------------------------
// TRA - original translation source a in text format
//-----------------------------------------------------------------------------

const String NORMAL_FONT_TAG("//#NormalFont=");
const String SPEECH_FONT_TAG("//#SpeechFont=");
const String TEXT_DIRECTION_TAG("//#TextDirection=");
const String ENCODING_TAG("//#Encoding=");
const String GAMEENCODING_TAG("//#GameEncoding=");
const char *TAG_DEFAULT = "DEFAULT";
const char *TAG_DIRECTION_LEFT = "LEFT";
const char *TAG_DIRECTION_RIGHT = "RIGHT";

static int ReadOptionalInt(const String &text)
{
    if (text == TAG_DEFAULT)
        return -1;
    return StrUtil::StringToInt(text, -1);
}

static void ReadSpecialTags(Translation &tra, const String &line)
{
    if (line.StartsWith(NORMAL_FONT_TAG))
    {
        tra.NormalFont = ReadOptionalInt(line.Mid(NORMAL_FONT_TAG.GetLength()));
    }
    else if (line.StartsWith(SPEECH_FONT_TAG))
    {
        tra.SpeechFont = ReadOptionalInt(line.Mid(SPEECH_FONT_TAG.GetLength()));
    }
    else if (line.StartsWith(TEXT_DIRECTION_TAG))
    {
        String directionText = line.Mid(TEXT_DIRECTION_TAG.GetLength());
        if (directionText == TAG_DIRECTION_LEFT)
        {
            tra.RightToLeft = 1;
        }
        else if (directionText == TAG_DIRECTION_RIGHT)
        {
            tra.RightToLeft = 2;
        }
        else
        {
            tra.RightToLeft = -1;
        }
    }
    // TODO: make a generic dictionary instead and save any option
    else if (line.StartsWith(ENCODING_TAG))
    {
        tra.StrOptions["encoding"] = line.Mid(ENCODING_TAG.GetLength());
    }
    else if (line.StartsWith(GAMEENCODING_TAG))
    {
        tra.StrOptions["gameencoding"] = line.Mid(GAMEENCODING_TAG.GetLength());
    }
}

HError ReadTRS(Translation &tra, Stream *in)
{
    TextStreamReader sr(in);

    String line;
    for (line = sr.ReadLine(); !sr.EOS(); line = sr.ReadLine())
    {
        if (line.StartsWith("//"))
        {
            ReadSpecialTags(tra, line);
            continue;
        }
        String src = line;
        String dst = sr.ReadLine();
        // TODO: warn about duplicates
        if (tra.Dict.count(src) == 0)
        {
            tra.Dict.insert(std::make_pair(src, dst));
        }
    }

    sr.ReleaseStream(); // we do not want to delete it
    return HError::None();
}

//-----------------------------------------------------------------------------
// TRA - compiled translation in a binary format
//-----------------------------------------------------------------------------

HError WriteTRA(const Translation &tra, Stream *out)
{
    // Check if translation object is meaningful
    if (tra.Dict.size() < 1)
        return new Error("Translation source appears to be empty");
    bool has_translation = false;
    for (const auto &kv : tra.Dict)
    {
        has_translation |= !kv.first.IsEmpty() && !kv.second.IsEmpty();
    }
    if (!has_translation)
        return new Error("Translation source did not appear to have any translated lines.");

    // Write translation
    WriteTraData(tra, out);

    return HError::None();
}

} // namespace DataUtil
} // namespace AGS
