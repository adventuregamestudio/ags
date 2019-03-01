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

#include <stdlib.h>
#include <string.h>
#include "ac/dynobj/managedobjectpool.h"
#include "ac/dynobj/cc_dynamicarray.h" // globalDynamicArray, constants
#include "debug/out.h"
#include "util/string_utils.h"               // fputstring, etc
#include "script/cc_error.h"
#include "script/script_common.h"
#include "util/stream.h"

using namespace AGS::Common;

void ManagedObjectPool::Init(int32_t theHandle, const char *theAddress,
                                            ICCDynamicObject *theCallback, ScriptValueType objType) {
    auto & o = objects[theHandle];

    o.obj_type = objType;
    o.handle = theHandle;
    o.addr = theAddress;
    o.callback = theCallback;
    o.refCount = 0;

    handleByAddress[theAddress] = theHandle;

    ManagedObjectLog("Allocated managed object handle=%d, type=%s", o.theHandle, o.theCallback->GetType());
}

int ManagedObjectPool::Remove(int32_t handle, bool force) {

    auto & o = objects[handle];

    if ((o.callback != NULL) && (o.callback->Dispose(o.addr, force) == 0) &&
        (force == false))
        return 0;

    ManagedObjectLog("Line %d Disposing managed object handle=%d", o.currentline, o.handle);

    auto it = handleByAddress.find(o.addr);
    if (it != handleByAddress.end()) {
        handleByAddress.erase(it);
    } 

    o.handle = 0;
    o.addr = 0;
    o.callback = nullptr;

    return 1;
}

int32_t ManagedObjectPool::AddRef(int32_t handle) {
    auto & o = objects[handle];

    o.refCount += 1;
    ManagedObjectLog("Line %d AddRef: handle=%d new refcount=%d", o.currentline, o.handle, o.refCount);
    return o.refCount;
}

int ManagedObjectPool::CheckDispose(int32_t handle) {
    auto & o = objects[handle];

    if ((o.refCount < 1) && (o.callback != NULL)) {
        return Remove(handle);
    }
    return 0;
}

int32_t ManagedObjectPool::SubRef(int32_t handle) {
    auto & o = objects[handle];

    if ((disableDisposeForObject != NULL) && (o.addr == disableDisposeForObject)) {
        o.refCount--;
        ManagedObjectLog("Line %d SubRefNoDispose: handle=%d new refcount=%d", o.currentline, o.handle, o.refCount);
    } else {
        o.refCount--;
        ManagedObjectLog("Line %d SubRef: handle=%d new refcount=%d", o.currentline, o.handle, o.refCount);
        CheckDispose(handle);
    }
    return o.refCount;
}

int32_t ManagedObjectPool::AddressToHandle(const char *addr) {
    try {
        return handleByAddress.at(addr);
    } catch (std::out_of_range) {
        return 0;
    }
}

const char* ManagedObjectPool::HandleToAddress(int32_t handle) {
    // this function is called often (whenever a pointer is used)
    if ((handle < 1) || (handle >= arrayAllocLimit))
        return NULL;
    if (objects[handle].handle == 0)
        return NULL;
    return objects[handle].addr;
}

ScriptValueType ManagedObjectPool::HandleToAddressAndManager(int32_t handle, void *&object, ICCDynamicObject *&manager) {
    object = NULL;
    manager = NULL;
    // this function is called often (whenever a pointer is used)
    if ((handle < 1) || (handle >= arrayAllocLimit))
        return kScValUndefined;
    if (objects[handle].handle == 0)
        return kScValUndefined;
    object = (void*)objects[handle].addr;
    manager = objects[handle].callback;
    return objects[handle].obj_type;
}

int ManagedObjectPool::RemoveObject(const char *address) {
    int32_t handl = AddressToHandle(address);
    if (handl == 0)
        return 0;

    Remove(handl, true);

    return 1;
}

void ManagedObjectPool::RunGarbageCollectionIfAppropriate()
{
    if (objectCreationCounter > GARBAGE_COLLECTION_INTERVAL)
    {
        objectCreationCounter = 0;
        RunGarbageCollection();
    }
}

void ManagedObjectPool::RunGarbageCollection()
{
    ManagedObjectLog("Running garbage collection");

    for (int i = 1; i < numObjects; i++) 
    {
        if ((objects[i].refCount < 1) && (objects[i].callback != NULL)) 
        {
            Remove(i);
        }
    }
}

int ManagedObjectPool::AddObject(const char *address, ICCDynamicObject *callback, bool plugin_object, int useSlot) {
    if (useSlot == -1)
        useSlot = numObjects;

    objectCreationCounter++;

    if (useSlot < arrayAllocLimit) {
        // still space in the array, so use it
        Init(useSlot, address, callback, plugin_object ? kScValPluginObject : kScValDynamicObject);
        if (useSlot == numObjects)
            numObjects++;
        return useSlot;
    }
    else {
        // array has been used up
        if (useSlot == numObjects) {
            // if adding new (not un-serializing) check for empty slot
            // check backwards, since newer objects don't tend to last
            // long
            for (int i = arrayAllocLimit - 1; i >= 1; i--) {
                if (objects[i].handle == 0) {
                    Init(i, address, callback, plugin_object ? kScValPluginObject : kScValDynamicObject);
                    return i;
                }
            }
        }
        // no empty slots, expand array
        while (useSlot >= arrayAllocLimit)
            arrayAllocLimit += ARRAY_INCREMENT_SIZE;

        objects = (ManagedObject*)realloc(objects, sizeof(ManagedObject) * arrayAllocLimit);
        memset(&objects[useSlot], 0, sizeof(ManagedObject) * ARRAY_INCREMENT_SIZE);
        Init(useSlot, address, callback, plugin_object ? kScValPluginObject : kScValDynamicObject);
        if (useSlot == numObjects)
            numObjects++;
        return useSlot;
    }
}

void ManagedObjectPool::WriteToDisk(Stream *out) {
    int serializeBufferSize = SERIALIZE_BUFFER_SIZE;
    char *serializeBuffer = (char*)malloc(serializeBufferSize);

    out->WriteInt32(OBJECT_CACHE_MAGIC_NUMBER);
    out->WriteInt32(1);  // version
    out->WriteInt32(numObjects);

    // use this opportunity to clean up any non-referenced pointers
    RunGarbageCollection();

    for (int i = 1; i < numObjects; i++) 
    {
        if ((objects[i].handle) && (objects[i].callback != NULL)) {
            // write the type of the object
            StrUtil::WriteCStr((char*)objects[i].callback->GetType(), out);
            // now write the object data
            int bytesWritten = objects[i].callback->Serialize(objects[i].addr, serializeBuffer, serializeBufferSize);
            if ((bytesWritten < 0) && ((-bytesWritten) > serializeBufferSize))
            {
                // buffer not big enough, re-allocate with requested size
                serializeBufferSize = -bytesWritten;
                serializeBuffer = (char*)realloc(serializeBuffer, serializeBufferSize);
                bytesWritten = objects[i].callback->Serialize(objects[i].addr, serializeBuffer, serializeBufferSize);
            }
            out->WriteInt32(bytesWritten);
            if (bytesWritten > 0)
                out->Write(serializeBuffer, bytesWritten);
            out->WriteInt32(objects[i].refCount);
        }
        else  // write empty string if we cannot serialize it
            out->WriteInt8(0); 
    }

    free(serializeBuffer);
}

int ManagedObjectPool::ReadFromDisk(Stream *in, ICCObjectReader *reader) {
    int serializeBufferSize = SERIALIZE_BUFFER_SIZE;
    char *serializeBuffer = (char*)malloc(serializeBufferSize);
    char typeNameBuffer[200];

    if (in->ReadInt32() != OBJECT_CACHE_MAGIC_NUMBER) {
        cc_error("Data was not written by ccSeralize");
        return -1;
    }

    if (in->ReadInt32() != 1) {
        cc_error("Invalid data version");
        return -1;
    }

    int numObjs = in->ReadInt32();

    if (numObjs >= arrayAllocLimit) {
        arrayAllocLimit = numObjs + ARRAY_INCREMENT_SIZE;
        free(objects);
        objects = (ManagedObject*)calloc(sizeof(ManagedObject), arrayAllocLimit);
    }
    numObjects = numObjs;

    for (int i = 1; i < numObjs; i++) {
        StrUtil::ReadCStr(typeNameBuffer, in, sizeof(typeNameBuffer));
        if (typeNameBuffer[0] != 0) {
            int numBytes = in->ReadInt32();
            if (numBytes > serializeBufferSize) {
                serializeBufferSize = numBytes;
                serializeBuffer = (char*)realloc(serializeBuffer, serializeBufferSize);
            }
            if (numBytes > 0)
                in->Read(serializeBuffer, numBytes);

            if (strcmp(typeNameBuffer, CC_DYNAMIC_ARRAY_TYPE_NAME) == 0)
            {
                globalDynamicArray.Unserialize(i, serializeBuffer, numBytes);
            }
            else
            {
                reader->Unserialize(i, typeNameBuffer, serializeBuffer, numBytes);
            }
            objects[i].refCount = in->ReadInt32();
        }
    }

    free(serializeBuffer);
    return 0;
}

void ManagedObjectPool::reset() {
    // de-allocate all objects
    for (int kk = 1; kk < arrayAllocLimit; kk++) {
        if (objects[kk].handle) {
            Remove(kk, true);
        }
    }
    memset(&objects[0], 0, sizeof(ManagedObject) * arrayAllocLimit);
    numObjects = 1;
}

ManagedObjectPool::ManagedObjectPool() {
    numObjects = 1;
    arrayAllocLimit = 10;
    objects = (ManagedObject*)calloc(sizeof(ManagedObject), arrayAllocLimit);
    disableDisposeForObject = NULL;
}

ManagedObjectPool pool;
