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

#include <string.h>
#include "core/types.h"
#include "ac/dynobj/cc_agsdynamicobject.h"
#include "ac/common.h"               // quit()
#include "util/bbop.h"

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
#if defined (AGS_BIG_ENDIAN)
    AGS::Common::BitByteOperations::SwapBytesInt32(*iptr);
#endif
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
    int value = *((int*)chptr);
#if defined (AGS_BIG_ENDIAN)
    AGS::Common::BitByteOperations::SwapBytesInt32(value);
#endif
    return value;
}

uint8_t AGSCCDynamicObject::GetPropertyUInt8(const char *address, intptr_t offset)
{
    return *(uint8_t*)(address + offset);
}

int16_t AGSCCDynamicObject::GetPropertyInt16(const char *address, intptr_t offset)
{
    return *(uint16_t*)(address + offset);
}

int32_t AGSCCDynamicObject::GetPropertyInt32(const char *address, intptr_t offset)
{
    return *(uint32_t*)(address + offset);
}

void AGSCCDynamicObject::SetPropertyUInt8(const char *address, intptr_t offset, uint8_t value)
{
    *(uint8_t*)(address + offset) = value;
}

void AGSCCDynamicObject::SetPropertyInt16(const char *address, intptr_t offset, int16_t value)
{
    *(int16_t*)(address + offset) = value;
}

void AGSCCDynamicObject::SetPropertyInt32(const char *address, intptr_t offset, int32_t value)
{
    *(int32_t*)(address + offset) = value;
}
