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
// IScriptObject: script managed object interface.
// Provides interaction with a object which allocation and lifetime is
// managed by the engine and/or the managed pool rather than the script VM.
// These may be both static objects existing throughout the game, and
// dynamic objects allocated by the script command.
//
//=============================================================================
#ifndef __CC_SCRIPTOBJECT_H
#define __CC_SCRIPTOBJECT_H

#include <utility>
#include "core/types.h"


struct IScriptObject;

// A convenience struct for grouping handle and dynamic object
struct DynObjectRef
{
    const int Handle = 0;
    void * const Obj = nullptr;
    IScriptObject * const Mgr = nullptr;

    DynObjectRef() = default;
    DynObjectRef(int handle, void *obj, IScriptObject *mgr)
        : Handle(handle), Obj(obj), Mgr(mgr) {}
    inline operator bool() const { return Handle > 0; }
};


struct IScriptObject
{
    // WARNING: The first section of this interface is also a part of the AGS plugin API!

    // when a ref count reaches 0, this is called with the address
    // of the object. Return 1 to remove the object from memory, 0 to
    // leave it
    // The "force" flag tells system to detach the object, breaking any links and references
    // to other managed objects or game resources (instead of disposing these too).
    // TODO: it might be better to rewrite the managed pool and remove this flag at all,
    // because it makes the use of this interface prone to mistakes.
    virtual int Dispose(void *address, bool force = false) = 0;
    // return the type name of the object
    virtual const char *GetType() = 0;
    // serialize the object into BUFFER (which is BUFSIZE bytes)
    // return number of bytes used
    // TODO: pass savegame format version
    virtual int Serialize(void *address, uint8_t *buffer, int bufsize) = 0;

    // WARNING: following section is not a part of plugin API, therefore these methods
    // should **never** be called for kScValPluginObject script objects!

    // Legacy support for reading and writing object values by their relative offset.
    // These methods allow to "remap" script struct field access, by taking the
    // legacy offset, and using it rather as a field ID than an address, for example.
    // Consequently these also let trigger side-effects, such as updating an object
    // after a field value is written to.
    //
    // GetFieldPtr is used to access a script struct member which is a raw buffer,
    // such as a char array.
    // RE: GetFieldPtr() -
    // According to AGS script specification, when the old-string pointer or char array is passed
    // as an argument, the byte-code does not include any specific command for the member variable
    // retrieval and instructs to pass an address of the object itself with certain offset.
    // This results in functions like StrCopy writing directly over object address.
    // There may be other implementations, but the big question is: how to detect when this is
    // necessary, because byte-code does not contain any distinct operation for this case.
    // The worst thing here is that with the current byte-code structure we can never tell whether
    // offset 0 means getting pointer to whole object or a pointer to its first field.
    virtual void   *GetFieldPtr(void *address, intptr_t offset)               = 0;
    virtual void    Read(void *address, intptr_t offset, uint8_t *dest, size_t size) = 0;
    virtual uint8_t ReadInt8(void *address, intptr_t offset)                  = 0;
    virtual int16_t ReadInt16(void *address, intptr_t offset)                 = 0;
    virtual int32_t ReadInt32(void *address, intptr_t offset)                 = 0;
    virtual float   ReadFloat(void *address, intptr_t offset)                 = 0;
    virtual void    Write(void *address, intptr_t offset, const uint8_t *src, size_t size) = 0;
    virtual void    WriteInt8(void *address, intptr_t offset, uint8_t val)    = 0;
    virtual void    WriteInt16(void *address, intptr_t offset, int16_t val)   = 0;
    virtual void    WriteInt32(void *address, intptr_t offset, int32_t val)   = 0;
    virtual void    WriteFloat(void *address, intptr_t offset, float val)     = 0;

protected:
    IScriptObject() = default;
    ~IScriptObject() = default;
};

// The interface of a script objects deserializer that handles multiple types.
struct ICCObjectCollectionReader
{
    // TODO: pass savegame format version
    virtual void Unserialize(int32_t handle, const char *objectType, const char *serializedData, int dataSize) = 0;
};

// The interface of a script objects deserializer that handles a single type.
// WARNING: a part of the plugin API.
struct ICCObjectReader
{
    virtual void Unserialize(int32_t handle, const char *serializedData, int dataSize) = 0;
};

#endif // __CC_SCRIPTOBJECT_H
