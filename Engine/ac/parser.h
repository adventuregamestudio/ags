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
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__PARSER_H
#define __AGS_EE_AC__PARSER_H

#include "ac/wordsdictionary.h"

int Parser_FindWordID(const char *wordToFind);
const char* Parser_SaidUnknownWord();
void ParseText(const char *text);
int Parser_Said(const char*checkwords);
int SaidUnknownWord(char *buffer);

//=============================================================================

class ITextParser
{
public:
    enum PatternItemFlags
    {
        kPatternItemOptional     = 0x1, // non-strict match expected
        kPatternItemAltFirst     = 0x2, // first item of the alternatives sequence
        kPatternItemAltNext      = 0x4  // next of the alternatives sequence
    };

    typedef std::vector<uint16_t> ParsedSentence;
    typedef std::vector<std::pair<uint16_t, PatternItemFlags>> ParsedPattern;

    virtual ~ITextParser() = default;
    // Parses a simple sentence, e.g. a user input, and converts it to the word ID list
    virtual bool ParseSentence(const String &src_text, ParsedSentence &words, size_t max_words, String *bad_parsed_word) = 0;
    // Parses input text and converts it into a pattern sequence, that may be matched with a user input
    virtual bool ParsePattern(const String &src_text, ParsedPattern &pattern, size_t max_words, String *bad_parsed_word) = 0;
    // Matches the given pattern with the input sentence
    virtual bool MatchPattern(const ParsedSentence &input, const ParsedPattern &pattern) = 0;
};

ITextParser *CreateGameTextParser(AGS::Common::WordsDictionary *dict, bool is_unicode, const String &locale_name);
ITextParser *GetGameTextParser();
uint16_t FindWordInDictionary(const String &lookfor);
//bool parse_sentence(const char *src_text, std::vector<uint16_t> &wordarray, size_t max_words, const std::vector<uint16_t> *compareto, String *bad_parsed_word);

#endif // __AGS_EE_AC__PARSER_H
