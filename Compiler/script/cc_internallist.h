#ifndef __CC_INTERNALLIST_H
#define __CC_INTERNALLIST_H

#define SCODE_META    (-2)   // meta tag follows
#define SCODE_INVALID (-1)   // this should never get added, so it's a fault
#define SMETA_LINENUM 1
#define SMETA_END 2
struct ccInternalList {
    int length;    // size of array, in ints
    int allocated; // memory allocated for array, in bytes
    long*script;
    int pos;
    int lineAtEnd;
    int cancelCurrentLine;  // whether to set currentline=-10 if end reached

    void startread();
    long peeknext();
    long getnext();
    void write(int value);
    // write a meta symbol (ie. non-code thingy)
    void write_meta(int type,int param);
    void shutdown();
    void init();
    ~ccInternalList();
    ccInternalList();
};

#endif // __CC_INTERNALLIST_H