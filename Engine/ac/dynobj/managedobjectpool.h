//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================

#ifndef __CC_MANAGEDOBJECTPOOL_H
#define __CC_MANAGEDOBJECTPOOL_H

#include <vector>
#include <queue>
#include <unordered_map>

#include "core/platform.h"
#include "script/runtimescriptvalue.h"
#include "ac/dynobj/cc_scriptobject.h"   // IScriptObject

namespace AGS { namespace Common { class Stream; }}
using namespace AGS; // FIXME later

struct ManagedObjectPool final {
private:
    // TODO: find out if we can make handle size_t
    struct ManagedObject {
        ScriptValueType obj_type;
        int32_t handle;
        void *addr;
        IScriptObject *callback;
        int refCount;

        bool isUsed() const { return obj_type != kScValUndefined; }

        ManagedObject() 
            : obj_type(kScValUndefined), handle(0), addr(nullptr), callback(nullptr), refCount(0) {}
        ManagedObject(ScriptValueType obj_type, int32_t handle, void *addr, IScriptObject * callback) 
            : obj_type(obj_type), handle(handle), addr(addr), callback(callback), refCount(0) {}
    };

    int objectCreationCounter;  // used to do garbage collection every so often

    int32_t nextHandle {}; // TODO: manage nextHandle's going over INT32_MAX !
    std::queue<int32_t> available_ids;
    std::vector<ManagedObject> objects;
    std::unordered_map<void*, int32_t> handleByAddress;

    int  Add(int handle, void *address, IScriptObject *callback, ScriptValueType obj_type);
    int  Remove(ManagedObject &o, bool force = false);
    void RunGarbageCollection();

public:

    int32_t AddRef(int32_t handle);
    int CheckDispose(int32_t handle);
    int32_t SubRef(int32_t handle);
    int32_t AddressToHandle(void *addr);
    void* HandleToAddress(int32_t handle);
    ScriptValueType HandleToAddressAndManager(int32_t handle, void *&object, IScriptObject *&manager);
    int RemoveObject(void *address);
    void RunGarbageCollectionIfAppropriate();
    int AddObject(void *address, IScriptObject *callback, ScriptValueType obj_type);
    int AddUnserializedObject(void *address, IScriptObject *callback, ScriptValueType obj_type, int handle);
    void WriteToDisk(Common::Stream *out);
    int ReadFromDisk(Common::Stream *in, ICCObjectCollectionReader *reader);
    void reset();
    ManagedObjectPool();

    void *disableDisposeForObject {nullptr};
};

extern ManagedObjectPool pool;

// Extreme(!!) verbosity managed memory pool log
#if DEBUG_MANAGED_OBJECTS
#define ManagedObjectLog(...) Debug::Printf(kDbgGroup_ManObj, kDbgMsg_Debug, __VA_ARGS__)
#else
#define ManagedObjectLog(...)
#endif

#endif // __CC_MANAGEDOBJECTPOOL_H
