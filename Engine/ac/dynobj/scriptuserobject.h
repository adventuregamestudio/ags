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
//
// Managed object, which size and contents are defined by user script
//
//=============================================================================
#ifndef __AGS_EE_DYNOBJ__SCRIPTUSERSTRUCT_H
#define __AGS_EE_DYNOBJ__SCRIPTUSERSTRUCT_H

#include "ac/dynobj/cc_agsdynamicobject.h"

struct ScriptUserObject : ICCDynamicObject
{
public:
    ScriptUserObject();
    virtual ~ScriptUserObject();

    static ScriptUserObject *CreateManaged(size_t size);
    void            Create(const char *data, size_t size);

    // return the type name of the object
    virtual const char *GetType();
    virtual int Dispose(const char *address, bool force);
    // serialize the object into BUFFER (which is BUFSIZE bytes)
    // return number of bytes used
    virtual int Serialize(const char *address, char *buffer, int bufsize);
    virtual void Unserialize(int index, const char *serializedData, int dataSize);

    // Support for reading and writing object values by their relative offset
    virtual void    Read(const char *address, intptr_t offset, void *dest, int size);
    virtual uint8_t ReadInt8(const char *address, intptr_t offset);
    virtual int16_t ReadInt16(const char *address, intptr_t offset);
    virtual int32_t ReadInt32(const char *address, intptr_t offset);
    virtual float   ReadFloat(const char *address, intptr_t offset);
    virtual void    Write(const char *address, intptr_t offset, void *src, int size);
    virtual void    WriteInt8(const char *address, intptr_t offset, uint8_t val);
    virtual void    WriteInt16(const char *address, intptr_t offset, int16_t val);
    virtual void    WriteInt32(const char *address, intptr_t offset, int32_t val);
    virtual void    WriteFloat(const char *address, intptr_t offset, float val);

private:
    // NOTE: we use signed int for Size at the moment, because the managed
    // object interface's Serialize() function requires the object to return
    // negative value of size in case the provided buffer was not large
    // enough. Since this interface is also a part of Plugin API, we would
    // need more significant change to program before we could use different
    // approach.
    int32_t  _size;
    char    *_data;
};

#endif // __AGS_EE_DYNOBJ__SCRIPTUSERSTRUCT_H
