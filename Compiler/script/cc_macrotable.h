#ifndef __CC_MACROTABLE_H
#define __CC_MACROTABLE_H

#include <map>
#include "util/string.h"

typedef AGS::Common::String AGString;

#define MAX_LINE_LENGTH 500

struct MacroTable {
private:
    std::map<AGString,AGString> _macro_table;
public:
    bool contains(const AGString &name);
    AGString get_macro(const AGString &name) ;
    void add(const AGString &macroname, const AGString &value);
    void remove(AGString &macroname);
    void merge(MacroTable & macro_table);
    void clear();
};

#endif // __CC_MACROTABLE_H