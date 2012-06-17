
#include <stdio.h>
#include "acmain/ac_maindefines.h"
#include "acrun/ac_ccdynamicobject.h"
#include "ac/ac_common.h"

// *** The script serialization routines for built-in types

int AGSCCDynamicObject::Dispose(const char *address, bool force) {
    // cannot be removed from memory
    return 0;
}

void AGSCCDynamicObject::StartSerialize(char *sbuffer) {
    bytesSoFar = 0;
    serbuffer = sbuffer;
}

void AGSCCDynamicObject::SerializeInt(int val) {
    char *chptr = &serbuffer[bytesSoFar];
    int *iptr = (int*)chptr;
    *iptr = val;
    bytesSoFar += 4;
}

int AGSCCDynamicObject::EndSerialize() {
    return bytesSoFar;
}

void AGSCCDynamicObject::StartUnserialize(const char *sbuffer, int pTotalBytes) {
    bytesSoFar = 0;
    totalBytes = pTotalBytes;
    serbuffer = (char*)sbuffer;
}

int AGSCCDynamicObject::UnserializeInt() {
    if (bytesSoFar >= totalBytes)
        quit("Unserialise: internal error: read past EOF");

    char *chptr = &serbuffer[bytesSoFar];
    bytesSoFar += 4;
    int *iptr = (int*)chptr;
    return *iptr;
}
