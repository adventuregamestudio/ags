#ifndef __CC_INTERNALLIST_H
#define __CC_INTERNALLIST_H

#include "cs_parser_common.h"

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
