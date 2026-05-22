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
#include <map>
#include "ac/wordsdictionary.h"
#include "util/string_utils.h"
#include "util/textstreamreader.h"
#include "util/textstreamwriter.h"

using namespace AGS::Common;

namespace AGS
{
namespace DataUtil
{

//-----------------------------------------------------------------------------
// TRS - original translation source a in text format
//-----------------------------------------------------------------------------

const char   OPTION_SEPARATOR = '=';
const String NORMAL_FONT_TAG = "NormalFont";
const String SPEECH_FONT_TAG = "SpeechFont";
const String TEXT_DIRECTION_TAG = "TextDirection";
const String AUTO_PARSERSAID_TAG = "AutoTranslateParserSaid";
const String ENCODING_TAG = "Encoding";
const String GAMEENCODING_TAG = "GameEncoding";
const String LANGUAGE_TAG = "Language";
const String FONT_OVERRIDE_TAG = "Font";
const String TAG_DEFAULT = "DEFAULT";
const String TAG_DIRECTION_LEFT = "LEFT";
const String TAG_DIRECTION_RIGHT = "RIGHT";
const String TAG_ON = "ON";
const String TAG_OFF = "OFF";
const char   ANNOTATE_SEPARATOR = ':';
const String ANNOTATE_PARSERWORD = "PARSERWORD";

const String SECTION_TEXTPARSER = "Game Text Parser";


static int ReadOptionalInt(const String &text)
{
    if (text == TAG_DEFAULT)
        return -1;
    return StrUtil::StringToInt(text, -1);
}

static String WriteOptionalInt(int value)
{
    if (value < 0)
        return TAG_DEFAULT;
    return String::FromFormat("%d", value);
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
            options.push_back(StrUtil::GetKeyValue(sec, OPTION_SEPARATOR));
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
    const auto key_value = StrUtil::GetKeyValue(line, OPTION_SEPARATOR);
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
            tra.RightToLeft = kTextDirection_LTR;
        }
        else if (directionText == TAG_DIRECTION_RIGHT)
        {
            tra.RightToLeft = kTextDirection_RTL;
        }
        else
        {
            tra.RightToLeft = kTextDirection_Default;
        }
    }
    else if (key == AUTO_PARSERSAID_TAG)
    {
        tra.OptFlags |= kTraOpt_AutoTranslateSaid;
    }
    else if (key == ENCODING_TAG)
    {
        tra.EncodingHint = value;
        tra.StrOptions["encoding"] = value;
    }
    else if (key == GAMEENCODING_TAG)
    {
        tra.GameEncodingHint = value;
        tra.StrOptions["gameencoding"] = value;
    }
    else if (key == LANGUAGE_TAG)
    {
        tra.LanguageHint = value;
        tra.StrOptions["language"] = value;
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

static void WriteFontOverrides(const Translation &tra, TextStreamWriter &sw)
{
    for (const auto &font_over : tra.FontOverrides)
    {
        String font_line;
        int fontIndex = font_over.first;
        const FontInfo &finfo = font_over.second;
        font_line.AppendFmt("//#Font%d=", fontIndex);
        if (finfo.FontID >= 0)
        {
            font_line.AppendFmt("Font%d;", finfo.FontID);
        }
        else
        {
            // Only write non-default values. Unfortunately there's no way to know
            // which values user set in the original source file.
            font_line.AppendFmt("File=%s;", finfo.FileName.GetCStr());
            if (finfo.Size > 0)
                font_line.AppendFmt("Size=%d;", finfo.Size);
            if (finfo.SizeMultiplier > 1)
                font_line.AppendFmt("SizeMultiplier=%d;", finfo.SizeMultiplier);

            if (finfo.Outline == FONT_OUTLINE_AUTO)
                font_line.Append("Outline=AUTO;");
            else if(finfo.Outline >= 0)
                font_line.AppendFmt("Outline=Font%d;", finfo.Outline);

            if (finfo.Outline == FONT_OUTLINE_AUTO)
            {
                if (finfo.AutoOutlineStyle == FontInfo::kRounded)
                    font_line.Append("AutoOutline=ROUND;");

                font_line.AppendFmt("AutoOutlineThickness=%d;", finfo.AutoOutlineThickness);
            }

            if ((finfo.Flags & FFLG_LOGICALNOMINALHEIGHT) == 0)
                font_line.Append("HeightDefinition=REAL;");
            else if ((finfo.Flags & FFLG_LOGICALCUSTOMHEIGHT) != 0)
                font_line.Append("HeightDefinition=CUSTOM;");

            if ((finfo.Flags & FFLG_LOGICALCUSTOMHEIGHT) != 0)
            {
                font_line.AppendFmt("CustomHeight=%d;", finfo.CustomHeight);
            }

            if (finfo.YOffset != 0)
                font_line.AppendFmt("VerticalOffset=%d;", finfo.YOffset);
            if (finfo.LineSpacing != 0)
                font_line.AppendFmt("LineSpacing=%d;", finfo.LineSpacing);
            if (finfo.CharacterSpacing != 0)
                font_line.AppendFmt("CharacterSpacing=%d;", finfo.CharacterSpacing);
        }
        sw.WriteLine(font_line);
    }
}

// This is a result of parsing translation item annotations,
// contains only known item options, no custom annotations are saved
struct TraItemOptions
{
    // Text Parser's word ID, which this translation item corresponds to.
    // -1 if does not correspond to any.
    int ParserWordID = -1;
};

TraItemOptions ParseItemOptions(const std::vector<String> &annotations)
{
    TraItemOptions options;

    // Parse for known annotations
    for (const auto &annotation : annotations)
    {
        const auto key_value = StrUtil::GetKeyValue(annotation, ANNOTATE_SEPARATOR);
        const String key = key_value.first;
        const String value = key_value.second;

        if (key == ANNOTATE_PARSERWORD)
        {
            options.ParserWordID = StrUtil::StringToInt(value, -1);
        }
    }

    return options;
}

void AddParserWords(WordsDictionary &dict, const String &line, uint16_t word_id)
{
    const auto words = line.Split(',');
    for (const auto &word : words)
    {
        String word_fixed = word;
        word_fixed.Trim();
        if (!word_fixed.IsNullOrSpace())
            dict.GetWords().insert(std::make_pair(word_fixed, word_id));
    }
}

HError ReadTRS(Translation &tra, std::unique_ptr<Stream> &&in)
{
    // TODO: We meed a separate struct for representing TRS contents, including
    // list of annotations per trs item, user comments, etc. This would allow
    // to save TRS back as-is.

    TextStreamReader sr(std::move(in));
    std::vector<String> annotate_next_line;

    for (String line = sr.ReadLine(); !sr.EOS(); line = sr.ReadLine())
    {
        if (line.StartsWith("//"))
        {
            if (line.GetLength() > 2 && line[2] == '#')
            {
                ReadSpecialTags(tra, line.Mid(3));
            }
            else if (line.GetLength() > 2 && line[2] == '$')
            {
                annotate_next_line.push_back(line.Mid(3));
            }
            continue;
        }

        String src = line;
        String dst = sr.ReadLine();
        if (!src.IsEmpty() && !dst.IsEmpty())
        {
            // Currently doing item split into categories in-place
            // TODO: save all items + annotations in a TRS contents struct, and split them up
            // as a conversion between this TRS struct and Translation struct.
            TraItemOptions options;
            if (annotate_next_line.size() > 0)
            {
                options = ParseItemOptions(annotate_next_line);
                annotate_next_line.clear();
            }

            if (options.ParserWordID >= 0)
                AddParserWords(tra.ParserDict, dst, options.ParserWordID);
            // TODO: warn about duplicates
            else if (tra.Dict.count(src) == 0)
                tra.Dict.insert(std::make_pair(src, dst));
        }
    }

    return HError::None();
}

struct TranslationEntry
{
    String Key;
    String Value;
    std::vector<String> Annotations;

    TranslationEntry() = default;
    TranslationEntry(const String &key, const String &value)
        : Key(key), Value(value) {}
};

struct TranslationSection
{
    String Name;
    String Comment;
    std::vector<TranslationEntry> Entries;

    TranslationSection() = default;
    TranslationSection(const String &name)
        : Name(name) {}
};

// Organizes translated lines by sections.
// There's not much to do if we have a runtime Translation object;
// there's enough data to make only 2 sections:
// - general
// - parser words
static void MakeSections(const Translation &translation, std::vector<TranslationSection> &sectionLists)
{
    TranslationSection general_section;
    for (const auto &entry : translation.Dict)
        general_section.Entries.push_back(TranslationEntry(entry.first, entry.second));
    sectionLists.push_back(std::move(general_section));

    if (translation.ParserDict.GetWords().size() > 0)
    {
        TranslationSection parser_section(SECTION_TEXTPARSER);
        std::multimap<uint16_t, String> words_by_id;
        for (const auto &word : translation.ParserDict.GetWords())
            words_by_id.insert(std::make_pair(word.second, word.first));

        for (const auto &word : words_by_id)
        {
            // FIXME: it appears that there's no way to fully reverse the parser word translation entry,
            // the keys in base game language are not stored within the translation!
            // probably add another TRA extension, for the reference. But then, it should not be read
            // into existing Translation struct, because that struct is meant for runtime use,
            // where such reference will be redundant, excess data.
            TranslationEntry entry("???", word.second);
            entry.Annotations.push_back(String::FromFormat("%s:%d", ANNOTATE_PARSERWORD.GetCStr(), word.first));
            parser_section.Entries.push_back(entry);
        }
        sectionLists.push_back(std::move(parser_section));
    }
}

static HError WriteTRS(const Translation &tra, const std::vector<TranslationSection> &sectionLists, std::unique_ptr<Stream> &&out)
{
    TextStreamWriter sw(std::move(out));

    sw.WriteLine("// AGS TRANSLATION SOURCE FILE");
    sw.WriteLine("// Format is alternating lines with original game text and replacement");
    sw.WriteLine("// text. If you don't want to translate a line, just leave the following");
    sw.WriteLine("// line blank. Lines starting with '//' are comments - DO NOT translate");
    sw.WriteLine("// them. Special characters such as [ and %%s symbolise things within the");
    sw.WriteLine("// game, so should be left in an appropriate place in the message.");
    sw.WriteLine("// ");
    sw.WriteLine("// ** Translation settings are below");
    sw.WriteLine("// ** Leave them as \"DEFAULT\" to use the game settings");
    sw.WriteLine("// The normal font to use - DEFAULT or font number");
    sw.WriteLineFormat("//#NormalFont=%s", WriteOptionalInt(tra.NormalFont).GetCStr());
    sw.WriteLine("// The speech font to use - DEFAULT or font number");
    sw.WriteLineFormat("//#SpeechFont=%s", WriteOptionalInt(tra.SpeechFont).GetCStr());
    sw.WriteLine("// Text direction - DEFAULT, LEFT or RIGHT");
    sw.WriteLineFormat("//#TextDirection=%s", ((tra.RightToLeft == kTextDirection_RTL) ? TAG_DIRECTION_RIGHT.GetCStr() : ((tra.RightToLeft == kTextDirection_Default) ? TAG_DEFAULT.GetCStr() : TAG_DIRECTION_LEFT.GetCStr())));
    sw.WriteLine("// Text encoding hint - ASCII or UTF-8");
    sw.WriteLineFormat("//#Encoding=%s", (tra.EncodingHint.IsEmpty() ? "ASCII" : tra.EncodingHint.GetCStr()));
    sw.WriteLine("// Text language, use standard locale strings, like 'en', 'en_US', etc");
    String lang_hint = tra.LanguageHint;
    lang_hint.Replace('-', '_');
    sw.WriteLineFormat("//#Language=%s", lang_hint.GetCStr());
    sw.WriteLine("// Whether engine should translate Parser.Said strings automatically - ON or OFF");
    sw.WriteLineFormat("//#AutoTranslateParserSaid=%s", ((tra.OptFlags & kTraOpt_AutoTranslateSaid) != 0) ? TAG_ON.GetCStr() : TAG_OFF.GetCStr());
    if (tra.FontOverrides.size() != 0)
    {
        WriteFontOverrides(tra, sw);
    }
    sw.WriteLine("//  ");
    sw.WriteLine("// ** REMEMBER, WRITE YOUR TRANSLATION IN THE EMPTY LINES, DO");
    sw.WriteLine("// ** NOT CHANGE THE EXISTING TEXT.");

    for (const auto &section : sectionLists)
    {
        const auto &entries = section.Entries;
        if (entries.size() == 0)
            continue;

        sw.WriteLine("//-----------------------------------------------------------------------------");
        if (section.Name.IsNullOrSpace())
            sw.WriteLine("//$SECTION:");
        else if (section.Comment.IsNullOrSpace())
            sw.WriteLineFormat("//$SECTION: %s", section.Name.GetCStr());
        else
            sw.WriteLineFormat("//$SECTION: %s; %s", section.Name.GetCStr(), section.Comment.GetCStr());
        sw.WriteLine("//-----------------------------------------------------------------------------");
        for (const auto &entry : entries)
        {
            if (entry.Annotations.size() > 0)
            {
                for (const auto &annotation : entry.Annotations)
                    sw.WriteLineFormat("//$%s", annotation.GetCStr());
            }

            sw.WriteLine(entry.Key);
            sw.WriteLine(entry.Value);
        }
    }
    return HError::None();
}

HError WriteTRS(const Translation &tra, std::unique_ptr<Stream> &&out)
{
    std::vector<TranslationSection> sectionLists;
    MakeSections(tra, sectionLists);
    WriteTRS(tra, sectionLists, std::move(out));
    return HError::None();
}

//-----------------------------------------------------------------------------
// TRA - compiled translation in a binary format
//-----------------------------------------------------------------------------

HError WriteTRA(const Translation &tra, std::unique_ptr<Stream> &&out)
{
    // Check if translation object is meaningful
    if (tra.Dict.size() < 1)
        printf("WARNING: input translation appears to be empty.\n");

    bool has_translation = false;
    for (const auto &kv : tra.Dict)
    {
        has_translation |= !kv.first.IsEmpty() && !kv.second.IsEmpty();
    }
    if (!has_translation)
        printf("WARNING: input translation did not appear to have any translated lines.\n");

    // Write translation
    WriteTraData(tra, std::move(out));
    return HError::None();
}

} // namespace DataUtil
} // namespace AGS
