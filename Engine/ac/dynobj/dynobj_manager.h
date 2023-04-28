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
#include "ac/dynobj/cc_dynamicobject.h"

// Forward declaration
namespace AGS { namespace Common { class Stream; } }
using namespace AGS; // FIXME later

// set the class that will be used for dynamic strings
extern void  ccSetStringClassImpl(ICCStringClass *theClass);
// register a memory handle for the object and allow script
// pointers to point to it
extern int32_t ccRegisterManagedObject(const void *object, ICCDynamicObject *, bool plugin_object = false);
// register a de-serialized object
extern int32_t ccRegisterUnserializedObject(int index, const void *object, ICCDynamicObject *, bool plugin_object = false);
// unregister a particular object
extern int   ccUnRegisterManagedObject(const void *object);
// remove all registered objects
extern void  ccUnregisterAllObjects();
// serialize all objects to disk
extern void  ccSerializeAllObjects(Common::Stream *out);
// un-serialise all objects (will remove all currently registered ones)
extern int   ccUnserializeAllObjects(Common::Stream *in, ICCObjectReader *callback);
// dispose the object if RefCount==0
extern void  ccAttemptDisposeObject(int32_t handle);
// translate between object handles and memory addresses
extern int32_t ccGetObjectHandleFromAddress(const void *address);
// TODO: not sure if it makes any sense whatsoever to use "const char*"
// in these functions, might as well change to char* or just void*.
extern const char *ccGetObjectAddressFromHandle(int32_t handle);
extern ScriptValueType ccGetObjectAddressAndManagerFromHandle(int32_t handle, void *&object, ICCDynamicObject *&manager);

extern int ccAddObjectReference(int32_t handle);
extern int ccReleaseObjectReference(int32_t handle);

extern ICCStringClass *stringClassImpl;

#endif // __AGS_EE_DYNOBJ__DYNOBJMANAGER_H
