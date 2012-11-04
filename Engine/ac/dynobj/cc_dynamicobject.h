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

#include "util/file.h"

// Forward declaration
namespace AGS { namespace Common { class DataStream; } }
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
    virtual void    Read(const char *address, int offset, void *dest, int size) = 0;
    virtual uint8_t ReadInt8(const char *address, long offset)                  = 0;
    virtual int16_t ReadInt16(const char *address, long offset)                 = 0;
    virtual int32_t ReadInt32(const char *address, long offset)                 = 0;
    virtual float   ReadFloat(const char *address, long offset)                 = 0;
    virtual void    Write(const char *address, int offset, void *src, int size) = 0;
    virtual void    WriteInt8(const char *address, long offset, uint8_t val)    = 0;
    virtual void    WriteInt16(const char *address, long offset, int16_t val)   = 0;
    virtual void    WriteInt32(const char *address, long offset, int32_t val)   = 0;
    virtual void    WriteFloat(const char *address, long offset, float val)     = 0;
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
extern long  ccRegisterManagedObject(const void *object, ICCDynamicObject *);
// register a de-serialized object
extern long  ccRegisterUnserializedObject(int index, const void *object, ICCDynamicObject *);
// unregister a particular object
extern int   ccUnRegisterManagedObject(const void *object);
// remove all registered objects
extern void  ccUnregisterAllObjects();
// serialize all objects to disk
extern void  ccSerializeAllObjects(Common::DataStream *out);
// un-serialise all objects (will remove all currently registered ones)
extern int   ccUnserializeAllObjects(Common::DataStream *in, ICCObjectReader *callback);
// dispose the object if RefCount==0
extern void  ccAttemptDisposeObject(long handle);
// translate between object handles and memory addresses
extern long  ccGetObjectHandleFromAddress(const char *address);
extern const char *ccGetObjectAddressFromHandle(long handle);
extern void  ccGetObjectAddressAndManagerFromHandle(long handle, void *&object, ICCDynamicObject *&manager);

extern int ccAddObjectReference(long handle);
extern int ccReleaseObjectReference(long handle);

extern ICCStringClass *stringClassImpl;

#endif // __CC_DYNAMICOBJECT_H
