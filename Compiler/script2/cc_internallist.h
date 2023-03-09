#ifndef __CC_INTERNALLIST_H
#define __CC_INTERNALLIST_H

#include <map>
#include <vector>
#include <algorithm>

#include "cs_parser_common.h"

namespace AGS
{
// A helper class that contains post-parsed list of sections,
// and a offset-to-section map, useful for finding a symbol's location.
class SectionList
{
    // list of sections
    std::vector<std::string> _sections;
    // starting line offset to section index
    std::map<size_t, size_t> _off2sec;
public:
    SectionList() = default;
    SectionList(const std::vector<std::string> &sections,
                const std::map<size_t, size_t> &off2sec)
        : _sections(sections)
        , _off2sec(off2sec)
    {}
    SectionList(const std::vector<std::string> &&sections,
                const std::map<size_t, size_t> &&off2sec)
        : _sections(std::move(sections))
        , _off2sec(std::move(off2sec))
    {}

    // Returns a full list of sections
    inline const std::vector<std::string> &GetSections() const { return _sections; }
    // Maps given line-start offset to the section; returns SIZE_MAX on failure
    inline size_t GetSectionIdAt(size_t off) const
    {
        auto it = _off2sec.upper_bound(off);
        return it != _off2sec.end() ? (--it)->second : SIZE_MAX;
    }
};


class LineHandler
{
    // stores section names as strings
    std::vector <std::string> _sections;

    struct SectionLine
    {
        size_t SectionId, Lineno;
    };

    // maps srclist indexes to lines of the program source
    std::map<size_t, SectionLine> _lineStartTable;
    mutable size_t _cacheLineStart, _cacheLineEnd;
    mutable SectionLine _cacheSectionLine;

    void UpdateCacheIfNecessary(size_t pos) const;
    
public:

    LineHandler();

    // Start a new section. From now on, all AddLineAt() will refer to that section.
    inline int NewSection(std::string const &section) { _sections.push_back(section); return 0; }

    // Get the section id that corresponds to the offset
    inline size_t GetSectionIdAt(size_t offset) const { UpdateCacheIfNecessary(offset); return _cacheSectionLine.SectionId; }

    // Convert section IDs to sections
    std::string const &SectionId2Section(size_t id) const { return id >= _sections.size() ? _sections[0] : _sections[id]; }

    // Record that the code line lineno begins at this offset.
    void AddLineAt(size_t offset, size_t lineno);
        
    // Get the code line that corresponds to the offset
    inline size_t GetLinenoAt(size_t offset) const {  UpdateCacheIfNecessary(offset); return _cacheSectionLine.Lineno; }

    // Generate a SectionList object, copying collected data
    SectionList CreateSectionList() const;
};

// A list of input tokens. Only _len tokens, beginning at _offset, are taken into
// consideration, the other tokens are treated as if they weren't there.
// The list can be traversed using GetNext() / PeekNext() or directly with [..]
class SrcList
{
public:
    static Symbol const kEOF = 0;

private:
    std::vector<Symbol> &_script;
    LineHandler &_lineHandler;
    size_t _offset; 
    size_t _len;    // note: _len has the length relative to [0], not relative to [_offset]
    size_t &_cursor; // note: _cursor has the position relative to [0], not relative to [_offset]

public:
    SrcList(std::vector<Symbol> &script, LineHandler &line_handler, size_t &cursor);

    // This uses the same symbols and the same cursor variable that src_list does;
    // but the resulting SrcList starts at [offset] and has the length len.
    // No symbols are actually copied.
    // NOTE: If you move the cursor of the new list then the cursor of the original list
    // moves by the same amount because the cursor variable is shared. This is intentional.
    SrcList(SrcList const &src_list, size_t offset, size_t len);

    inline size_t GetCursor() const { return _cursor - _offset; }
    inline void SetCursor(size_t idx) { _cursor = idx + _offset; }
    
    inline size_t Length() const { return _len; };
    inline bool ReachedEOF() const { return _cursor - _offset >= _len  || _cursor >= _script.size(); }

    // Set the cursor to the start of the list
    inline void StartRead() { SetCursor(0); }
    // Read the next symbol
    Symbol GetNext();
    // Look at the symbol that will be read next, but don't read it yet
    inline Symbol PeekNext() const { return ReachedEOF() ? kEOF : _script[_cursor]; }
    // Move the cursor back by 1 space.
    inline void BackUp() { size_t c = GetCursor(); SetCursor((c > 0u) ? c - 1u : 0u); }

    // Get symbol at idx. Moves the cursor, too.
    // Note: Can't assign through this operator, this is intentional.
    Symbol operator[](size_t idx) { SetCursor(idx); return GetNext(); }

    // Whether the index is within the list
    inline bool InRange(size_t idx) const { return idx < _len; }

    // Shorten *this
    void EatFirstSymbol();
    void EatLastSymbol();

    // Note that when this is a sub-list of an original list, the line numbers and sections
    // will still be relative to the original list. This is intentional.
    // (When the user gets an error, they want to know the "real" line where the error is,
    // not a line reference that is relative to an excerpt of their program.)
    inline size_t GetLinenoAt(size_t pos) const { return _lineHandler.GetLinenoAt(pos + _offset); }
    inline size_t GetSectionIdAt (size_t pos) const { return _lineHandler.GetSectionIdAt(pos + _offset); }
    // Get the data for the symbol that has been read last.
    // Note: The cursor points at the symbol that will be read next, but we need the data
    // of the symbol that has just been read. That's the symbol in front of the cursor.
    inline size_t GetLineno() const { return GetLinenoAt(std::max<size_t>(1u, _cursor) - 1); }
    inline size_t GetSectionId() const { return GetSectionIdAt(std::max<size_t>(1u, _cursor) - 1); }

    inline std::string const &SectionId2Section(size_t id) const { return _lineHandler.SectionId2Section(id); }

    //
    // Used for building the srclist, not for processing the srclist
    //
    inline void Append(Symbol symb) { _script.push_back(symb); _len++; }
    inline void NewLine(size_t lineno) { _lineHandler.AddLineAt(_script.size(), lineno); }
    inline void NewSection(std::string const &section) { _lineHandler.NewSection(section); }
};
} // namespace AGS

#endif // __CC_INTERNALLIST_H
