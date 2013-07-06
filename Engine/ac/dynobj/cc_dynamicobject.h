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
// 'C'-style script compiler
//
//=============================================================================

#ifndef __CC_DYNAMICOBJECT_H
#define __CC_DYNAMICOBJECT_H

#include "core/types.h"
#include "script/runtimescriptvalue.h"

// Forward declaration
namespace AGS { namespace Common { class Stream; } }
using namespace AGS; // FIXME later


// OBJECT-BASED SCRIPTING RUNTIME FUNCTIONS
// interface
struct ICCDynamicObject {
    // when a ref count reaches 0, this is called with the address
    // of the object. Return 1 to remove the object from memory, 0 to
    // leave it
    virtual int Dispose(const char *address, bool force) = 0;
    // return the type name of the object
    virtual const char *GetType() = 0;
    // serialize the object into BUFFER (which is BUFSIZE bytes)
    // return number of bytes used
    virtual int Serialize(const char *address, char *buffer, int bufsize) = 0;

    // Legacy support for reading and writing object values by their relative offset
    virtual uint8_t GetPropertyUInt8(const char *address, intptr_t offset) = 0;
    virtual int16_t GetPropertyInt16(const char *address, intptr_t offset) = 0;
    virtual int32_t GetPropertyInt32(const char *address, intptr_t offset) = 0;
    virtual void    SetPropertyUInt8(const char *address, intptr_t offset, uint8_t value) = 0;
    virtual void    SetPropertyInt16(const char *address, intptr_t offset, int16_t value) = 0;
    virtual void    SetPropertyInt32(const char *address, intptr_t offset, int32_t value) = 0;
};

struct ICCObjectReader {
    virtual void Unserialize(int index, const char *objectType, const char *serializedData, int dataSize) = 0;
};
struct ICCStringClass {
    virtual void* CreateString(const char *fromText) = 0;
};

// set the class that will be used for dynamic strings
extern void  ccSetStringClassImpl(ICCStringClass *theClass);
// register a memory handle for the object and allow script
// pointers to point to it
extern int32_t ccRegisterManagedObject(const void *object, ICCDynamicObject *, bool plugin_object = false);
// register a de-serialized object
extern int32_t ccRegisterUnserializedObject(int index, const void *object, ICCDynamicObject *, bool plugin_object = false);
// unregister a particular object
extern int   ccUnRegisterManagedObject(const void *object);
// remove all registered objects
extern void  ccUnregisterAllObjects();
// serialize all objects to disk
extern void  ccSerializeAllObjects(Common::Stream *out);
// un-serialise all objects (will remove all currently registered ones)
extern int   ccUnserializeAllObjects(Common::Stream *in, ICCObjectReader *callback);
// dispose the object if RefCount==0
extern void  ccAttemptDisposeObject(int32_t handle);
// translate between object handles and memory addresses
extern int32_t ccGetObjectHandleFromAddress(const char *address);
extern const char *ccGetObjectAddressFromHandle(int32_t handle);
extern ScriptValueType ccGetObjectAddressAndManagerFromHandle(int32_t handle, void *&object, ICCDynamicObject *&manager);
extern void  ccReassignManagedObjectAddressRange(const char *range_start, const char *new_address, int count, int object_size);

extern int ccAddObjectReference(int32_t handle);
extern int ccReleaseObjectReference(int32_t handle);

extern ICCStringClass *stringClassImpl;

#endif // __CC_DYNAMICOBJECT_H
