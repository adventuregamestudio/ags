//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================

#include "ac/dynobj/cc_agsdynamicobject.h"
#include "ac/common.h"               // quit()

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
