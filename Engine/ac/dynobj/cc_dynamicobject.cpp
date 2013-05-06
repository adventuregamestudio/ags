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
// C-Script run-time interpreter (c) 2001 Chris Jones
//
// You must DISABLE OPTIMIZATIONS AND REGISTER VARIABLES in your compiler
// when compiling this, or strange results can happen.
//
// There is a problem with importing functions on 16-bit compilers: the
// script system assumes that all parameters are passed as 4 bytes, which
// ints are not on 16-bit systems. Be sure to define all parameters as longs,
// or join the 21st century and switch to DJGPP or Visual C++.
//
//=============================================================================

//#define DEBUG_MANAGED_OBJECTS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ac/dynobj/cc_dynamicobject.h"
#include "ac/dynobj/managedobjectpool.h"
#include "game/script_objects.h"
#include "script/cc_error.h"
#include "util/stream.h"

using AGS::Common::Stream;

ICCStringClass *stringClassImpl = NULL;

// set the class that will be used for dynamic strings
void ccSetStringClassImpl(ICCStringClass *theClass) {
    stringClassImpl = theClass;
}

// register a memory handle for the object and allow script
// pointers to point to it
int32_t ccRegisterManagedObject(const void *object, ICCDynamicObject *callback, bool plugin_object) {
    int32_t handl = pool.AddObject((const char*)object, callback, plugin_object);

#ifdef DEBUG_MANAGED_OBJECTS
    char bufff[200];
    sprintf(bufff,"Register managed object type '%s' handle=%d addr=%08X", ((callback == NULL) ? "(unknown)" : callback->GetType()), handl, object);
    write_log(bufff);
#endif

    return handl;
}

// register a de-serialized object
int32_t ccRegisterUnserializedObject(int index, const void *object, ICCDynamicObject *callback, bool plugin_object) {
    return pool.AddObject((const char*)object, callback, plugin_object, index);
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
void ccSerializeAllObjects(Stream *out) {
    pool.WriteToDisk(out);
}

// un-serialise all objects (will remove all currently registered ones)
int ccUnserializeAllObjects(Stream *in, ICCObjectReader *callback) {
    // un-register all existing objects, ready for the un-serialization
    ccUnregisterAllObjects();
    return pool.ReadFromDisk(in, callback);
}

// dispose the object if RefCount==0
void ccAttemptDisposeObject(int32_t handle) {
    if (pool.HandleToAddress(handle) != NULL)
        pool.CheckDispose(handle);
}

// translate between object handles and memory addresses
int32_t ccGetObjectHandleFromAddress(const char *address) {
    // set to null
    if (address == NULL)
        return 0;

    int32_t handl = pool.AddressToHandle(address);

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

const char *ccGetObjectAddressFromHandle(int32_t handle) {
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

ScriptValueType ccGetObjectAddressAndManagerFromHandle(int32_t handle, void *&object, ICCDynamicObject *&manager)
{
    if (handle == 0) {
        object = NULL;
        manager = NULL;
        return kScValUndefined;
    }
    ScriptValueType obj_type = pool.HandleToAddressAndManager(handle, object, manager);
    if (object == NULL) {
        cc_error("Error retrieving pointer: invalid handle %d", handle);
    }
    return obj_type;
}

int ccAddObjectReference(int32_t handle) {
    if (handle == 0)
        return 0;

    return pool.AddRef(handle);
}

int ccReleaseObjectReference(int32_t handle) {
    if (handle == 0)
        return 0;

    if (pool.HandleToAddress(handle) == NULL) {
        cc_error("Error releasing pointer: invalid handle %d", handle);
        return -1;
    }

    return pool.SubRef(handle);
}
