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
// Managed script object interface.
//
//=============================================================================
#ifndef __CC_DYNAMICOBJECT_H
#define __CC_DYNAMICOBJECT_H

#include <utility>
#include "core/types.h"


// A pair of managed handle and abstract object pointer
typedef std::pair<int32_t, void*> DynObjectRef;


// OBJECT-BASED SCRIPTING RUNTIME FUNCTIONS
// interface
struct ICCDynamicObject {
    // when a ref count reaches 0, this is called with the address
    // of the object. Return 1 to remove the object from memory, 0 to
    // leave it
    // The "force" flag tells system to detach the object, breaking any links and references
    // to other managed objects or game resources (instead of disposing these too).
    // TODO: it might be better to rewrite the managed pool and remove this flag at all,
    // because it makes the use of this interface prone to mistakes.
    virtual int Dispose(const char *address, bool force = false) = 0;
    // return the type name of the object
    virtual const char *GetType() = 0;
    // serialize the object into BUFFER (which is BUFSIZE bytes)
    // return number of bytes used
    // TODO: pass savegame format version
    virtual int Serialize(const char *address, char *buffer, int bufsize) = 0;

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
    virtual const char* GetFieldPtr(const char *address, intptr_t offset)           = 0;
    virtual void    Read(const char *address, intptr_t offset, void *dest, int size) = 0;
    virtual uint8_t ReadInt8(const char *address, intptr_t offset)                  = 0;
    virtual int16_t ReadInt16(const char *address, intptr_t offset)                 = 0;
    virtual int32_t ReadInt32(const char *address, intptr_t offset)                 = 0;
    virtual float   ReadFloat(const char *address, intptr_t offset)                 = 0;
    virtual void    Write(const char *address, intptr_t offset, void *src, int size) = 0;
    virtual void    WriteInt8(const char *address, intptr_t offset, uint8_t val)    = 0;
    virtual void    WriteInt16(const char *address, intptr_t offset, int16_t val)   = 0;
    virtual void    WriteInt32(const char *address, intptr_t offset, int32_t val)   = 0;
    virtual void    WriteFloat(const char *address, intptr_t offset, float val)     = 0;

protected:
    ICCDynamicObject() = default;
    ~ICCDynamicObject() = default;
};

struct ICCObjectReader {
    // TODO: pass savegame format version
    virtual void Unserialize(int index, const char *objectType, const char *serializedData, int dataSize) = 0;
};
struct ICCStringClass {
    virtual DynObjectRef CreateString(const char *fromText) = 0;
};

#endif // __CC_DYNAMICOBJECT_H
