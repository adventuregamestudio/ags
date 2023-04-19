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

#ifndef __CC_MANAGEDOBJECTPOOL_H
#define __CC_MANAGEDOBJECTPOOL_H

#include <list>
#include <vector>
#include <queue>
#include <unordered_map>

#include "core/platform.h"
#include "script/runtimescriptvalue.h"
#include "ac/dynobj/cc_dynamicobject.h"   // ICCDynamicObject

namespace AGS { namespace Common { class Stream; }}
using namespace AGS; // FIXME later

struct ManagedObjectPool final {
private:
    struct GCObject;
    // TODO: find out if we can make handle unsigned
    struct ManagedObject {
        ScriptValueType obj_type = kScValUndefined;
        int32_t handle = 0; // TODO: find out if may not store this here?
        // FIXME: this makes no sense having this as "const char*",
        // void* will be proper (and in all related functions)
        const char *addr = nullptr;
        ICCDynamicObject *callback = nullptr;
        int refCount = 0;
        // For GC
        // TODO: optimize the object storage...
        int gcRefCount = 0; // for scan & sweep algorithm

        bool isUsed() const { return obj_type != kScValUndefined; }

        ManagedObject() 
            : obj_type(kScValUndefined), handle(0), addr(nullptr), callback(nullptr), refCount(0) {}
        ManagedObject(ScriptValueType obj_type, int32_t handle, const char *addr, ICCDynamicObject * callback) 
            : obj_type(obj_type), handle(handle), addr(addr), callback(callback), refCount(0) {}
    };

    int objectCreationCounter;  // used to do garbage collection every so often

    int32_t nextHandle {}; // TODO: manage nextHandle's going over INT32_MAX !
    std::queue<int32_t> available_ids;
    std::vector<ManagedObject> objects;
    std::unordered_map<const char *, int32_t> handleByAddress;

    // Scan lists for the garbage collection;
    // TODO: this is bit inefficient, because of certain mem duplication, and extra
    // cross-references; ideally this should be merged with ManagedObject (and maybe even ICCDynamicObject)!
    struct GCObject
    {
        int32_t handle = 0;

        GCObject(int32_t h) : handle(h) {}
    };
    std::list<GCObject> gcUsedList;
    std::list<GCObject> gcRemList;

    int Remove(ManagedObject &o, bool force = false);
    void RunGarbageCollection();

public:

    int32_t AddRef(int32_t handle);
    int CheckDispose(int32_t handle);
    int32_t SubRef(int32_t handle);
    int32_t AddressToHandle(const char *addr);
    const char* HandleToAddress(int32_t handle);
    ScriptValueType HandleToAddressAndManager(int32_t handle, void *&object, ICCDynamicObject *&manager);
    int RemoveObject(const char *address);
    void RunGarbageCollectionIfAppropriate();
    int AddObject(const char *address, ICCDynamicObject *callback, bool plugin_object, bool persistent);
    int AddUnserializedObject(const char *address, ICCDynamicObject *callback, int handle,
        bool plugin_object, bool persistent);
    void WriteToDisk(Common::Stream *out);
    int ReadFromDisk(Common::Stream *in, ICCObjectReader *reader);
    void reset();
    ManagedObjectPool();

    // Remaps typeids for the managed objects that contain typeid fields;
    // uses provided typeid maps
    void RemapTypeids(const std::unordered_map<uint32_t, uint32_t> &typeid_map);

    const char* disableDisposeForObject {nullptr};
};

extern ManagedObjectPool pool;

// Extreme(!!) verbosity managed memory pool log
#if DEBUG_MANAGED_OBJECTS
#define ManagedObjectLog(...) Debug::Printf(kDbgGroup_ManObj, kDbgMsg_Debug, __VA_ARGS__)
#else
#define ManagedObjectLog(...)
#endif

#endif // __CC_MANAGEDOBJECTPOOL_H
