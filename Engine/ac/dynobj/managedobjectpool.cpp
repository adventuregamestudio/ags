
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ac/dynobj/managedobjectpool.h"
#include "ac/dynobj/cc_dynamicarray.h" // globalDynamicArray, constants
#include "util/string_utils.h"               // fputstring, etc
#include "script/cc_error.h"
#include "util/datastream.h"

using AGS::Common::DataStream;

void ManagedObjectPool::ManagedObject::init(long theHandle, const char *theAddress, ICCDynamicObject *theCallback) {
    handle = theHandle;
    addr = theAddress;
    callback = theCallback;
    refCount = 0;

#ifdef DEBUG_MANAGED_OBJECTS
    char bufff[200];
    sprintf(bufff,"Allocated managed object handle=%d, type=%s", theHandle, theCallback->GetType());
    write_log(bufff);
#endif
}

int ManagedObjectPool::ManagedObject::remove(bool force) {

    if ((callback != NULL) && (callback->Dispose(addr, force) == 0) &&
        (force == false))
        return 0;

#ifdef DEBUG_MANAGED_OBJECTS
    char bufff[200];
    sprintf(bufff,"Line %d Disposing managed object handle=%d", currentline, handle);
    write_log(bufff);
#endif

    handle = 0;
    addr = 0;
    callback = NULL;
    return 1;
}

int ManagedObjectPool::ManagedObject::AddRef() {

#ifdef DEBUG_MANAGED_OBJECTS
    char bufff[200];
    sprintf(bufff,"Line %d AddRef: handle=%d new refcount=%d", currentline, handle, refCount+1);
    write_log(bufff);
#endif

    return ++refCount;
}

int ManagedObjectPool::ManagedObject::CheckDispose() {
    if ((refCount < 1) && (callback != NULL)) {
        if (remove(false))
            return 1;
    }
    return 0;
}

int ManagedObjectPool::ManagedObject::SubRef() {
    refCount--;

#ifdef DEBUG_MANAGED_OBJECTS
    char bufff[200];
    sprintf(bufff,"Line %d SubRef: handle=%d new refcount=%d", currentline, handle, refCount);
    write_log(bufff);
#endif

    return CheckDispose();
}

void ManagedObjectPool::ManagedObject::SubRefNoDispose() {
    refCount--;

#ifdef DEBUG_MANAGED_OBJECTS
    char bufff[200];
    sprintf(bufff,"Line %d SubRefNoDispose: handle=%d new refcount=%d", currentline, handle, refCount);
    write_log(bufff);
#endif
}

long ManagedObjectPool::AddRef(long handle) {
        return objects[handle].AddRef();
}

int ManagedObjectPool::CheckDispose(long handle) {
    return objects[handle].CheckDispose();
}

long ManagedObjectPool::SubRef(long handle) {
    if ((disableDisposeForObject != NULL) && 
        (objects[handle].addr == disableDisposeForObject))
        objects[handle].SubRefNoDispose();
    else
        objects[handle].SubRef();
    return objects[handle].refCount;
}

long ManagedObjectPool::AddressToHandle(const char *addr) {
    // this function is only called when a pointer is set
    // SLOW LOOP ALERT, improve at some point
    for (int kk = 1; kk < arrayAllocLimit; kk++) {
        if (objects[kk].addr == addr)
            return objects[kk].handle;
    }
    return 0;
}

const char* ManagedObjectPool::HandleToAddress(long handle) {
    // this function is called often (whenever a pointer is used)
    if ((handle < 1) || (handle >= arrayAllocLimit))
        return NULL;
    if (objects[handle].handle == 0)
        return NULL;
    return objects[handle].addr;
}

int ManagedObjectPool::RemoveObject(const char *address) {
    long handl = AddressToHandle(address);
    if (handl == 0)
        return 0;

    objects[handl].remove(true);
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
    //write_log("Running garbage collection");

    for (int i = 1; i < numObjects; i++) 
    {
        if ((objects[i].refCount < 1) && (objects[i].callback != NULL)) 
        {
            objects[i].remove(false);
        }
    }
}

int ManagedObjectPool::AddObject(const char *address, ICCDynamicObject *callback, int useSlot) {
    if (useSlot == -1)
        useSlot = numObjects;

    objectCreationCounter++;

    if (useSlot < arrayAllocLimit) {
        // still space in the array, so use it
        objects[useSlot].init(useSlot, address, callback);
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
                    objects[i].init(i, address, callback);
                    return i;
                }
            }
        }
        // no empty slots, expand array
        while (useSlot >= arrayAllocLimit)
            arrayAllocLimit += ARRAY_INCREMENT_SIZE;

        objects = (ManagedObject*)realloc(objects, sizeof(ManagedObject) * arrayAllocLimit);
        memset(&objects[useSlot], 0, sizeof(ManagedObject) * ARRAY_INCREMENT_SIZE);
        objects[useSlot].init(useSlot, address, callback);
        if (useSlot == numObjects)
            numObjects++;
        return useSlot;
    }
}

void ManagedObjectPool::WriteToDisk(DataStream *out) {
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
            fputstring((char*)objects[i].callback->GetType(), out);
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

int ManagedObjectPool::ReadFromDisk(DataStream *in, ICCObjectReader *reader) {
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
        fgetstring_limit(typeNameBuffer, in, 199);
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
        if (objects[kk].handle)
            objects[kk].remove(true);
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
