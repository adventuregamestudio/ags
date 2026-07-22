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
#include <cstdio>
#include <locale>
#include "ac/common.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_translation.h"
#include "ac/parser.h"
#include "ac/string.h"
#include "debug/debug_log.h"
#include "util/string.h"
#include "util/string_compat.h"
#include "util/utf8.h"

using namespace AGS::Common;

extern GameSetupStruct game;

int Parser_FindWordID(const char *wordToFind)
{
    uint16_t word_id = WordsDictionary::INVALIDWORD;
    auto *parser = GetBaseTextParser();
    if (parser)
        word_id = parser->FindWordInDictionary(String::Wrapper(wordToFind));
    return word_id != WordsDictionary::INVALIDWORD ? word_id : -1;
}

const char* Parser_SaidUnknownWord()
{
    if (play.bad_parsed_word.IsEmpty())
        return nullptr;
    return CreateNewScriptString(play.bad_parsed_word);
}

void ParseText(const char *text)
{
    play.parsed_words.clear();
    play.bad_parsed_word = {};
    // We always use a translated text parser here (this may be still a default language parser)
    auto *parser = GetTranslationTextParser();
    if (parser)
        parser->ParseSentence(text, play.parsed_words, MAX_PARSED_WORDS, &play.bad_parsed_word);
}

// Said: call with argument for example "get apple"; we then check
// word by word if it matches (using dictonary ID equivalence to match
// synonyms). Returns 1 if it does, 0 if not.
int Said(const char *checkwords)
{
    ITextParser *parser = nullptr;
    // If the Parser.Said input is auto-translated, then we got to parse it using the
    // dictionary from the translation file. However, if it's not translated, then we
    // must parse the input using base word dictionary.
    if (game.options[OPT_AUTOTRANSPARSERSAID])
    {
        checkwords = get_translation(checkwords);
        parser = GetTranslationTextParser();
    }
    else
    {
        parser = GetBaseTextParser();
    }

    if (!parser)
        return 0;

    ITextParser::ParsedPattern pattern;
    if (parser->ParsePattern(checkwords, pattern, MAX_PARSED_WORDS, nullptr))
    {
        return parser->MatchPattern(play.parsed_words, pattern) ? 1 : 0;
    }
    return 0;
}

int SaidUnknownWord(char *buffer)
{
    VALIDATE_STRING(buffer);
    snprintf(buffer, MAX_MAXSTRLEN, "%s", play.bad_parsed_word.GetCStr());
    if (play.bad_parsed_word.IsEmpty())
        return 0;
    return 1;
}

//=============================================================================

class TextParser : public ITextParser
{
public:
    TextParser(const WordsDictionary *dict, bool is_unicode, const String &locale_name);

    uint16_t FindWordInDictionary(const String &lookfor) const override;
    // Parses a simple sentence, e.g. a user input, and converts it to the word ID list
    bool ParseSentence(const String &src_text, ParsedSentence &words, size_t max_words, String *bad_parsed_word) override;
    // Parses input text and converts it into a pattern sequence, that may be matched with a user input
    bool ParsePattern(const String &src_text, ParsedPattern &pattern, size_t max_words, String *bad_parsed_word) override;
    // Matches the given pattern with the input sentence
    bool MatchPattern(const ParsedSentence &input, const ParsedPattern &pattern) override;

private:
    inline bool IsValidWordChar(int ch) const
    {
        return std::isalnum<wchar_t>(ch, _loc)
            // allow following punctuation chars to be a part of the word:
            || (ch == '\'') || (ch == '-') || (ch == '_');
    }

    // TODO: consider refactor & move this class to common utils
    struct Utf8StringView
    {
        const char *Ptr = nullptr;
        size_t CharIndex = 0u;

        Utf8StringView() = default;
        Utf8StringView(const char *cstr)
            : Ptr(cstr)
            , CharIndex(0u)
        {}
    };

    // Returns the next text character
    int PeekNextChar(Utf8StringView &text_ptr) const;
    // Scans forward, skip any whitespace characters, advances text pointer to the
    // character following the last whitespace, returns that character code
    int SkipWhitespace(Utf8StringView &text_ptr) const;
    // Scans forward, finds best matching word or multi-word (always chooses the
    // *longest* matching word), advances text pointer to the character following
    // the found word, returns the found word.
    String ScanWordOrMultiWord(Utf8StringView &text_ptr) const;
    // Reads characters until reaching a non-word character, returns resulting word
    String ReadWord(Utf8StringView &text_ptr) const;

    // Adds the last parsed word to the words sequence, which may contain 1 or more words
    void AddToWordSequence();
    // Finalize current words sequence, convert and record as word IDs
    void FinalizeWordSequence(bool optional, bool alternate, String *bad_parsed_word);

    const WordsDictionary *_dict;
    bool _isUnicode = false;
    std::locale _loc;
    String _parsedWord;
    std::vector<String> _wordSequence;
    ParsedPattern _pattern;
};

TextParser::TextParser(const WordsDictionary *dict, bool is_unicode, const String &locale_name)
    : _dict(dict)
    , _isUnicode(is_unicode)
    , _loc(GetLocaleSafe(locale_name.GetCStr()))
{
}

uint16_t TextParser::FindWordInDictionary(const String &lookfor) const
{
    if (_dict == nullptr)
        return WordsDictionary::INVALIDWORD;

    uint16_t word_id = _dict->FindWord(lookfor);
    if (word_id != WordsDictionary::INVALIDWORD)
        return word_id;
    return WordsDictionary::INVALIDWORD;
}

int TextParser::PeekNextChar(Utf8StringView &text_ptr) const
{
    return ugetc(text_ptr.Ptr);
}

int TextParser::SkipWhitespace(Utf8StringView &text_ptr) const
{
    int text_char = 0;
    Utf8StringView prev_ptr = text_ptr;
    for (prev_ptr = text_ptr, text_char = ugetxc(&text_ptr.Ptr), ++text_ptr.CharIndex;
        std::isspace<wchar_t>(text_char, _loc);
        prev_ptr = text_ptr, text_char = ugetxc(&text_ptr.Ptr), ++text_ptr.CharIndex);
    text_ptr = prev_ptr;
    return text_char;
}

String TextParser::ScanWordOrMultiWord(Utf8StringView &text_ptr) const
{
    Utf8StringView after_best_word_ptr = text_ptr;
    String full_word, best_word;
    while (*text_ptr.Ptr)
    {
        SkipWhitespace(text_ptr);
        String word = ReadWord(text_ptr);
        if (word.IsEmpty())
            break;

        // NOTE: this actually assumes that multi-words must have strictly 1 space between words
        if (!full_word.IsEmpty())
            full_word.AppendChar(' ');
        full_word.Append(word);
        // if nothing matches, then we'll return the first found word (it will be detected as invalid later);
        // if something else matches, then record that instead
        if (best_word.IsEmpty() ||
            (FindWordInDictionary(full_word) != WordsDictionary::INVALIDWORD))
        {
            after_best_word_ptr = text_ptr;
            best_word = full_word;
        }
    }
    text_ptr = after_best_word_ptr;
    return best_word;
}

String TextParser::ReadWord(Utf8StringView &text_ptr) const
{
    Utf8StringView begin_ptr = text_ptr;
    Utf8StringView prev_ptr = text_ptr;
    int text_char = 0;
    for (prev_ptr = text_ptr, text_char = ugetxc(&text_ptr.Ptr), ++text_ptr.CharIndex;
        IsValidWordChar(text_char);
        prev_ptr = text_ptr, text_char = ugetxc(&text_ptr.Ptr), ++text_ptr.CharIndex);
    text_ptr = prev_ptr;
    return String(begin_ptr.Ptr, text_ptr.Ptr - begin_ptr.Ptr);
}

bool TextParser::ParseSentence(const String &src_text, ParsedSentence &words, size_t max_words, String *bad_parsed_word)
{
    // For the purpose of parsing user input, we ignore the advanced constructs,
    // such as optional or alternative words.
    ParsedPattern pattern;
    if (ParsePattern(src_text, pattern, max_words, bad_parsed_word))
    {
        for (const auto &item : pattern)
        {
            if (item.second & ITextParser::kPatternItemOptional)
                continue;
            if (item.second & ITextParser::kPatternItemAltNext)
                continue;
            words.push_back(item.first);
        }
        return true;
    }
    return false;
}

bool TextParser::ParsePattern(const String &src_text, ParsedPattern &pattern, size_t max_words, String *bad_parsed_word)
{
    String uniform_text = src_text;
    _isUnicode ? uniform_text.MakeLowerUTF8() : uniform_text.MakeLower();
    Utf8StringView text_ptr(uniform_text.GetCStr());
    int text_char = 0;
    bool is_optional = false, is_alternate = false;
    _pattern = {};

    do
    {
        text_char = PeekNextChar(text_ptr);
        size_t text_char_index = text_ptr.CharIndex;
        if (std::isspace<wchar_t>(text_char, _loc))
        {
            // Finalize accumulated word sequence
            FinalizeWordSequence(is_optional, is_alternate, bad_parsed_word);
            is_alternate = false; // finish alternate sequence if there was one

            text_char = SkipWhitespace(text_ptr);
            text_char_index = text_ptr.CharIndex - 1;
        }

        // Optional word sequence begin
        if (text_char == '[')
        {
            if (is_optional)
            {
                debug_script_warn("TextParser: invalid nested '[' at index %zu\n\tText: %s", text_char_index, src_text.GetCStr());
                return false;
            }
            // Finalize accumulated word sequence
            FinalizeWordSequence(is_optional, is_alternate, bad_parsed_word);
            is_optional = true;
            is_alternate = false;
        }
        // Optional word sequence end
        else if (text_char == ']')
        {
            if (!is_optional)
            {
                debug_script_warn("TextParser: unexpected ']' at index %zu\n\tText: %s", text_char_index, src_text.GetCStr());
                return false;
            }
            else if (_wordSequence.size() == 0)
            {
                debug_script_warn("TextParser: empty optional word around index %zu\n\tText: %s", text_char_index, src_text.GetCStr());
            }
            // Finalize accumulated word sequence
            FinalizeWordSequence(is_optional, is_alternate, bad_parsed_word);
            is_optional = false;
            is_alternate = false;
        }
        // Alternate word sequence begin/continue
        else if (text_char == ',')
        {
            if ((_wordSequence.size() == 0) && _parsedWord.IsEmpty())
            {
                debug_script_warn("TextParser: unexpected ',' at index %zu\n\tText: %s", text_char_index, src_text.GetCStr());
                return false;
            }
            // Add previous word to the words sequence
            AddToWordSequence();
            is_alternate = true;
        }
        // Unsupported non-word char
        else if (!IsValidWordChar(text_char))
        {
            char uch[Utf8::UtfSz + 1]{};
            usetc(uch, text_char);
            debug_script_warn("TextParser: unexpected '%s' at index %zu\n\tText: %s", uch, text_char_index, src_text.GetCStr());
            return false;
        }
        // Beginning of the next word
        else
        {
            // Scan forward and find the longest known word or multi-word
            _parsedWord = ScanWordOrMultiWord(text_ptr);
            continue; // skip char advance
        }

        text_ptr.Ptr++;
        text_ptr.CharIndex++;
    }
    while (*text_ptr.Ptr);

    if (is_optional)
    {
        debug_script_warn("TextParser: no closing ']' for optional word\n\tText: %s", src_text.GetCStr());
    }

    // In case we have pending parsed words
    FinalizeWordSequence(is_optional, is_alternate, bad_parsed_word);

    pattern = _pattern;
    return true;
}

bool TextParser::MatchPattern(const ParsedSentence &input, const ParsedPattern &pattern)
{
    if (pattern.size() == 0)
        return false; // there's nothing to match

    size_t pattern_i = 0u;
    for (const auto &in : input)
    {
        if (in == WordsDictionary::IGNOREWORD)
            continue;
        // If we have reached a pattern end while not matching all input, this means a failure.
        if (pattern_i == pattern.size())
            return false;
        // ROL means we ignore the rest of the input;
        // if we got there, we consider the input matching the pattern
        if (pattern[pattern_i].first == WordsDictionary::RESTOFLINE)
            return true;

        // Try if we get any matches for this input in the pattern;
        // note that if there are optional items in the pattern, then we might skip some entries.
        bool has_match = false;
        bool try_next = false;
        do
        {
            try_next = false;
            // Test if next input word matches the next pattern item,
            // or if next pattern item is ANYWORD
            has_match = (in == pattern[pattern_i].first) || (pattern[pattern_i].first == WordsDictionary::ANYWORD);
            // If it failed to match, then try few other options
            // First try if the pattern has alternatives which may match
            if (!has_match && (pattern[pattern_i].second & kPatternItemAltFirst))
            {
                size_t alt_i = pattern_i;
                while ((++alt_i < pattern.size()) && (pattern[alt_i].second & kPatternItemAltNext))
                {
                    if ((in == pattern[alt_i].first) || (pattern[alt_i].first == WordsDictionary::ANYWORD))
                    {
                        has_match = true;
                        break;
                    }
                }
            }
            // Second, try if the current pattern item is optional
            if (!has_match && (pattern[pattern_i].second & kPatternItemOptional))
            {
                try_next = true;
            }

            // Advance to the next pattern item;
            // skip any alternates for this pattern item (regardless of whether it was optional or not)
            while ((++pattern_i < pattern.size()) && (pattern[pattern_i].second & kPatternItemAltNext));
        }
        while (try_next && pattern_i < pattern.size());

        if (!has_match)
            return false; // failure
    }

    // If we have reached a input's end while not matching all the required pattern, - this means a failure.
    for (; pattern_i < pattern.size(); ++pattern_i)
    {
        if ((pattern[pattern_i].first != WordsDictionary::RESTOFLINE) &&
            ((pattern[pattern_i].second & kPatternItemOptional) == 0))
            return false;
    }

    return true; // success
}

void TextParser::AddToWordSequence()
{
    if (!_parsedWord.IsEmpty())
        _wordSequence.push_back(_parsedWord);
    _parsedWord.Empty();
}

void TextParser::FinalizeWordSequence(bool optional, bool alternate, String *bad_parsed_word)
{
    // Add last parsed word if it was not added yet
    if (!_parsedWord.IsEmpty())
        AddToWordSequence();

    bool next_is_alt = false;
    for (const auto& word : _wordSequence)
    {
        uint16_t word_id = FindWordInDictionary(word);
        // We add any words to the pattern here, including invalid word id.
        // The only way invalid words can be matched though is by ANYWORD or ROL.
        int flags = kPatternItemOptional * optional | (kPatternItemAltFirst * alternate * !next_is_alt)
            | (kPatternItemAltNext * alternate * next_is_alt);
        _pattern.push_back(std::make_pair(word_id, static_cast<PatternItemFlags>(flags)));
        // Remember the first invalid word, in case script will ask about "unknown words"
        if (word_id == WordsDictionary::INVALIDWORD)
        {
            if (bad_parsed_word && bad_parsed_word->IsEmpty())
                *bad_parsed_word = word;
        }
        next_is_alt = alternate;
    }
    _wordSequence.clear();
}

std::unique_ptr<ITextParser> gl_BaseTextParser;
std::unique_ptr<ITextParser> gl_TranslationTextParser;

std::unique_ptr<ITextParser> CreateTextParser(WordsDictionary *dict, bool is_unicode, const String &locale_name)
{
    return std::unique_ptr<ITextParser>(new TextParser(dict, is_unicode, locale_name));
}

void SetBaseTextParser(std::unique_ptr<ITextParser> parser)
{
    gl_BaseTextParser = std::move(parser);
}

void SetTranslationTextParser(std::unique_ptr<ITextParser> parser)
{
    gl_TranslationTextParser = std::move(parser);
}

ITextParser *GetBaseTextParser()
{
    return gl_BaseTextParser.get();
}

ITextParser *GetTranslationTextParser()
{
    return gl_TranslationTextParser.get();
}

void MergeParserDictionary(WordsDictionary *dst_dict, const WordsDictionary *src_dict)
{
    std::unordered_map<uint16_t, std::vector<String>> word_lists;
    for (const auto src_word : src_dict->GetWords())
    {
        word_lists[src_word.second].push_back(src_word.first);
    }

    for (const auto dst_word : dst_dict->GetWords())
    {
        if (word_lists.count(dst_word.second) > 0)
            word_lists.erase(dst_word.second);
    }

    for (const auto src_word_group : word_lists)
    {
        for (const auto src_word : src_word_group.second)
            dst_dict->GetWords().insert(std::make_pair(src_word, src_word_group.first));
    }
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"
#include "ac/dynobj/scriptstring.h"

// int (const char *wordToFind)
RuntimeScriptValue Sc_Parser_FindWordID(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(Parser_FindWordID, const char);
}

// void  (char*text)
RuntimeScriptValue Sc_ParseText(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_POBJ(ParseText, /*const*/ char);
}

// const char* ()
RuntimeScriptValue Sc_Parser_SaidUnknownWord(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ(const char, myScriptStringImpl, Parser_SaidUnknownWord);
}

// int  (char*checkwords)
RuntimeScriptValue Sc_Said(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(Said, const char);
}



void RegisterParserAPI()
{
    ScFnRegister parser_api[] = {
        { "Parser::FindWordID^1",     API_FN_PAIR(Parser_FindWordID) },
        { "Parser::ParseText^1",      API_FN_PAIR(ParseText) },
        { "Parser::SaidUnknownWord^0",API_FN_PAIR(Parser_SaidUnknownWord) },
        { "Parser::Said^1",           API_FN_PAIR(Said) },
    };

    ccAddExternalFunctions(parser_api);
}
