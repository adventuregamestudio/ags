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
#include <string>
#include <limits>
#include <stdlib.h>
#include "cc_internallist.h"

const AGS::Symbol AGS::SrcList::kEOF = 0;

static int currentline;  

AGS::LineHandler::LineHandler()
    : _sections()
    , _lineStartTable()
    , _cacheLineStart(1)    // Invalidate the cache
    , _cacheLineEnd(0)      // Invalidate the cache
    , _cacheSectionLine({})
{
    _sections.push_back("");
    _moduleNames.push_back("");
    // Add sentinels to the table for simpler lookup algorithms
    size_t const maxsize = std::numeric_limits<size_t>::max();
    _lineStartTable[0] = SectionLine{ 0, 1 };
    _lineStartTable[maxsize] = SectionLine{ 0, maxsize };
}

void AGS::LineHandler::UpdateCacheIfNecessary(size_t pos) const
{
    if (_cacheLineStart <= pos && pos < _cacheLineEnd)
        return;

    // Cache miss
    // We've entered a record for the largest possible line number at object creation time,
    // so we know that upper_bound(offset) will find a record.
    auto it = _lineStartTable.upper_bound(pos);
    _cacheLineEnd = it->first;
    _cacheLineStart = (--it)->first;
    _cacheSectionLine = SectionLine{ it->second.SectionId, it->second.Lineno };
}

void AGS::LineHandler::AddLineAt(size_t offset, size_t lineno)
{
    _lineStartTable[offset].SectionId = _sections.size() - 1u;
    _lineStartTable[offset].Lineno = lineno;
    // The cache may no longer be correct, so invalidate it
    _cacheLineStart = 1u;
    _cacheLineEnd = 0u;
}

AGS::SectionList AGS::LineHandler::CreateSectionList() const
{
    std::vector<std::string> sections = _sections;
    std::vector<std::string> module_names = _moduleNames;
    std::map<size_t, size_t> off2sec;
    for (const auto &item : _lineStartTable)
        off2sec[item.first] = item.second.SectionId;
    return AGS::SectionList(std::move(sections), std::move(module_names), std::move(off2sec));
}

AGS::SrcList::SrcList(std::vector<Symbol> &script, LineHandler &line_handler, size_t &cursor)
    : _script(&script)
    , _lineHandler(&line_handler)
    , _begin(0u)
    , _end(script.size())
    , _cursor(&cursor)
{
}

AGS::SrcList::SrcList(SrcList const &src_list, size_t const offset, size_t const len)
    : _script(src_list._script)
    , _lineHandler(src_list._lineHandler)
    , _begin(offset + src_list._begin)
    , _cursor(src_list._cursor)
{
    size_t const script_size = _script->size();

    if (_begin > script_size)
        _begin = _end = script_size;

    _end = _begin + len;
    if (_end > script_size)
        _end = script_size;
    if (_end > src_list._end)
        _end = src_list._end;
}

AGS::SrcList::SrcList(SrcList const &src_list)
    : _script(src_list._script)
    , _lineHandler(src_list._lineHandler)
    , _begin(src_list._begin)
    , _end(src_list._end)
    , _cursor(src_list._cursor)
{
}

AGS::SrcList &AGS::SrcList::operator=(SrcList const &other)
{
    if (this == &other)
        return *this;

    // This class does NOT own the things pointed to
    // so no attempts to release pointers
    this->_script = other._script;
    this->_lineHandler = other._lineHandler;
    this->_begin = other._begin;
    this->_end = other._end;
    this->_cursor = other._cursor;
    return *this;
}

AGS::SrcList &AGS::SrcList::operator=(SrcList const &&other) noexcept
{
    if (this == &other)
        return *this;

    // This class does NOT own the things pointed to
    // so no attempts to release pointers
    this->_script = other._script;
    this->_lineHandler = other._lineHandler;
    this->_begin = other._begin;
    this->_end = other._end;
    this->_cursor = other._cursor;
    return *this;
}

AGS::Symbol AGS::SrcList::GetNext()
{
    Symbol const p = PeekNext();
    if (*_cursor >= _begin && *_cursor < _end)
        ++(*_cursor); // Don't increment '_cursor'!
    return p;
}

void AGS::SrcList::SkipTo(SymbolList const &stoplist)
{
    int delimeter_nesting_depth = 0;
    for (; !ReachedEOF(); GetNext())
    {
        // Note that the scanner/tokenizer has already verified
        // that all opening symbols get closed and 
        // that we don't have (...] or similar in the input
        Symbol const next_sym = PeekNext();
        switch (next_sym)
        {
        case kKW_OpenBrace:
        case kKW_OpenBracket:
        case kKW_OpenParenthesis:
            ++delimeter_nesting_depth;
            continue;

        case kKW_CloseBrace:
        case kKW_CloseBracket:
        case kKW_CloseParenthesis:
            if (--delimeter_nesting_depth < 0)
                return;
            continue;

        }
        if (0 < delimeter_nesting_depth)
            continue;

        for (auto it = stoplist.begin(); it != stoplist.end(); ++it)
            if (next_sym == *it)
                return;
    }
}

void AGS::SrcList::EatFirstSymbol()
{
    if (++_begin > _end)
        _begin = _end;
    if (*_cursor < _begin)
        *_cursor = _begin;
}

void AGS::SrcList::EatLastSymbol()
{
    if (_end < 1u || _end < _begin)
        return;
    --_end;
}
