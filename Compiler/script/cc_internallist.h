#ifndef __CC_INTERNALLIST_H
#define __CC_INTERNALLIST_H

#include <map>
#include <vector>

#include "cs_parser_common.h"

namespace AGS
{
class LineHandler
{
    // maps sections to IDs
    class SectionMap
    {
    private:
        std::string _cacheSection = "";
        size_t _cacheId;
        std::vector <std::string> _section;

    public:
        size_t Section2Id(std::string const &section);
        std::string const Id2Section(size_t id) const { return id < _section.size() ? _section[id] : ""; }
        SectionMap();
    } _sectionMap;

    // maps srclist indexes to lines of the program source
    std::map<size_t, size_t> _lineStartTable;
    mutable size_t _cacheLineStart, _cacheLineEnd, _cacheLineNo;

    // maps srclist indexes to sections of the program source
    std::map<size_t, size_t> _sectionIdTable;
    mutable size_t _cacheSectionIdStart, _cacheSectionIdEnd, _cacheSectionId;
    
public:
    inline int Section2Id(std::string const &section) { return _sectionMap.Section2Id(section); }
    inline std::string const Id2Section(int id) const { return _sectionMap.Id2Section(id); }

    LineHandler();

    // Record that the named section begins at the offset of the SymbolList, 
    void AddSectionAt(size_t offset, std::string const &section);

    // Get the section number that corresponds to the offset
    size_t GetSectionIdAt(size_t offset) const;

    // Record that the code line lineno begins at the offset of the SymbolList, 
    void AddLineAt(size_t offset, size_t lineno);
        
    // Get the code line that corresponds to the offset
    size_t GetLineAt(size_t offset) const;
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
    size_t _len;
    size_t _cursor; // note: cursor has the actual position. Don't add _offset to cursor.

    int lineAtEnd;

public:
    SrcList(std::vector<Symbol> &script, LineHandler &line_handler);
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
    inline size_t GetLineno(size_t idx) { return _lineHandler.GetLineAt(idx + _offset); }
    inline size_t GetLineno() { return _lineHandler.GetLineAt(_cursor); }
    inline size_t GetSectionId(size_t idx) { return _lineHandler.GetSectionIdAt(idx + _offset); }
    inline size_t GetSectionId() { return _lineHandler.GetSectionIdAt(_cursor); }

    inline int Section2Id(std::string const &section) { return _lineHandler.Section2Id(section); }
    inline std::string const Id2Section(int id) const { return _lineHandler.Id2Section(id); }

    //
    // Used for building the srclist, not for processing the srclist
    //
    inline void Append(Symbol symb) { _script.push_back(symb); _len++; }
    inline void NewLine(size_t lineno) { _lineHandler.AddLineAt(_script.size(), lineno); }
    inline void NewSection(std::string const &section) { _lineHandler.AddSectionAt(_script.size(), section); }
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
    void ccInternalList::write(AGS::Symbol value);

    // Append the meta command
    void ccInternalList::write_meta(AGS::Symbol type, int param);

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
