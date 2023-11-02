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
// IScriptObject: script managed object interface.
// Provides interaction with a object which allocation and lifetime is
// managed by the engine and/or the managed pool rather than the script VM.
// These may be both static objects existing throughout the game, and
// dynamic objects allocated by the script command.
//
//=============================================================================
#ifndef __CC_SCRIPTOBJECT_H
#define __CC_SCRIPTOBJECT_H

#include <functional>
#include <unordered_map>
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

    // When a ref count reaches 0, this is called with the address
    // of the object. Return 1 to remove the object from memory, 0 to
    // leave it.
    // FIXME: the return value is really never used in the engine, but
    // we cannot currently change to void without adjusting plugin API.
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


    //
    // Interface of a managed object that contains typeid fields.
    // Remap typeid fields using the provided map
    virtual void RemapTypeids(void *address,
        const std::unordered_map<uint32_t, uint32_t> &typeid_map) = 0;

    //
    // Inteface of a managed object that contains references to other objects.
    // Type of function that performs an operation over a managed handle
    typedef std::function<void(int handle)> PfnTraverseRefOp;
    // Traverse all managed references in this object, and run callback for each of them
    virtual void TraverseRefs(void *address, PfnTraverseRefOp traverse_op) = 0;


    // Legacy support for reading and writing object values by their relative offset.
    // WARNING: following were never a part of plugin API, therefore these methods
    // should **never** be called for kScValPluginObject script objects!
    //
    // RE: GetFieldPtr()
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

// The interface of a dynamic String allocator.
struct ICCStringClass
{
    virtual DynObjectRef CreateString(const char *fromText) = 0;
};

// The interface of a script objects deserializer that handles multiple types.
// Is supposed to create an object based on object type, and add one to the managed pool.
struct ICCObjectCollectionReader
{
    // TODO: pass savegame format version
    virtual void Unserialize(int32_t handle, const char *objectType, const char *serializedData, int dataSize) = 0;
};

// The interface of a script objects deserializer that handles a single type.
// Is supposed to create an object of the predefined type, and add one to the managed pool.
// WARNING: a part of the plugin API.
struct ICCObjectReader
{
    // TODO: find a way to pass savegame format version?
    virtual void Unserialize(int32_t handle, const char *serializedData, int dataSize) = 0;
};

#endif // __CC_SCRIPTOBJECT_H
