#include <vector>
#include <string>
#include <algorithm>
#include <limits>
#include <stdlib.h>
#include "cc_internallist.h"

// TODO Throw me away as soon as internallist has died!
static int currentline;  

AGS::LineHandler::LineHandler()
    : _cacheLineStart(1) // Invalidate the cache
    , _cacheLineEnd (0) // Invalidate the cache
{
    _sections = { "" };
    _lineStartTable.clear();

    // Add sentinels to the table for simpler lookup algorithms
    size_t const maxsize = std::numeric_limits<size_t>::max();
    _lineStartTable[0] = SectionLine{ 0, 0 };
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

AGS::SrcList::SrcList(std::vector<Symbol> &script, LineHandler &line_handler)
    : _script(script)
    , _lineHandler(line_handler)
    , _offset(0)
    , _len (script.size())
    , _cursor(0)
{
}

AGS::SrcList::SrcList(SrcList const &src_list, size_t offset, size_t len)
    : _script(src_list._script)
    , _lineHandler(src_list._lineHandler)
    , _offset(offset + src_list._offset)
    , _len(len)
    , _cursor(offset + src_list._offset)
{
    size_t const maxlen = std::max<int>(_script.size() - _offset, 0);
    _len = std::min<size_t>(_len, maxlen);
}

AGS::Symbol AGS::SrcList::GetNext()
{
    Symbol const p = PeekNext();
    if (!ReachedEOF())
        _cursor++;
    return p;
}

void ccInternalList::startread()
{
    pos = 0;
}

bool ccInternalList::isPosValid(int pos)
{
    return pos >= 0 && pos < length;
}

AGS::Symbol ccInternalList::peeknext()
{
    int tpos = pos;
    // this should work even if 3 bytes aren't remaining
    while (isPosValid(tpos) && (script[tpos] == SCODE_META))
        tpos += 3;

    if (isPosValid(tpos))
        return script[tpos];
    else
        return static_cast<AGS::Symbol>(SCODE_INVALID);
}


AGS::Symbol ccInternalList::getnext() {
    // process line numbers internally
    while (isPosValid(pos) && script[pos] == SCODE_META)
    {
        long bytesRemaining = length - pos;
        if (bytesRemaining >= 3)
        {
            if (script[pos + 1] == SMETA_LINENUM)
            {
                currentline = script[pos + 2];
            }
            else if (script[pos + 1] == SMETA_END)
            {
                lineAtEnd = currentline;
                if (cancelCurrentLine)
                {
                    currentline = -10;
                }
                // TODO DEFECT?: If we break, we return SCODE_META *and* increase pos, so next getnext will return SMETA_END.
                break;
            }
        }
        pos += 3;
    }
    if (pos >= length)
    {
        if (cancelCurrentLine)
            currentline = -10;
        return static_cast<AGS::Symbol>(SCODE_INVALID);
    }

    if (isPosValid(pos))
        return script[pos++];
    else
        return static_cast<AGS::Symbol>(SCODE_INVALID);
}


void ccInternalList::write(AGS::Symbol value) {
    if ((length + 1) * sizeof(AGS::Symbol) >= (unsigned long)allocated)
    {

        if (allocated < 64)
            allocated += 64;
        else
            allocated *= 2;

        script = static_cast<AGS::SymbolScript>(realloc(script, allocated));
    }
    script[length] = value;
    length++;
}

// write a meta symbol (ie. non-code thingy)
void ccInternalList::write_meta(AGS::Symbol type, int param) {
    write(SCODE_META);
    write(type);
    write(param);
}

void ccInternalList::shutdown() {
    if (script != NULL) { free(script); }
    script = NULL;
    length = 0;
    allocated = 0;
}

void ccInternalList::init() {
    allocated = 0;
    length = 0;
    script = NULL;
    pos = -1;
    lineAtEnd = -1;
    cancelCurrentLine = 1;
}

ccInternalList::~ccInternalList() {
    shutdown();
}

ccInternalList::ccInternalList() {
    init();
}

bool ccInternalList::reached_eof()
{
    if (peeknext() != SCODE_INVALID)
        return false;

    // We are past the last symbol in the file
    getnext();
    currentline = lineAtEnd;
    return true;
}
