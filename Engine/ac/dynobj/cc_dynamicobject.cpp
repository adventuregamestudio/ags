/*
C-Script run-time interpreter (c) 2001 Chris Jones

You must DISABLE OPTIMIZATIONS AND REGISTER VARIABLES in your compiler
when compiling this, or strange results can happen.

There is a problem with importing functions on 16-bit compilers: the
script system assumes that all parameters are passed as 4 bytes, which
ints are not on 16-bit systems. Be sure to define all parameters as longs,
or join the 21st century and switch to DJGPP or Visual C++.

This is UNPUBLISHED PROPRIETARY SOURCE CODE;
the contents of this file may not be disclosed to third parties,
copied or duplicated in any form, in whole or in part, without
prior express permission from Chris Jones.
*/
//#define DEBUG_MANAGED_OBJECTS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ac/dynobj/cc_dynamicobject.h"
#include "ac/dynobj/managedobjectpool.h"
#include "script/cc_error.h"
#include "util/datastream.h"

using AGS::Common::CDataStream;

ICCStringClass *stringClassImpl = NULL;

// set the class that will be used for dynamic strings
void ccSetStringClassImpl(ICCStringClass *theClass) {
    stringClassImpl = theClass;
}

// register a memory handle for the object and allow script
// pointers to point to it
long ccRegisterManagedObject(const void *object, ICCDynamicObject *callback) {
    long handl = pool.AddObject((const char*)object, callback);

#ifdef DEBUG_MANAGED_OBJECTS
    char bufff[200];
    sprintf(bufff,"Register managed object type '%s' handle=%d addr=%08X", ((callback == NULL) ? "(unknown)" : callback->GetType()), handl, object);
    write_log(bufff);
#endif

    return handl;
}

// register a de-serialized object
long ccRegisterUnserializedObject(int index, const void *object, ICCDynamicObject *callback) {
    return pool.AddObject((const char*)object, callback, index);
}

// unregister a particular object
int ccUnRegisterManagedObject(const void *object) {
    return pool.RemoveObject((const char*)object);
}

// remove all registered objects
void ccUnregisterAllObjects() {
    pool.reset();
}

// serialize all objects to disk
void ccSerializeAllObjects(CDataStream *out) {
    pool.WriteToDisk(out);
}

// un-serialise all objects (will remove all currently registered ones)
int ccUnserializeAllObjects(CDataStream *in, ICCObjectReader *callback) {
    // un-register all existing objects, ready for the un-serialization
    ccUnregisterAllObjects();
    return pool.ReadFromDisk(in, callback);
}

// dispose the object if RefCount==0
void ccAttemptDisposeObject(long handle) {
    if (pool.HandleToAddress(handle) != NULL)
        pool.CheckDispose(handle);
}

// translate between object handles and memory addresses
long ccGetObjectHandleFromAddress(const char *address) {
    // set to null
    if (address == NULL)
        return 0;

    long handl = pool.AddressToHandle(address);

#ifdef DEBUG_MANAGED_OBJECTS
    char bufff[200];
    sprintf(bufff,"Line %d WritePtr: %08X to %d", currentline, address, handl);
    write_log(bufff);
#endif

    if (handl == 0) {
        cc_error("Pointer cast failure: the object being pointed to is not in the managed object pool");
        return -1;
    }
    return handl;
}

const char *ccGetObjectAddressFromHandle(long handle) {
    if (handle == 0) {
        return NULL;
    }
    const char *addr = pool.HandleToAddress(handle);

#ifdef DEBUG_MANAGED_OBJECTS
    char bufff[200];
    sprintf(bufff,"Line %d ReadPtr: %d to %08X", currentline, handle, addr);
    write_log(bufff);
#endif

    if (addr == NULL) {
        cc_error("Error retrieving pointer: invalid handle %d", handle);
        return NULL;
    }
    return addr;
}

int ccAddObjectReference(long handle) {
    if (handle == 0)
        return 0;

    return pool.AddRef(handle);
}

int ccReleaseObjectReference(long handle) {
    if (handle == 0)
        return 0;

    if (pool.HandleToAddress(handle) == NULL) {
        cc_error("Error releasing pointer: invalid handle %d", handle);
        return -1;
    }

    return pool.SubRef(handle);
}
