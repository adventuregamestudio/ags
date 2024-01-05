//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// This is a collection of common implementations of the IScriptObject
// interface. Intended to be used as parent classes for majority of the
// script object managers.
//
// CCBasicObject: parent for managers that treat object contents as raw
// byte buffer.
//
// AGSCCDynamicObject, extends CCBasicObject: parent for built-in dynamic
// object managers; provides simplier serialization methods working with
// streams instead of a raw memory buffer.
//
// AGSCCStaticObject, extends CCBasicObject: a formal stub, intended as
// a parent for built-in static object managers.
//
//=============================================================================
#ifndef __AC_CCDYNAMICOBJECT_H
#define __AC_CCDYNAMICOBJECT_H

#include "ac/dynobj/cc_scriptobject.h"

namespace AGS { namespace Common { class Stream; } }


// CCBasicObject: basic implementation of the script object interface,
// intended to be used as a parent for object/manager classes that do not
// require specific implementation.
// * Dispose ignored, never deletes any data on its own;
// * Serialization skipped, does not save or load anything;
// * Provides default implementation for reading and writing data fields,
//   treats the contents of an object as a raw byte buffer.
struct CCBasicObject : public IScriptObject
{
public:
    virtual ~CCBasicObject() = default;

    // Dispose the object
    int Dispose(void* /*address*/, bool /*force*/) override;
    // Serialize the object into BUFFER (which is BUFSIZE bytes)
    // return number of bytes used
    int Serialize(void* /*address*/, uint8_t* /*buffer*/, int /*bufsize*/) override;

    //
    // Legacy support for reading and writing object fields by their relative offset
    //
    void *GetFieldPtr(void* address, intptr_t offset) override;
    void Read(void *address, intptr_t offset, uint8_t *dest, size_t size) override;
    uint8_t ReadInt8(void *address, intptr_t offset) override;
    int16_t ReadInt16(void *address, intptr_t offset) override;
    int32_t ReadInt32(void *address, intptr_t offset) override;
    float ReadFloat(void *address, intptr_t offset) override;
    void Write(void *address, intptr_t offset, const uint8_t *src, size_t size) override;
    void WriteInt8(void *address, intptr_t offset, uint8_t val) override;
    void WriteInt16(void *address, intptr_t offset, int16_t val) override;
    void WriteInt32(void *address, intptr_t offset, int32_t val) override;
    void WriteFloat(void *address, intptr_t offset, float val) override;
};


// AGSCCDynamicObject: standard parent implementation for the built-in
// script objects/manager.
// * Serialization from a raw buffer; provides a virtual function that
//   accepts Stream, to be implemented in children instead.
// * Provides Unserialize interface that accepts Stream.
struct AGSCCDynamicObject : public CCBasicObject
{
public:
    virtual ~AGSCCDynamicObject() = default;

    // TODO: pass savegame format version
    int Serialize(void *address, uint8_t *buffer, int bufsize) override;
    // Try unserializing the object from the given input stream
    virtual void Unserialize(int index, AGS::Common::Stream *in, size_t data_sz) = 0;

protected:
    // Savegame serialization
    // Calculate and return required space for serialization, in bytes
    virtual size_t CalcSerializeSize(const void *address) = 0;
    // Write object data into the provided stream
    virtual void Serialize(const void *address, AGS::Common::Stream *out) = 0;
};


// CCStaticObject is a base class for managing static global objects in script.
// The static objects can never be disposed, and do not support serialization
// through IScriptObject interface.
struct AGSCCStaticObject : public CCBasicObject
{
public:
    virtual ~AGSCCStaticObject() = default;

    const char *GetType() override { return "StaticObject"; }
};


extern AGSCCStaticObject GlobalStaticManager;

#endif // __AC_CCDYNAMICOBJECT_H
