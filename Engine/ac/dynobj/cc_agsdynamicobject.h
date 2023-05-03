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
// The common implementation for ICCDynamicObject interface.
// Intended to be used as a parent class for majority of the
// dynamic object managers.
//
// Basic implementation of:
// * Serialization from a raw buffer; provides a virtual function that
//   accepts Stream, to be implemented in children instead.
// * Provides Unserialize interface that accepts Stream.
// * Data Read/Write methods that treat the contents of the object as
//   a raw byte buffer.
//
//=============================================================================
#ifndef __AC_CCDYNAMICOBJECT_H
#define __AC_CCDYNAMICOBJECT_H

#include "ac/dynobj/cc_dynamicobject.h"

namespace AGS { namespace Common { class Stream; } }


struct AGSCCDynamicObject : ICCDynamicObject
{
protected:
    virtual ~AGSCCDynamicObject() = default;
public:
    // default implementation
    int Dispose(const char *address, bool force) override;

    // TODO: pass savegame format version
    int Serialize(const char *address, char *buffer, int bufsize) override;
    // Try unserializing the object from the given input stream
    virtual void Unserialize(int index, AGS::Common::Stream *in, size_t data_sz) = 0;

    // Legacy support for reading and writing object values by their relative offset
    const char* GetFieldPtr(const char *address, intptr_t offset) override;
    void    Read(const char *address, intptr_t offset, void *dest, int size) override;
    uint8_t ReadInt8(const char *address, intptr_t offset) override;
    int16_t ReadInt16(const char *address, intptr_t offset) override;
    int32_t ReadInt32(const char *address, intptr_t offset) override;
    float   ReadFloat(const char *address, intptr_t offset) override;
    void    Write(const char *address, intptr_t offset, void *src, int size) override;
    void    WriteInt8(const char *address, intptr_t offset, uint8_t val) override;
    void    WriteInt16(const char *address, intptr_t offset, int16_t val) override;
    void    WriteInt32(const char *address, intptr_t offset, int32_t val) override;
    void    WriteFloat(const char *address, intptr_t offset, float val) override;

protected:
    // Savegame serialization
    // Calculate and return required space for serialization, in bytes
    virtual size_t CalcSerializeSize(const char *address) = 0;
    // Write object data into the provided stream
    virtual void Serialize(const char *address, AGS::Common::Stream *out) = 0;
};

#endif // __AC_CCDYNAMICOBJECT_H