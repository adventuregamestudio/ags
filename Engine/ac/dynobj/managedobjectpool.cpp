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

const auto OBJECT_CACHE_MAGIC_NUMBER = 0xa30b;
const auto SERIALIZE_BUFFER_SIZE = 10240;
const auto GARBAGE_COLLECTION_INTERVAL = 1024;
const auto RESERVED_SIZE = 2048;

int ManagedObjectPool::Remove(int32_t handle, bool force) {
    auto & o = objects[handle];

    bool canBeRemovedFromPool = o.callback->Dispose(o.addr, force);
    if (!(canBeRemovedFromPool || force)) { return 0; }

    handleByAddress.erase(o.addr);
    objects.erase(handle);
    // <-- obj reference invalid at this point.
    ManagedObjectLog("Line %d Disposed managed object handle=%d", currentline, handle);

    return 1;
}

int32_t ManagedObjectPool::AddRef(int32_t handle) {
    auto & o = objects[handle];

    o.refCount += 1;
    ManagedObjectLog("Line %d AddRef: handle=%d new refcount=%d", currentline, o.handle, o.refCount);
    return o.refCount;
}

int ManagedObjectPool::CheckDispose(int32_t handle) {
    auto & o = objects[handle];
    if (o.refCount >= 1) { return 0; }
    return Remove(handle);
}

int32_t ManagedObjectPool::SubRef(int32_t handle) {
    auto & o = objects[handle];

    o.refCount--;
    auto newRefCount = o.refCount;
    auto canBeDisposed = (o.addr != disableDisposeForObject);
    if (canBeDisposed) {
        CheckDispose(handle);
    }
    // <-- obj reference invalid at this point.
    ManagedObjectLog("Line %d SubRef: handle=%d new refcount=%d canBeDisposed=%d", currentline, handle, newRefCount, canBeDisposed);
    return newRefCount;
}

int32_t ManagedObjectPool::AddressToHandle(const char *addr) {
    return handleByAddress.at(addr);
}

// this function is called often (whenever a pointer is used)
const char* ManagedObjectPool::HandleToAddress(int32_t handle) {
    auto it = objects.find(handle);
    if (it == objects.end()) { return nullptr; }
    auto & o = it->second;
    return o.addr;
}

// this function is called often (whenever a pointer is used)
ScriptValueType ManagedObjectPool::HandleToAddressAndManager(int32_t handle, void *&object, ICCDynamicObject *&manager) {
    auto it = objects.find(handle);
    if (it == objects.end()) { return kScValUndefined; }
    auto & o = it->second;

    object = (void *)o.addr;  // WARNING: This strips the const from the char* pointer.
    manager = o.callback;
    return o.obj_type;
}

int ManagedObjectPool::RemoveObject(const char *address) {
    int32_t handl = AddressToHandle(address);
    if (handl == 0) { return 0; }
    Remove(handl, true);
    return 1;
}

void ManagedObjectPool::RunGarbageCollectionIfAppropriate()
{
    if (objectCreationCounter <= GARBAGE_COLLECTION_INTERVAL) { return; }
    RunGarbageCollection();
    objectCreationCounter = 0;
}

void ManagedObjectPool::RunGarbageCollection()
{
    for (auto it = objects.begin(); it != objects.end() /* not hoisted */; /* no increment */)
    {
        auto & o = it->second;
        it++; // iterator is invalid _after_ erase, so increment now.
        if (o.refCount < 1) {
            Remove(o.handle);
        }
    }
    ManagedObjectLog("Ran garbage collection");
}

inline int handle_increment(int handle) {
    if (handle >= INT_MAX || handle <= 0) {
        return 1;
    }
    return handle + 1;
}

int ManagedObjectPool::AddObject(const char *address, ICCDynamicObject *callback, bool plugin_object, int useSlot) 
{
    int handle;

    if (useSlot >= 0) {
        if (objects.find(useSlot) != objects.end()) {
            cc_error("Slot used: %d", useSlot);
            return -1;
        }
        handle = useSlot;
    } else {
        handle = nextHandle;
        while (objects.find(handle) != objects.end()) {
            handle = handle_increment(handle);
        }
    }

    objects.insert({handle, {
        ManagedObject {
            /* obj_type: */ plugin_object ? kScValPluginObject : kScValDynamicObject,
            /* handle: */ handle,
            /* addr: */ address,
            /* callback: */ callback,
            /* refCount: */ 0
        }
    }});
    handleByAddress.insert({address, handle});

    nextHandle = handle_increment(handle);
    objectCreationCounter++;

    ManagedObjectLog("Allocated managed object handle=%d, type=%s", handle, callback->GetType());

    return handle;
}

void ManagedObjectPool::WriteToDisk(Stream *out) {

    // use this opportunity to clean up any non-referenced pointers
    RunGarbageCollection();

    int serializeBufferSize = SERIALIZE_BUFFER_SIZE;
    char *serializeBuffer = (char*)malloc(serializeBufferSize);

    out->WriteInt32(OBJECT_CACHE_MAGIC_NUMBER);
    out->WriteInt32(2);  // version
    out->WriteInt32(objects.size());

    for( const auto& it : objects ) {
        auto & o = it.second;

        // handle
        out->WriteInt32(o.handle);
        // write the type of the object
        StrUtil::WriteCStr((char*)o.callback->GetType(), out);
        // now write the object data
        int bytesWritten = o.callback->Serialize(o.addr, serializeBuffer, serializeBufferSize);
        if ((bytesWritten < 0) && ((-bytesWritten) > serializeBufferSize))
        {
            // buffer not big enough, re-allocate with requested size
            serializeBufferSize = -bytesWritten;
            serializeBuffer = (char*)realloc(serializeBuffer, serializeBufferSize);
            bytesWritten = o.callback->Serialize(o.addr, serializeBuffer, serializeBufferSize);
        }
        assert(bytesWritten >= 0);
        out->WriteInt32(bytesWritten);
        out->Write(serializeBuffer, bytesWritten);
        out->WriteInt32(o.refCount);

        ManagedObjectLog("Wrote handle = %d", o.handle);
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

    auto version = in->ReadInt32();

    switch (version) {
        case 1:
            {
                // IMPORTANT: numObjs is "nextHandleId", which is why we iterate from 1 to numObjs-1
                int numObjs = in->ReadInt32();
                for (int i = 1; i < numObjs; i++) {
                    StrUtil::ReadCStr(typeNameBuffer, in, sizeof(typeNameBuffer));
                    if (typeNameBuffer[0] != 0) {
                        int numBytes = in->ReadInt32();
                        if (numBytes > serializeBufferSize) {
                            serializeBufferSize = numBytes;
                            serializeBuffer = (char*)realloc(serializeBuffer, serializeBufferSize);
                        }
                        assert(serializeBuffer != nullptr);
                        in->Read(serializeBuffer, numBytes);
                        if (strcmp(typeNameBuffer, CC_DYNAMIC_ARRAY_TYPE_NAME) == 0) {
                            globalDynamicArray.Unserialize(i, serializeBuffer, numBytes);
                        } else {
                            reader->Unserialize(i, typeNameBuffer, serializeBuffer, numBytes);
                        }
                        objects[i].refCount = in->ReadInt32();
                        ManagedObjectLog("Read handle = %d", objects[i].handle);
                    }
                }
            }
            break;
        case 2:
            {
                // This is actually number of objects written.
                int objectsSize = in->ReadInt32();
                for (int i = 0; i < objectsSize; i++) {
                    auto handle = in->ReadInt32();
                    assert (handle >= 1);
                    StrUtil::ReadCStr(typeNameBuffer, in, sizeof(typeNameBuffer));
                    assert (typeNameBuffer[0] != 0);
                    int numBytes = in->ReadInt32();
                    assert (numBytes >= 0);
                    if (numBytes > serializeBufferSize) {
                        serializeBufferSize = numBytes;
                        serializeBuffer = (char*)realloc(serializeBuffer, serializeBufferSize);
                    }
                    assert(serializeBuffer != nullptr);
                    in->Read(serializeBuffer, numBytes);
                    if (strcmp(typeNameBuffer, CC_DYNAMIC_ARRAY_TYPE_NAME) == 0) {
                        globalDynamicArray.Unserialize(handle, serializeBuffer, numBytes);
                    } else {
                        reader->Unserialize(handle, typeNameBuffer, serializeBuffer, numBytes);
                    }
                    objects[handle].refCount = in->ReadInt32();
                    ManagedObjectLog("Read handle = %d", objects[i].handle);
                }
            }
            break;
        default:
            cc_error("Invalid data version: %d", version);
            return -1;
    }

    free(serializeBuffer);
    return 0;
}

// de-allocate all objects
void ManagedObjectPool::reset() {
    for (auto it = objects.begin(); it != objects.end() /* not hoisted */; /* no increment */) {
        auto & o = it->second;
        it++; // iterator is invalid _after_ erase, so increment now.
        Remove(o.handle, true);
    }
    assert(objects.empty());
}

ManagedObjectPool::ManagedObjectPool() {
    objects.reserve(RESERVED_SIZE);
    handleByAddress.reserve(RESERVED_SIZE);
}

ManagedObjectPool pool;
