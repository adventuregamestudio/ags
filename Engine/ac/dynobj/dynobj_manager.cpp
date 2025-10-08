//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "ac/dynobj/dynobj_manager.h"
#include <stdlib.h>
#include <string.h>
#include "ac/dynobj/managedobjectpool.h"
#include "debug/out.h"
#include "script/cc_common.h"
#include "util/stream.h"

using namespace AGS::Common;

// register a memory handle for the object and allow script
// pointers to point to it
int32_t ccRegisterManagedObject(void *object, IScriptObject *callback, ScriptValueType obj_type) {
    int32_t handl = pool.AddObject(object, callback, obj_type);

    ManagedObjectLog("Register managed object type '%s' handle=%d addr=%08X",
        ((callback == NULL) ? "(unknown)" : callback->GetType()), handl, object);

    return handl;
}

// register a de-serialized object
int32_t ccRegisterUnserializedObject(int index, void *object, IScriptObject *callback, ScriptValueType obj_type) {
    return pool.AddUnserializedObject(object, callback, obj_type, index);
}

// unregister a particular object
int ccUnRegisterManagedObject(void *object) {
    return pool.RemoveObject(object);
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
int ccUnserializeAllObjects(Stream *in, ICCObjectCollectionReader *callback) {
    return pool.ReadFromDisk(in, callback);
}

// dispose the object if RefCount==0
void ccAttemptDisposeObject(int32_t handle) {
    pool.CheckDispose(handle);
}

// translate between object handles and memory addresses
int32_t ccGetObjectHandleFromAddress(void *address) {
    // set to null
    if (address == nullptr)
        return 0;

    int32_t handl = pool.AddressToHandle(address);

    ManagedObjectLog("Line %d WritePtr: %08X to %d", currentline, address, handl);

    if (handl == 0) {
        cc_error("Pointer cast failure: the object being pointed to is not in the managed object pool");
        return -1;
    }
    return handl;
}

void *ccGetObjectAddressFromHandle(int32_t handle) {
    if (handle == 0) {
        return nullptr;
    }
    void *addr = pool.HandleToAddress(handle);

    ManagedObjectLog("Line %d ReadPtr: %d to %08X", currentline, handle, addr);

    if (addr == nullptr) {
        cc_error("Error retrieving pointer: invalid handle %d", handle);
        return nullptr;
    }
    return addr;
}

ScriptValueType ccGetObjectAddressAndManagerFromHandle(int32_t handle, void *&object, IScriptObject *&manager)
{
    if (handle == 0) {
        object = nullptr;
        manager = nullptr;
        return kScValUndefined;
    }
    ScriptValueType obj_type = pool.HandleToAddressAndManager(handle, object, manager);
    if (obj_type == kScValUndefined) {
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

    if (pool.HandleToAddress(handle) == nullptr) {
        cc_error("Error releasing pointer: invalid handle %d", handle);
        return -1;
    }

    return pool.SubRef(handle);
}

int ccAssignObjectHandle(void *address)
{
    int handle = ccGetObjectHandleFromAddress(address);
    if (handle > 0)
        ccAddObjectReference(handle);
    return handle;
}

int ccRemoveObjectHandle(int handle)
{
    if (handle > 0)
        ccReleaseObjectReference(handle);
    return 0;
}

int ccReplaceObjectHandle(int32_t old_handle, int32_t new_handle)
{
    if (old_handle == new_handle)
        return new_handle;

    if (old_handle > 0)
        ccReleaseObjectReference(old_handle);
    if (new_handle > 0)
        ccAddObjectReference(new_handle);
    return new_handle;
}

int ccReplaceObjectHandle(int32_t old_handle, void *new_address)
{
    return ccReplaceObjectHandle(old_handle, ccGetObjectHandleFromAddress(new_address));
}

void ccTraverseManagedObjects(const String &type, PfnProcessManagedObject callback)
{
    pool.TraverseManagedObjects(type, callback);
}
