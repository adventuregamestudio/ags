/*
** 'C'-style script compiler
** Copyright (C) 2000-2001, Chris Jones
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE;
** the contents of this file may not be disclosed to third parties,
** copied or duplicated in any form, in whole or in part, without
** prior express permission from Chris Jones.
**
*/

#ifndef __CC_DYNAMICOBJECT_H
#define __CC_DYNAMICOBJECT_H


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
extern void  ccSerializeAllObjects(FILE *output);
// un-serialise all objects (will remove all currently registered ones)
extern int   ccUnserializeAllObjects(FILE *input, ICCObjectReader *callback);
// dispose the object if RefCount==0
extern void  ccAttemptDisposeObject(long handle);
// translate between object handles and memory addresses
extern long  ccGetObjectHandleFromAddress(const char *address);
extern const char *ccGetObjectAddressFromHandle(long handle);

extern int ccAddObjectReference(long handle);
extern int ccReleaseObjectReference(long handle);

extern ICCStringClass *stringClassImpl;

#endif // __CC_DYNAMICOBJECT_H