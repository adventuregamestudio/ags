#ifndef __CC_INTERNALLIST_H
#define __CC_INTERNALLIST_H

#include <map>
#include <vector>
#include <algorithm>

#include "cs_parser_common.h"

namespace AGS
{
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

    // Get the section number that corresponds to the offset
    inline size_t GetSectionIdAt(size_t offset) const { UpdateCacheIfNecessary(offset); return _cacheSectionLine.SectionId; }

    // Convert section IDs to sections
    std::string const &SectionId2Section(size_t id) const { return (id < 0 || id >= _sections.size()) ? _sections[0] : _sections[id]; }

    // Record that the code line lineno begins at the offset of the SymbolList, 
    void AddLineAt(size_t offset, size_t lineno);
        
    // Get the code line that corresponds to the offset
    inline size_t GetLinenoAt(size_t offset) const {  UpdateCacheIfNecessary(offset); return _cacheSectionLine.Lineno; }
};

// A list of input tokens. Only _len tokens, beginning at _offset, are taken into
// consideration, the other tokens are treated as if they weren't there.
// The list can be traversed using GetNext() / PeekNext() or directly with [..]
class SrcList
{
public:
    static int const kEOF = -2;

private:
    std::vector<Symbol> &_script;
    LineHandler &_lineHandler;
    size_t _offset; 
    size_t _len;    // note: _len has the length relative to [0], not relative to [_offset]
    size_t _cursor; // note: _cursor has the position relative to [0], not relative to [_offset]

    int lineAtEnd;

public:
    SrcList(std::vector<Symbol> &script, LineHandler &line_handler);

    // This references the symbol list of src_list but makes it look as if it would start at [offset].
    // Note: src_list must live at least as long as "this" or else "this" will have a dangling reference. 
    SrcList(SrcList const &src_list, size_t offset, size_t len);

    inline size_t GetCursor() const { return _cursor - _offset; }
    inline void SetCursor(size_t idx) { _cursor = idx + _offset; }

    inline size_t GetSize() const { return _len - _offset; };
    
    inline bool ReachedEOF() const { return _cursor - _offset >= _len  || _cursor >= _script.size(); }
    inline Symbol PeekNext() const { return ReachedEOF() ? kEOF : _script[_cursor]; }
    Symbol GetNext();

    inline bool InRange(size_t idx) const { return idx < _len && idx + _offset < _script.size(); }

    // Note: Can't assign through this operator, this is intentional.
    inline Symbol operator[](size_t idx) const { return InRange(idx) ? _script[idx + _offset] : kEOF; }

    // Note that when this is a sub-list of an original list, the line numbers
    // will still be relative to the original list. This is intentional.
    inline size_t GetLinenoAt(size_t pos) const { return _lineHandler.GetLinenoAt(pos + _offset); }
    inline size_t GetSectionIdAt (size_t pos) const { return _lineHandler.GetSectionIdAt(pos + _offset); }
    // Get the data to the current position.
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

#define SCODE_META    (-2)   // meta tag follows
#define SCODE_INVALID (-1)   // this should never get added, so it's a fault
#define SMETA_LINENUM (1)
#define SMETA_END (2)


struct ccInternalList {
    int length;    // size of array, in ints
    int allocated; // memory allocated for array, in bytes
    AGS::SymbolScript script;
    int pos;
    int lineAtEnd;
    int cancelCurrentLine;  // whether to set currentline=-10 if end reached

    // Reset the cursor to the start of the list
    void startread();

    // Have a look at the next symbol without eating it
    AGS::Symbol peeknext();

    // Eat and get the next symbol, update global current_line
    AGS::Symbol getnext();  // and update global current_line

    // Append the value
    void write(AGS::Symbol value);

    // Append the meta command
    void write_meta(AGS::Symbol type, int param);

    // Free internal memory allocated
    void shutdown();

    void init();

    ~ccInternalList();

    ccInternalList();

    // Whether input exhausted
    bool reached_eof();

private:
    bool isPosValid(int pos);
};

#endif // __CC_INTERNALLIST_H
