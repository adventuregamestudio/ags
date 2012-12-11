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
// Script API function type and helper macros for forwarding runtime script
// values to real engine functions.
//
//=============================================================================
#ifndef __AGS_EE_SCRIPT__SCRIPTAPI_H
#define __AGS_EE_SCRIPT__SCRIPTAPI_H

#include "core/types.h"

struct RuntimeScriptValue;

// TODO: replace void* with base object class when possible; also put array class for parameters
typedef RuntimeScriptValue ScriptAPIFunction(void *self, RuntimeScriptValue *params, int32_t param_count);

// Helper macros for script functions
#define ASSERT_SELF(METHOD) \
    if (!self) \
    { \
        AGS::Common::Out::FPrint("Object pointer is null in call to " #METHOD); \
        return RuntimeScriptValue(); \
    }

#define ASSERT_PARAM_COUNT(METHOD, X) \
    if (X > 0 && (!params || param_count < X)) \
    { \
        AGS::Common::Out::FPrint("Not enough parameters in call to "#METHOD": expected %d, got %d", X, param_count); \
        return RuntimeScriptValue(); \
    }

#define ASSERT_OBJ_PARAM_COUNT(METHOD, X) \
    ASSERT_SELF(METHOD) \
    ASSERT_PARAM_COUNT(METHOD, X)

//-----------------------------------------------------------------------------
// Calls to static functions

#define API_SCALL_OBJ_PINT2(RET_CLASS, RET_MGR, FUNCTION) \
    ASSERT_PARAM_COUNT(FUNCTION, 2) \
    return RuntimeScriptValue().SetDynamicObject((void*)(RET_CLASS*)FUNCTION(params[0].GetInt32(), params[1].GetInt32()), &RET_MGR);

//-----------------------------------------------------------------------------
// Calls to object functions

#define API_OBJCALL_VOID(CLASS, METHOD) \
    ASSERT_SELF(METHOD) \
    METHOD((CLASS*)self); \
    return RuntimeScriptValue();

#define API_OBJCALL_VOID_PINT(CLASS, METHOD) \
    ASSERT_OBJ_PARAM_COUNT(METHOD, 1); \
    METHOD((CLASS*)self, params[0].GetInt32()); \
    return RuntimeScriptValue();

#define API_OBJCALL_VOID_PINT2(CLASS, METHOD) \
    ASSERT_OBJ_PARAM_COUNT(METHOD, 2); \
    METHOD((CLASS*)self, params[0].GetInt32(), params[1].GetInt32()); \
    return RuntimeScriptValue();

#define API_OBJCALL_VOID_PINT3(CLASS, METHOD) \
    ASSERT_OBJ_PARAM_COUNT(METHOD, 3); \
    METHOD((CLASS*)self, params[0].GetInt32(), params[1].GetInt32(), params[2].GetInt32()); \
    return RuntimeScriptValue();

#define API_OBJCALL_VOID_PINT4(CLASS, METHOD) \
    ASSERT_OBJ_PARAM_COUNT(METHOD, 4); \
    METHOD((CLASS*)self, params[0].GetInt32(), params[1].GetInt32(), params[2].GetInt32(), params[3].GetInt32()); \
    return RuntimeScriptValue();

#define API_OBJCALL_VOID_PINT5(CLASS, METHOD) \
    ASSERT_OBJ_PARAM_COUNT(METHOD, 5); \
    METHOD((CLASS*)self, params[0].GetInt32(), params[1].GetInt32(), params[2].GetInt32(), params[3].GetInt32(), params[4].GetInt32()); \
    return RuntimeScriptValue();

#define API_OBJCALL_VOID_PINT3_POBJ(CLASS, METHOD, P1CLASS) \
    ASSERT_OBJ_PARAM_COUNT(METHOD, 4); \
    METHOD((CLASS*)self, params[0].GetInt32(), params[1].GetInt32(), params[2].GetInt32(), (P1CLASS*)params[3].GetPtr()); \
    return RuntimeScriptValue();

#define API_OBJCALL_VOID_POBJ(CLASS, METHOD, P1CLASS) \
    ASSERT_OBJ_PARAM_COUNT(METHOD, 1); \
    METHOD((CLASS*)self, (P1CLASS*)params[0].GetPtr()); \
    return RuntimeScriptValue();

#define API_OBJCALL_VOID_POBJ_PINT(CLASS, METHOD, P1CLASS) \
    ASSERT_OBJ_PARAM_COUNT(METHOD, 2); \
    METHOD((CLASS*)self, (P1CLASS*)params[0].GetPtr(), params[1].GetInt32()); \
    return RuntimeScriptValue();

#define API_OBJCALL_VOID_POBJ_PINT2(CLASS, METHOD, P1CLASS) \
    ASSERT_OBJ_PARAM_COUNT(METHOD, 3); \
    METHOD((CLASS*)self, (P1CLASS*)params[0].GetPtr(), params[1].GetInt32(), params[2].GetInt32()); \
    return RuntimeScriptValue();

#define API_OBJCALL_VOID_POBJ2(CLASS, METHOD, P1CLASS, P2CLASS) \
    ASSERT_OBJ_PARAM_COUNT(METHOD, 2); \
    METHOD((CLASS*)self, (P1CLASS*)params[0].GetPtr(), (P2CLASS*)params[1].GetPtr()); \
    return RuntimeScriptValue();

#define API_OBJCALL_INT(CLASS, METHOD) \
    ASSERT_SELF(METHOD) \
    return RuntimeScriptValue().SetInt32(METHOD((CLASS*)self));

#define API_OBJCALL_INT_PINT(CLASS, METHOD) \
    ASSERT_OBJ_PARAM_COUNT(METHOD, 1) \
    return RuntimeScriptValue().SetInt32(METHOD((CLASS*)self, params[0].GetInt32()));

#define API_OBJCALL_INT_PINT2(CLASS, METHOD) \
    ASSERT_OBJ_PARAM_COUNT(METHOD, 2) \
    return RuntimeScriptValue().SetInt32(METHOD((CLASS*)self, params[0].GetInt32(), params[1].GetInt32()));

#define API_OBJCALL_INT_POBJ(CLASS, METHOD, P1CLASS) \
    ASSERT_OBJ_PARAM_COUNT(METHOD, 1) \
    return RuntimeScriptValue().SetInt32(METHOD((CLASS*)self, (P1CLASS*)params[0].GetPtr()));

#define API_OBJCALL_OBJ(CLASS, RET_CLASS, RET_MGR, METHOD) \
    ASSERT_SELF(METHOD) \
    return RuntimeScriptValue().SetDynamicObject((void*)(RET_CLASS*)METHOD((CLASS*)self), &RET_MGR);

#define API_OBJCALL_OBJ_PINT(CLASS, RET_CLASS, RET_MGR, METHOD) \
    ASSERT_OBJ_PARAM_COUNT(METHOD, 1) \
    return RuntimeScriptValue().SetDynamicObject((void*)(RET_CLASS*)METHOD((CLASS*)self, params[0].GetInt32()), &RET_MGR);

#define API_OBJCALL_OBJ_PINT2(CLASS, RET_CLASS, RET_MGR, METHOD) \
    ASSERT_OBJ_PARAM_COUNT(METHOD, 2) \
    return RuntimeScriptValue().SetDynamicObject((void*)(RET_CLASS*)METHOD((CLASS*)self, params[0].GetInt32(), params[1].GetInt32()), &RET_MGR);

#define API_OBJCALL_OBJ_PINT3(CLASS, RET_CLASS, RET_MGR, METHOD) \
    ASSERT_OBJ_PARAM_COUNT(METHOD, 3) \
    return RuntimeScriptValue().SetDynamicObject((void*)(RET_CLASS*)METHOD((CLASS*)self, params[0].GetInt32(), params[1].GetInt32(), params[2].GetInt32()), &RET_MGR);

#define API_OBJCALL_OBJ_POBJ(CLASS, RET_CLASS, RET_MGR, METHOD, P1CLASS) \
    ASSERT_OBJ_PARAM_COUNT(METHOD, 1) \
    return RuntimeScriptValue().SetDynamicObject((void*)(RET_CLASS*)METHOD((CLASS*)self, (P1CLASS*)params[0].GetPtr()), &RET_MGR);

#define API_OBJCALL_OBJAUTO_POBJ(CLASS, RET_CLASS, METHOD, P1CLASS) \
    ASSERT_OBJ_PARAM_COUNT(METHOD, 1) \
    RET_CLASS* ret_obj = METHOD((CLASS*)self, (P1CLASS*)params[0].GetPtr()); \
    return RuntimeScriptValue().SetDynamicObject(ret_obj, ret_obj);

#endif // __AGS_EE_SCRIPT__SCRIPTAPI_H
