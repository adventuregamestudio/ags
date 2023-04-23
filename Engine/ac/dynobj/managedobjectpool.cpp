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
#include <vector>
#include <string.h>
#include "ac/dynobj/managedobjectpool.h"
#include "debug/out.h"
#include "util/string_utils.h"               // fputstring, etc
#include "script/cc_common.h"
#include "util/memorystream.h"
#include "util/stream.h"

using namespace AGS::Common;

const auto OBJECT_CACHE_MAGIC_NUMBER = 0xa30b;
const auto OBJECT_CACHE_SAVE_VERSION = 2;
const auto SERIALIZE_BUFFER_SIZE = 10240;
const auto GARBAGE_COLLECTION_INTERVAL = 1024;
const auto RESERVED_SIZE = 2048;

int ManagedObjectPool::Remove(ManagedObject &o, bool force) {
    if (!o.isUsed()) { return 1; } // already removed

    o.refCount = 0; // mark as disposing, to avoid any access
    o.callback->Dispose(o.addr, force); // we always dispose and remove now!
    available_ids.push(o.handle);
    handleByAddress.erase(o.addr);
    ManagedObjectLog("Line %d Disposed managed object handle=%d", currentline, o.handle);
    o = ManagedObject();
    return 1;
}

int32_t ManagedObjectPool::AddRef(int32_t handle) {
    if (handle < 0 || (size_t)handle >= objects.size()) { return 0; }
    auto & o = objects[handle];
    if (!o.isUsed()) { return 0; }

    o.refCount++;
    ManagedObjectLog("Line %d AddRef: handle=%d new refcount=%d", currentline, o.handle, o.refCount);
    return o.refCount;
}

int ManagedObjectPool::CheckDispose(int32_t handle) {
    if (handle < 0 || (size_t)handle >= objects.size()) { return 1; }
    auto & o = objects[handle];
    if (!o.isUsed()) { return 1; }
    if (o.refCount >= 1) { return 0; }
    gcUsedList.erase(o.gcItUsed);
    return Remove(o);
}

int32_t ManagedObjectPool::SubRefCheckDispose(int32_t handle) {
    if (handle < 0 || (size_t)handle >= objects.size()) { return 0; }
    auto & o = objects[handle];
    if (!o.isUsed()) { return 0; }
    if (o.refCount <= 0) { return 0; } // already disposed / disposing

    o.refCount--;
    auto newRefCount = o.refCount;
    auto canBeDisposed = (o.addr != disableDisposeForObject);
    if (canBeDisposed) {
        CheckDispose(handle);
    }
    // object could be removed at this point, don't use any values.
    ManagedObjectLog("Line %d SubRef: handle=%d new refcount=%d canBeDisposed=%d", currentline, handle, newRefCount, canBeDisposed);
    return newRefCount;
}

int32_t ManagedObjectPool::SubRefNoCheck(int32_t handle)
{
    if (handle < 0 || (size_t)handle >= objects.size()) { return 0; }
    auto & o = objects[handle];
    if (!o.isUsed()) { return 0; }
    if (o.refCount <= 0) { return 0; } // already disposed / disposing
    o.refCount--;
    ManagedObjectLog("Line %d SubRefNoCheck: handle=%d new refcount=%d", currentline, handle, o.refCount);
    return o.refCount;
}

int32_t ManagedObjectPool::AddressToHandle(const char *addr) {
    if (addr == nullptr) { return 0; }
    auto it = handleByAddress.find(addr);
    if (it == handleByAddress.end()) { return 0; }
    return it->second;
}

// this function is called often (whenever a pointer is used)
const char* ManagedObjectPool::HandleToAddress(int32_t handle) {
    if (handle < 0 || (size_t)handle >= objects.size()) { return nullptr; }
    auto & o = objects[handle];
    if (!o.isUsed()) { return nullptr; }
    return o.addr;
}

// this function is called often (whenever a pointer is used)
ScriptValueType ManagedObjectPool::HandleToAddressAndManager(int32_t handle, void *&object, ICCDynamicObject *&manager) {
    if ((handle < 0 || (size_t)handle >= objects.size()) || !objects[handle].isUsed())
    {
        object = nullptr;
        manager = nullptr;
        return kScValUndefined;
    }
    auto &o = objects[handle];
    object = (void *)o.addr;  // WARNING: This strips the const from the char* pointer.
    manager = o.callback;
    return o.obj_type;
}

int ManagedObjectPool::RemoveObject(const char *address) {
    if (address == nullptr) { return 0; }
    auto it = handleByAddress.find(address);
    if (it == handleByAddress.end()) { return 0; }

    auto & o = objects[it->second];
    if ((o.gcRefCount & ManagedObject::GC_FLAG_EXCLUDED) == 0)
        gcUsedList.erase(o.gcItUsed);
    return Remove(o, true);
}

void ManagedObjectPool::RunGarbageCollectionIfAppropriate()
{
    if (objectCreationCounter <= GARBAGE_COLLECTION_INTERVAL) { return; }
    RunGarbageCollection();
    objectCreationCounter = 0;
}

void ManagedObjectPool::RunGarbageCollection()
{
    //
    // Proper GC resolving detached objects and circular dependencies
    //
    // Step 0: remove objects that have 0 refs already
    // Step 1: copy current ref counts
    for (auto it = gcUsedList.begin(); it != gcUsedList.end(); )
    {
        assert(it->handle != 0);
        auto test_it = it++;
        auto &obj = objects[test_it->handle];
        assert((obj.refCount & ManagedObject::GC_FLAG_EXCLUDED) == 0);
        if (obj.refCount == 0)
        {
            gcUsedList.erase(test_it);
            Remove(obj);
        }
        else
        {
            obj.gcRefCount = obj.refCount;
        }
    }
    // Step 2: remove all internal references: that is references
    // which are stored in objects themselves
    for (auto &gco : gcUsedList)
    {
        auto &objs = objects;
        auto &obj = objects[gco.handle];
        assert((gco.handle != 0) && (obj.refCount > 0));
        objects[gco.handle].callback->TraverseRefs(obj.addr,
            [&objs](int handle)
        {
            objs[handle].gcRefCount--;
        });
    }
    // Step 3: move all gc objects with 0 remaining counter to the removal list
    for (auto it = gcUsedList.begin(); it != gcUsedList.end(); )
    {
        if (objects[it->handle].gcRefCount == 0)
        {
            auto move_it = it++;
            gcRemList.splice(gcRemList.end(), gcUsedList, move_it);
        }
        else
        {
            ++it;
        }
    }
    // Step 4: add internal references for all objects remaining
    // in the normal list
    for (auto &gco : gcUsedList)
    {
        auto &objs = objects;
        objects[gco.handle].callback->TraverseRefs(objects[gco.handle].addr,
            [&objs](int handle)
        {
            objs[handle].gcRefCount++;
        });
    }
    // Step 5: move all gc objects with > 0 counter back from the removal list
    // FIXME: is there way to optimize this?
    int times_moved = 0;
    do
    {
        auto &objs = objects;
        times_moved = 0;
        for (auto it = gcRemList.begin(); it != gcRemList.end(); )
        {
            auto test_it = it++;
            if (objects[test_it->handle].gcRefCount > 0)
            {
                gcUsedList.splice(gcUsedList.end(), gcRemList, test_it);
                objects[test_it->handle].gcItUsed = test_it; // ugh... scary
                objects[test_it->handle].callback->TraverseRefs(objects[test_it->handle].addr,
                    [&objs](int handle)
                {
                    objs[handle].gcRefCount++;
                });
                times_moved = 1;
            }
        }
    } while (times_moved > 0);
    // Step 6: dispose those remaining in the removal list
    for (auto it = gcRemList.begin(); it != gcRemList.end(); )
    {
        auto rem_it = it++;
        Remove(objects[rem_it->handle]);
        gcRemList.erase(rem_it);
    }
    assert(gcRemList.empty());
    ManagedObjectLog("Ran garbage collection");
}

int ManagedObjectPool::Add(int handle, const char *address, ICCDynamicObject *callback,
    bool plugin_object, bool persistent)
{
    auto & o = objects[handle];
    if (o.isUsed()) { cc_error("used: %d", handle); return 0; }

    o = ManagedObject(plugin_object ? kScValPluginObject : kScValDynamicObject, handle, address, callback);

    handleByAddress.insert({address, o.handle});
    if (persistent) { // mark persistent object as excluded from GC
        o.gcRefCount = ManagedObject::GC_FLAG_EXCLUDED;
    } else { // if regular - then add to the GC scan
        o.gcItUsed = gcUsedList.insert(gcUsedList.end(), GCObject(o.handle));
        objectCreationCounter++;
    }
    ManagedObjectLog("Allocated managed object type=%s, handle=%d, addr=%08X", callback->GetType(), handle, address);
    return handle;
}

int ManagedObjectPool::AddObject(const char *address, ICCDynamicObject *callback,
    bool plugin_object, bool persistent) 
{
    int32_t handle;

    if (!available_ids.empty()) {
        handle = available_ids.front();
        available_ids.pop();
    } else {
        handle = nextHandle++;
        if ((size_t)handle >= objects.size()) {
           objects.resize(handle + 1024, ManagedObject());
        }
    }

    return Add(handle, address, callback, plugin_object, persistent);
}


int ManagedObjectPool::AddUnserializedObject(const char *address, ICCDynamicObject *callback, int handle,
    bool plugin_object, bool persistent) 
{
    if (handle < 0) { cc_error("Attempt to assign invalid handle: %d", handle); return 0; }
    if ((size_t)handle >= objects.size()) {
        objects.resize(handle + 1024, ManagedObject());
    }

    return Add(handle, address, callback, plugin_object, persistent);
}

void ManagedObjectPool::WriteToDisk(Stream *out) {

    // use this opportunity to clean up any non-referenced pointers
    RunGarbageCollection();

    std::vector<char> serializeBuffer;
    serializeBuffer.resize(SERIALIZE_BUFFER_SIZE);

    out->WriteInt32(OBJECT_CACHE_MAGIC_NUMBER);
    out->WriteInt32(OBJECT_CACHE_SAVE_VERSION);

    int size = 0;
    for (int i = 1; i < nextHandle; i++) {
        auto const & o = objects[i];
        if (o.isUsed()) { 
            size += 1;
        }
    }
    out->WriteInt32(size);

    for (int i = 1; i < nextHandle; i++) {
        auto const & o = objects[i];
        if (!o.isUsed()) { continue; }

        // handle
        out->WriteInt32(o.handle);
        // write the type of the object
        StrUtil::WriteCStr((char*)o.callback->GetType(), out);
        // now write the object data
        int bytesWritten = o.callback->Serialize(o.addr, &serializeBuffer.front(), serializeBuffer.size());
        if ((bytesWritten < 0) && ((size_t)(-bytesWritten) > serializeBuffer.size()))
        {
            // buffer not big enough, re-allocate with requested size
            serializeBuffer.resize(-bytesWritten);
            bytesWritten = o.callback->Serialize(o.addr, &serializeBuffer.front(), serializeBuffer.size());
        }
        assert(bytesWritten >= 0);
        out->WriteInt32(bytesWritten);
        out->Write(&serializeBuffer.front(), bytesWritten);
        out->WriteInt32(o.refCount);

        ManagedObjectLog("Wrote handle = %d", o.handle);
    }
}

int ManagedObjectPool::ReadFromDisk(Stream *in, ICCObjectReader *reader) {
    if (in->ReadInt32() != OBJECT_CACHE_MAGIC_NUMBER) {
        cc_error("Invalid data format");
        return -1;
    }

    auto version = in->ReadInt32();
    if (version < OBJECT_CACHE_SAVE_VERSION || version > OBJECT_CACHE_SAVE_VERSION) {
        cc_error("Data version %d is not supported", version);
        return -1;
    }

    char typeNameBuffer[200];
    std::vector<char> serializeBuffer;
    serializeBuffer.resize(SERIALIZE_BUFFER_SIZE);

    int objectsSize = in->ReadInt32();
    for (int i = 0; i < objectsSize; i++) {
        auto handle = in->ReadInt32();
        assert (handle >= 1);
        StrUtil::ReadCStr(typeNameBuffer, in, sizeof(typeNameBuffer));
        assert (typeNameBuffer[0] != 0);
        size_t numBytes = in->ReadInt32();
        if (numBytes > serializeBuffer.size()) {
            serializeBuffer.resize(numBytes);
        }
        in->Read(&serializeBuffer.front(), numBytes);
        // Delegate work to ICCObjectReader
        reader->Unserialize(handle, typeNameBuffer, &serializeBuffer.front(), numBytes);
        objects[handle].refCount = in->ReadInt32();
        ManagedObjectLog("Read handle = %d", objects[i].handle);
    }

    // re-adjust next handles. (in case saved in random order)
    while (!available_ids.empty()) { available_ids.pop(); }
    nextHandle = 1;

    for (const auto &o : objects) {
        if (o.isUsed()) { 
            nextHandle = o.handle + 1;
        }
    }
    for (int i = 1; i < nextHandle; i++) {
        if (!objects[i].isUsed()) {
            available_ids.push(i);
        }
    }

    return 0;
}

// de-allocate all objects
void ManagedObjectPool::reset() {
    for (int i = 1; i < nextHandle; i++) {
        auto & o = objects[i];
        if (!o.isUsed()) { continue; }
        Remove(o, true);
    }
    while (!available_ids.empty()) { available_ids.pop(); }
    gcUsedList.clear();
    gcRemList.clear();
    nextHandle = 1;
}

ManagedObjectPool::ManagedObjectPool() : objectCreationCounter(0), nextHandle(1), available_ids(), objects(RESERVED_SIZE, ManagedObject()), handleByAddress() {
    handleByAddress.reserve(RESERVED_SIZE);
}

void ManagedObjectPool::RemapTypeids(const std::unordered_map<uint32_t, uint32_t> &typeid_map)
{
    for (const auto &o : objects)
    {
        if (!o.isUsed()) continue;
        o.callback->RemapTypeids(o.addr, typeid_map);
    }
}


ManagedObjectPool pool;
