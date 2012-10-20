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

#include "ac/dynobj/cc_dynamicobject.h"   // ICCDynamicObject

namespace AGS { namespace Common { class DataStream; }}
using namespace AGS; // FIXME later

#define OBJECT_CACHE_MAGIC_NUMBER 0xa30b
#define SERIALIZE_BUFFER_SIZE 10240
const int ARRAY_INCREMENT_SIZE = 100;
const int GARBAGE_COLLECTION_INTERVAL = 100;

struct ManagedObjectPool {
    struct ManagedObject {
        long handle;
        const char *addr;
        ICCDynamicObject * callback;
        int  refCount;

        void init(long theHandle, const char *theAddress, ICCDynamicObject *theCallback);
        int remove(bool force);
        int AddRef();
        int CheckDispose();
        int SubRef();
        void SubRefNoDispose();
    };
private:

    ManagedObject *objects;
    int arrayAllocLimit;
    int numObjects;  // not actually numObjects, but the highest index used
    int objectCreationCounter;  // used to do garbage collection every so often

public:

    long AddRef(long handle);
    int CheckDispose(long handle);
    long SubRef(long handle);
    long AddressToHandle(const char *addr);
    const char* HandleToAddress(long handle);
    void HandleToAddressAndManager(long handle, void *&object, ICCDynamicObject *&manager);
    int RemoveObject(const char *address);
    void RunGarbageCollectionIfAppropriate();
    void RunGarbageCollection();
    int AddObject(const char *address, ICCDynamicObject *callback, int useSlot = -1);
    void WriteToDisk(Common::DataStream *out);
    int ReadFromDisk(Common::DataStream *in, ICCObjectReader *reader);
    void reset();
    ManagedObjectPool();

    const char* disableDisposeForObject;
};

extern ManagedObjectPool pool;

#endif // __CC_MANAGEDOBJECTPOOL_H