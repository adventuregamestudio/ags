#include <vector>
#include <string>
#include <algorithm>
#include <limits>
#include <stdlib.h>
#include "cc_internallist.h"

extern int currentline;  // in script_common

AGS::LineHandler::SectionMap::SectionMap()
    : _cacheSection("")
    , _cacheId(0)
    , _section({ "" })
{
}

size_t AGS::LineHandler::SectionMap::Section2Id(std::string const &section)
{
    if (section == _cacheSection)
        return _cacheId;
    _cacheSection = section;
    auto const it = std::find(_section.begin(), _section.end(), section);
    _cacheId = it - _section.begin();
    if (_cacheId == _section.size()) // hasn't been entered in the table yet
        _section.push_back(section); 
    return _cacheId;
}

AGS::LineHandler::LineHandler()
    : _cacheLineStart(0)
    , _cacheLineEnd (0)
    , _cacheLineNo(0)
    , _cacheSectionIdStart(0)
    , _cacheSectionIdEnd(0)
    , _cacheSectionId(0)
{
    // Add sentinels to the table for simpler lookup algorithms
    size_t const maxsize = std::numeric_limits<size_t>::max();
    _lineStartTable[0] = 0;
    _lineStartTable[maxsize] = maxsize;
    _sectionIdTable[0] = 0;
    _sectionIdTable[maxsize] = maxsize;
}

void AGS::LineHandler::AddSectionAt(size_t offset, std::string const & section)
{
    _sectionIdTable[offset] = Section2Id(section);
    // This makes the cache invalid, so
    _cacheSectionIdStart = _cacheSectionIdEnd = _cacheSectionId = 0;
}

size_t AGS::LineHandler::GetLineAt(size_t offset) const
{
    if (_cacheLineStart <= offset && offset < _cacheLineEnd)
        return _cacheLineNo;

    auto it = _lineStartTable.upper_bound(offset);
    _cacheLineEnd = it->first;
    it--;
    _cacheLineStart = it->first;
    _cacheLineNo = it->second;
    return _cacheLineNo;
}

size_t AGS::LineHandler::GetSectionIdAt(size_t offset) const
{
    if (_cacheSectionIdStart <= offset && offset < _cacheSectionIdEnd)
        return _cacheSectionId;

    auto it = _sectionIdTable.upper_bound(offset);
    _cacheSectionIdEnd = it->first;
    _cacheSectionIdStart = _cacheSectionId = 0;
    it--;
    _cacheSectionIdStart = it->first;
    _cacheSectionId = it->second;
    return _cacheSectionId;
}

void AGS::LineHandler::AddLineAt(size_t offset, size_t lineno)
{
    _lineStartTable[offset] = lineno;
    // This makes the cache invalid, so
    _cacheLineStart = _cacheLineEnd = _cacheLineNo = 0;
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
    , _offset(offset)
    , _len(len)
    , _cursor(_offset)
{
    size_t const maxlen = std::max<int>(_script.size() - _offset, 0);
    _len = std::min<size_t>(_len, maxlen);
}

AGS::Symbol AGS::SrcList::GetNext()
{
    Symbol const p = PeekNext();
    if (!ReachedEOF())
        _cursor++;
    currentline = GetLineno();
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
