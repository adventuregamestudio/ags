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

#ifndef __AC_CCDYNAMICOBJECT_H
#define __AC_CCDYNAMICOBJECT_H

#include "ac/dynobj/cc_dynamicobject.h"

struct AGSCCDynamicObject : ICCDynamicObject {
public:
    // default implementation
    virtual int Dispose(const char *address, bool force);

    virtual void Unserialize(int index, const char *serializedData, int dataSize) = 0;

    // Legacy support for reading and writing object values by their relative offset
    virtual uint8_t GetPropertyUInt8(const char *address, intptr_t offset);
    virtual int16_t GetPropertyInt16(const char *address, intptr_t offset);
    virtual int32_t GetPropertyInt32(const char *address, intptr_t offset);
    virtual void    SetPropertyUInt8(const char *address, intptr_t offset, uint8_t value);
    virtual void    SetPropertyInt16(const char *address, intptr_t offset, int16_t value);
    virtual void    SetPropertyInt32(const char *address, intptr_t offset, int32_t value);

protected:
    int bytesSoFar;
    int totalBytes;
    char *serbuffer;

    void StartSerialize(char *sbuffer);
    void SerializeInt(int val);
    int  EndSerialize();
    void StartUnserialize(const char *sbuffer, int pTotalBytes);
    int  UnserializeInt();

};

#endif // __AC_CCDYNAMICOBJECT_H