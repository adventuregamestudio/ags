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
//
// Dynamic object management utilities.
// TODO: frankly, many of these functions could be factored out by a direct
// use of ManagedPool class.
//
//=============================================================================
#ifndef __AGS_EE_DYNOBJ__DYNOBJMANAGER_H
#define __AGS_EE_DYNOBJ__DYNOBJMANAGER_H

#include "core/types.h"
#include "script/runtimescriptvalue.h"
#include "ac/dynobj/cc_scriptobject.h"
#include "util/string.h"

// Forward declaration
namespace AGS { namespace Common { class Stream; } }
using namespace AGS; // FIXME later

// register a memory handle for the object and allow script pointers to point to it
int32_t ccRegisterManagedObject(void *object, IScriptObject *, ScriptValueType obj_type = kScValScriptObject);
// register a new object and add a reference count
int32_t ccRegisterManagedObjectAndRef(void *object, IScriptObject *callback, ScriptValueType obj_type = kScValScriptObject);
// register a new object and mark it persistent (always referenced by the engine)
int32_t ccRegisterPersistentObject(void *object, IScriptObject *callback, ScriptValueType obj_type = kScValScriptObject);
// register a de-serialized object
int32_t ccRegisterUnserializedObject(int handle, void *object, IScriptObject *, ScriptValueType obj_type = kScValScriptObject);
// register a de-serialized object and mark it persistent (always referenced by the engine)
int32_t ccRegisterUnserializedPersistentObject(int handle, void *object, IScriptObject *callback, ScriptValueType obj_type = kScValScriptObject);
// unregister a particular object
int   ccUnRegisterManagedObject(void *object);
// remove all registered objects
void  ccUnregisterAllObjects();
// serialize all objects to disk
void  ccSerializeAllObjects(Common::Stream *out);
// un-serialise all objects (will remove all currently registered ones)
int   ccUnserializeAllObjects(Common::Stream *in, ICCObjectCollectionReader *callback);
// dispose the object if RefCount==0
void  ccAttemptDisposeObject(int32_t handle);
// translate between object handles and memory addresses
int32_t ccGetObjectHandleFromAddress(void *address);
void *ccGetObjectAddressFromHandle(int32_t handle);
IScriptObject *ccGetObjectManager(void *address);
ScriptValueType ccGetObjectAddressAndManagerFromHandle(int32_t handle, void *&object, IScriptObject *&manager);

int ccAddObjectReference(int32_t handle);
int ccReleaseObjectReference(int32_t handle);

typedef void (*PfnProcessManagedObject)(int handle, IScriptObject *obj);
// Iterates all managed objects identified by their Type ID, and runs a callback for each of them
void ccTraverseManagedObjects(const AGS::Common::String &type, PfnProcessManagedObject proc);

#endif // __AGS_EE_DYNOBJ__DYNOBJMANAGER_H
