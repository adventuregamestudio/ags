
#include "util/string.h"
#include "script/cc_common.h"
#include "script/cc_macrotable.h"

using namespace AGS::Common;


void MacroTable::merge(MacroTable &others) {
    _macro_table.insert(others._macro_table.begin(), others._macro_table.end());
}
bool MacroTable::contains(const String &name) {
    return _macro_table.count(name) > 0;
}
String MacroTable::get_macro(const String &name) {
    if(_macro_table.count(name)) {
        return _macro_table[name];
    }
    return nullptr;
}
void MacroTable::add(const String &macroname, const String &value) {
    if (this->contains(macroname)) {
        cc_error("macro '%s' already defined",macroname.GetCStr());
        return;
    }

    _macro_table[macroname] = value;
}
void MacroTable::remove(String &macroname) {
    if (!this->contains(macroname)) {
        cc_error("MacroTable::Remove: macro '%s' not found", macroname.GetCStr());
        return;
    }
    _macro_table.erase(macroname);
}

void MacroTable::clear() {
    _macro_table.clear();
}
