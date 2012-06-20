
#include <stdlib.h>
#include "cc_internallist.h"

extern int currentline;

void ccInternalList::startread() {
    pos=0;
}
long ccInternalList::peeknext() {
    int tpos = pos;
    while ((tpos < length) && (script[tpos] == SCODE_META))
        tpos+=3;
    if (tpos >= length)
        return SCODE_INVALID;
    return script[tpos];
}
long ccInternalList::getnext() {
    // process line numbers internally
    while (script[pos] == SCODE_META) {
        if (script[pos+1] == SMETA_LINENUM)
            currentline = script[pos+2];
        else if (script[pos+1] == SMETA_END) {
            lineAtEnd = currentline;
            if (cancelCurrentLine)
                currentline = -10;
            break;
        }
        pos+=3;
    }
    if (pos >= length) {
        if (cancelCurrentLine)
            currentline = -10;
        return SCODE_INVALID;
    }
    /*    if ((script[pos] >= 0) && (sym.get_type(script[pos]) == SYM_OPENBRACE))
    nested_level++;
    if ((script[pos] >= 0) && (sym.get_type(script[pos]) == SYM_CLOSEBRACE))
    nested_level--;*/

    return script[pos++];
}
void ccInternalList::write(int value) {
    if ((length+1) * sizeof(long) >= (unsigned long)allocated) {

        if (allocated < 1000)
            allocated += 1000;
        else
            allocated *= 2;

        script = (long*)realloc(script, allocated);
        /*long*oldscript=script;
        script = (long*)malloc(allocated);
        if (oldscript != NULL) {
        memcpy(script,oldscript,(length * sizeof(long)));
        free(oldscript);
        }*/
    }
    script[length] = value;
    length ++;
}
// write a meta symbol (ie. non-code thingy)
void ccInternalList::write_meta(int type,int param) {
    write(SCODE_META);
    write(type);
    write(param);
}
void ccInternalList::shutdown() {
    if (script!=NULL) free(script);
    script = NULL;
    length = 0;
    allocated = 0;
}
void ccInternalList::init() {
    allocated = 0;
    length = 0;
    script = NULL;
    cancelCurrentLine = 1;
}
ccInternalList::~ccInternalList() {
    shutdown();
}
ccInternalList::ccInternalList() {
    init();
}
