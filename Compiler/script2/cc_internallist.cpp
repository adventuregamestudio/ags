#include <vector>
#include <string>
#include <algorithm>
#include <limits>
#include <stdlib.h>
#include "cc_internallist.h"

static int currentline;  

AGS::LineHandler::LineHandler()
    : _sections()
    , _lineStartTable()
    , _cacheLineStart(1)    // Invalidate the cache
    , _cacheLineEnd(0)      // Invalidate the cache
    , _cacheSectionLine({})
{
    _sections.push_back("");
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
    _lineStartTable[offset].SectionId = _sections.size() - 1;
    _lineStartTable[offset].Lineno = lineno;
    // The cache may no longer be correct, so invalidate it
    _cacheLineStart = 1;
    _cacheLineEnd = 0;
}

AGS::SrcList::SrcList(std::vector<Symbol> &script, LineHandler &line_handler, size_t &cursor)
    : _script(script)
    , _lineHandler(line_handler)
    , _offset(0)
    , _len(script.size())
    , _cursor(cursor)
{
}

AGS::SrcList::SrcList(SrcList const &src_list, size_t offset, size_t len)
    : _script(src_list._script)
    , _lineHandler(src_list._lineHandler)
    , _offset(offset + src_list._offset)
    , _cursor(src_list._cursor)
{
    _len = len;
    // _len mustn't be so high that the number of bytes left in the underlying script
    // are exceeded
    size_t const script_size = _script.size();
    size_t const rest_of_script_size = (script_size < offset) ? 0 : script_size - offset;
    if (_len > rest_of_script_size)
        _len = rest_of_script_size;

    // _len mustn't be so high that the number of bytes left in the SrcList are exceeded
    size_t const src_len = src_list._len;
    size_t const rest_of_src_len = (src_len < offset) ? 0 : src_len - offset;
    if (_len > rest_of_src_len)
        _len = rest_of_src_len;
}

AGS::Symbol AGS::SrcList::GetNext()
{
    Symbol const p = PeekNext();
    if (!ReachedEOF())
        _cursor++;
    return p;
}

void AGS::SrcList::EatFirstSymbols(size_t num)
{
    if (_len < num)
        num = _len;
    _len -= num;
    _offset += num;
    if (_cursor < _offset)
        _cursor = _offset;
}
