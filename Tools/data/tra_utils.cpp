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

const String NORMAL_FONT_TAG("NormalFont");
const String SPEECH_FONT_TAG("SpeechFont");
const String TEXT_DIRECTION_TAG("TextDirection");
const String ENCODING_TAG("Encoding");
const String GAMEENCODING_TAG("GameEncoding");
const String FONT_OVERRIDE_TAG("Font");
const char *TAG_DEFAULT = "DEFAULT";
const char *TAG_DIRECTION_LEFT = "LEFT";
const char *TAG_DIRECTION_RIGHT = "RIGHT";

static int ReadOptionalInt(const String &text)
{
    if (text == TAG_DEFAULT)
        return -1;
    return StrUtil::StringToInt(text, -1);
}

static int ParseFontN(const String &line)
{
    if (line.StartsWith(FONT_OVERRIDE_TAG))
    {
        return StrUtil::StringToInt(line.Mid(FONT_OVERRIDE_TAG.GetLength()), -1);
    }
    return -1;
}

static bool ParseFontOverride(const String &line, FontInfo &finfo)
{
    // Format 1:
    //    FontN
    // Format 2:
    //    Property1=Value1;Property2=Value2;Property3=Value3;...
    int re_font_number = ParseFontN(line);
    if (re_font_number >= 0)
    {
        // This is a replacement with existing font
        finfo.FontID = re_font_number;
        return true;
    }
    else
    {
        // This is a new font generation
        finfo.FontID = -1; // mark it as not one of the game's font
        const auto sections = line.Split(';');
        std::vector<std::pair<String, String>> options;
        for (const auto &sec : sections)
        {
            options.push_back(StrUtil::GetKeyValue(sec));
        }
        for (const auto &opt : options)
        {
            const String &key = opt.first;
            const String &value = opt.second;

            if (key == "File")
            {
                finfo.FileName = value;
            }
            else if (key == "Size")
            {
                finfo.Size = StrUtil::StringToInt(value);
            }
            else if (key == "SizeMultiplier")
            {
                finfo.SizeMultiplier = StrUtil::StringToInt(value);
            }
            else if (key == "Outline")
            {
                if (value == "NONE")
                {
                    finfo.Outline = FONT_OUTLINE_NONE;
                }
                else if (value == "AUTO")
                {
                    finfo.Outline = FONT_OUTLINE_AUTO;
                }
                else
                {
                    int out_font_id = ParseFontN(value);
                    if (out_font_id >= 0)
                    {
                        finfo.Outline = out_font_id;
                    }
                }
            }
            else if (key == "AutoOutline")
            {
                if (value == "SQUARED")
                {
                    finfo.AutoOutlineStyle = FontInfo::kSquared;
                }
                else if (value == "ROUND")
                {
                    finfo.AutoOutlineStyle = FontInfo::kRounded;
                }
            }
            else if (key == "AutoOutlineThickness")
            {
                finfo.AutoOutlineThickness = StrUtil::StringToInt(value);
            }
            else if (key == "HeightDefinition")
            {
                if (value == "NOMINAL")
                {
                    finfo.SetHeightFlags(FFLG_LOGICALNOMINALHEIGHT);
                }
                else if (value == "REAL")
                {
                    finfo.SetHeightFlags(0 /* use real height */);
                }
                else if (value == "CUSTOM")
                {
                    finfo.SetHeightFlags(FFLG_LOGICALCUSTOMHEIGHT);
                }
            }
            else if (key == "CustomHeight")
            {
                finfo.CustomHeight = StrUtil::StringToInt(value);
            }
            else if (key == "VerticalOffset")
            {
                finfo.YOffset = StrUtil::StringToInt(value);
            }
            else if (key == "LineSpacing")
            {
                finfo.LineSpacing = StrUtil::StringToInt(value);
            }
            else if (key == "CharacterSpacing")
            {
                finfo.CharacterSpacing = StrUtil::StringToInt(value);
            }
        }
        return true;
    }
}

static void ReadSpecialTags(Translation &tra, const String &line)
{
    const auto key_value = StrUtil::GetKeyValue(line);
    const String key = key_value.first;
    const String value = key_value.second;
    if (key == NORMAL_FONT_TAG)
    {
        tra.NormalFont = ReadOptionalInt(value);
    }
    else if (key == SPEECH_FONT_TAG)
    {
        tra.SpeechFont = ReadOptionalInt(value);
    }
    else if (key == TEXT_DIRECTION_TAG)
    {
        String directionText = value;
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
    else if (key == ENCODING_TAG)
    {
        tra.StrOptions["encoding"] = value;
    }
    else if (key == GAMEENCODING_TAG)
    {
        tra.StrOptions["gameencoding"] = value;
    }
    else if (key.StartsWith(FONT_OVERRIDE_TAG))
    {
        int font_id = ParseFontN(key);
        if ((font_id >= 0) && (tra.FontOverrides.count(font_id) == 0))
        {
            FontInfo finfo;
            if (ParseFontOverride(value, finfo))
            {
                tra.FontOverrides[font_id] = finfo;
            }
        }
    }
}

HError ReadTRS(Translation &tra, std::unique_ptr<Stream> &&in)
{
    TextStreamReader sr(std::move(in));

    String line;
    for (line = sr.ReadLine(); !sr.EOS(); line = sr.ReadLine())
    {
        if (line.StartsWith("//"))
        {
            if (line.GetLength() > 2 && line[2] == '#')
                ReadSpecialTags(tra, line.Mid(3));
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

    return HError::None();
}

//-----------------------------------------------------------------------------
// TRA - compiled translation in a binary format
//-----------------------------------------------------------------------------

HError WriteTRA(const Translation &tra, std::unique_ptr<Stream> &&out)
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
        printf("WARNING: translation source did not appear to have any translated lines.\n");

    // Write translation
    WriteTraData(tra, std::move(out));
    return HError::None();
}

} // namespace DataUtil
} // namespace AGS
