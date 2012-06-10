#ifndef __CC_MANAGEDOBJECTPOOL_H
#define __CC_MANAGEDOBJECTPOOL_H

#include "cc_dynamicarray.h"    // ICCDynamicObject, constants
#include "cs_internal.h"        // fputstring, etc

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

        void init(long theHandle, const char *theAddress, ICCDynamicObject *theCallback) {
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

        int remove(bool force) {

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

        int AddRef() {

#ifdef DEBUG_MANAGED_OBJECTS
            char bufff[200];
            sprintf(bufff,"Line %d AddRef: handle=%d new refcount=%d", currentline, handle, refCount+1);
            write_log(bufff);
#endif

            return ++refCount;
        }

        int CheckDispose() {
            if ((refCount < 1) && (callback != NULL)) {
                if (remove(false))
                    return 1;
            }
            return 0;
        }

        int SubRef() {
            refCount--;

#ifdef DEBUG_MANAGED_OBJECTS
            char bufff[200];
            sprintf(bufff,"Line %d SubRef: handle=%d new refcount=%d", currentline, handle, refCount);
            write_log(bufff);
#endif

            return CheckDispose();
        }

        void SubRefNoDispose() {
            refCount--;

#ifdef DEBUG_MANAGED_OBJECTS
            char bufff[200];
            sprintf(bufff,"Line %d SubRefNoDispose: handle=%d new refcount=%d", currentline, handle, refCount);
            write_log(bufff);
#endif
        }
    };
private:

    ManagedObject *objects;
    int arrayAllocLimit;
    int numObjects;  // not actually numObjects, but the highest index used
    int objectCreationCounter;  // used to do garbage collection every so often

public:

    long AddRef(long handle) {
        return objects[handle].AddRef();
    }

    int CheckDispose(long handle) {
        return objects[handle].CheckDispose();
    }

    long SubRef(long handle) {
        if ((disableDisposeForObject != NULL) && 
            (objects[handle].addr == disableDisposeForObject))
            objects[handle].SubRefNoDispose();
        else
            objects[handle].SubRef();
        return objects[handle].refCount;
    }

    long AddressToHandle(const char *addr) {
        // this function is only called when a pointer is set
        // SLOW LOOP ALERT, improve at some point
        for (int kk = 1; kk < arrayAllocLimit; kk++) {
            if (objects[kk].addr == addr)
                return objects[kk].handle;
        }
        return 0;
    }

    const char* HandleToAddress(long handle) {
        // this function is called often (whenever a pointer is used)
        if ((handle < 1) || (handle >= arrayAllocLimit))
            return NULL;
        if (objects[handle].handle == 0)
            return NULL;
        return objects[handle].addr;
    }

    int RemoveObject(const char *address) {
        long handl = AddressToHandle(address);
        if (handl == 0)
            return 0;

        objects[handl].remove(true);
        return 1;
    }

    void RunGarbageCollectionIfAppropriate()
    {
        if (objectCreationCounter > GARBAGE_COLLECTION_INTERVAL)
        {
            objectCreationCounter = 0;
            RunGarbageCollection();
        }
    }

    void RunGarbageCollection()
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

    int AddObject(const char *address, ICCDynamicObject *callback, int useSlot = -1) {
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

    void WriteToDisk(FILE *output) {
        int serializeBufferSize = SERIALIZE_BUFFER_SIZE;
        char *serializeBuffer = (char*)malloc(serializeBufferSize);

        putw(OBJECT_CACHE_MAGIC_NUMBER, output);
        putw(1, output);  // version
        putw(numObjects, output);

        // use this opportunity to clean up any non-referenced pointers
        RunGarbageCollection();

        for (int i = 1; i < numObjects; i++) 
        {
            if ((objects[i].handle) && (objects[i].callback != NULL)) {
                // write the type of the object
                fputstring((char*)objects[i].callback->GetType(), output);
                // now write the object data
                int bytesWritten = objects[i].callback->Serialize(objects[i].addr, serializeBuffer, serializeBufferSize);
                if ((bytesWritten < 0) && ((-bytesWritten) > serializeBufferSize))
                {
                    // buffer not big enough, re-allocate with requested size
                    serializeBufferSize = -bytesWritten;
                    serializeBuffer = (char*)realloc(serializeBuffer, serializeBufferSize);
                    bytesWritten = objects[i].callback->Serialize(objects[i].addr, serializeBuffer, serializeBufferSize);
                }
                putw(bytesWritten, output);
                if (bytesWritten > 0)
                    fwrite(serializeBuffer, bytesWritten, 1, output);
                putw(objects[i].refCount, output);
            }
            else  // write empty string if we cannot serialize it
                fputc(0, output); 
        }

        free(serializeBuffer);
    }

    int ReadFromDisk(FILE *input, ICCObjectReader *reader) {
        int serializeBufferSize = SERIALIZE_BUFFER_SIZE;
        char *serializeBuffer = (char*)malloc(serializeBufferSize);
        char typeNameBuffer[200];

        if (getw(input) != OBJECT_CACHE_MAGIC_NUMBER) {
            cc_error("Data was not written by ccSeralize");
            return -1;
        }

        if (getw(input) != 1) {
            cc_error("Invalid data version");
            return -1;
        }

        int numObjs = getw(input);

        if (numObjs >= arrayAllocLimit) {
            arrayAllocLimit = numObjs + ARRAY_INCREMENT_SIZE;
            free(objects);
            objects = (ManagedObject*)calloc(sizeof(ManagedObject), arrayAllocLimit);
        }
        numObjects = numObjs;

        for (int i = 1; i < numObjs; i++) {
            fgetstring_limit(typeNameBuffer, input, 199);
            if (typeNameBuffer[0] != 0) {
                int numBytes = getw(input);
                if (numBytes > serializeBufferSize) {
                    serializeBufferSize = numBytes;
                    serializeBuffer = (char*)realloc(serializeBuffer, serializeBufferSize);
                }
                if (numBytes > 0)
                    fread(serializeBuffer, numBytes, 1, input);

                if (strcmp(typeNameBuffer, CC_DYNAMIC_ARRAY_TYPE_NAME) == 0)
                {
                    globalDynamicArray.Unserialize(i, serializeBuffer, numBytes);
                }
                else
                {
                    reader->Unserialize(i, typeNameBuffer, serializeBuffer, numBytes);
                }
                objects[i].refCount = getw(input);
            }
        }

        free(serializeBuffer);
        return 0;
    }

    void reset() {
        // de-allocate all objects
        for (int kk = 1; kk < arrayAllocLimit; kk++) {
            if (objects[kk].handle)
                objects[kk].remove(true);
        }
        memset(&objects[0], 0, sizeof(ManagedObject) * arrayAllocLimit);
        numObjects = 1;
    }

    ManagedObjectPool() {
        numObjects = 1;
        arrayAllocLimit = 10;
        objects = (ManagedObject*)calloc(sizeof(ManagedObject), arrayAllocLimit);
        disableDisposeForObject = NULL;
    }

    const char* disableDisposeForObject;
};

extern ManagedObjectPool pool;

#endif // __CC_MANAGEDOBJECTPOOL_H