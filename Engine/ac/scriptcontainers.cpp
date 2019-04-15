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
// Containers script API.
//
//=============================================================================
#include "ac/dynobj/cc_dynamicarray.h"
#include "ac/dynobj/cc_dynamicobject.h"
#include "ac/dynobj/scriptset.h"
#include "ac/dynobj/scriptstring.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

//=============================================================================
//
// Set of strings script API.
//
//=============================================================================

ScriptSetBase *Set_Create(bool sorted, bool case_sensitive)
{
    ScriptSetBase *set;
    if (sorted)
    {
        if (case_sensitive)
            set = new ScriptSet();
        else
            set = new ScriptSetCI();
    }
    else
    {
        if (case_sensitive)
            set = new ScriptHashSet();
        else
            set = new ScriptHashSetCI();
    }
    ccRegisterManagedObject(set, set);
    return set;
}

bool Set_Add(ScriptSetBase *set, const char *item)
{
    return set->Add(item);
}

void Set_Clear(ScriptSetBase *set)
{
    set->Clear();
}

bool Set_Contains(ScriptSetBase *set, const char *item)
{
    return set->Contains(item);
}

bool Set_Remove(ScriptSetBase *set, const char *item)
{
    return set->Remove(item);
}

bool Set_GetCaseSensitive(ScriptSetBase *set)
{
    return set->IsCaseSensitive();
}

bool Set_GetSorted(ScriptSetBase *set)
{
    return set->IsSorted();
}

int Set_GetItemCount(ScriptSetBase *set)
{
    return set->GetItemCount();
}

void *Set_GetItemsAsArray(ScriptSetBase *set)
{
    std::vector<const char*> items;
    set->GetItems(items);
    // NOTE: we need element size of "handle" for array of managed pointers
    DynObjectRef arr = globalDynamicArray.Create(items.size(), sizeof(int32_t), true);
    if (!arr.second)
        return nullptr;
    // Create script strings and put handles into array
    int32_t *slots = static_cast<int32_t*>(arr.second);
    for (auto s : items)
    {
        DynObjectRef str = stringClassImpl->CreateString(s);
        *(slots++) = str.first;
    }
    return arr.second;
}

RuntimeScriptValue Sc_Set_Create(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PBOOL2(ScriptSetBase, Set_Create);
}

RuntimeScriptValue Sc_Set_Add(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ(ScriptSetBase, Set_Add, const char);
}

RuntimeScriptValue Sc_Set_Clear(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptSetBase, Set_Clear);
}

RuntimeScriptValue Sc_Set_Contains(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ(ScriptSetBase, Set_Contains, const char);
}

RuntimeScriptValue Sc_Set_Remove(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ(ScriptSetBase, Set_Remove, const char);
}

RuntimeScriptValue Sc_Set_GetCaseSensitive(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptSetBase, Set_GetCaseSensitive);
}

RuntimeScriptValue Sc_Set_GetSorted(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptSetBase, Set_GetSorted);
}

RuntimeScriptValue Sc_Set_GetItemCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptSetBase, Set_GetItemCount);
}

RuntimeScriptValue Sc_Set_GetItemAsArray(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptSetBase, void, globalDynamicArray, Set_GetItemsAsArray);
}



void RegisterContainerAPI()
{
    ccAddExternalStaticFunction("Set::Create", Sc_Set_Create);
    ccAddExternalObjectFunction("Set::Add", Sc_Set_Add);
    ccAddExternalObjectFunction("Set::Clear", Sc_Set_Clear);
    ccAddExternalObjectFunction("Set::Contains", Sc_Set_Contains);
    ccAddExternalObjectFunction("Set::Remove", Sc_Set_Remove);
    ccAddExternalObjectFunction("Set::get_CaseSensitive", Sc_Set_GetCaseSensitive);
    ccAddExternalObjectFunction("Set::get_Sorted", Sc_Set_GetSorted);
    ccAddExternalObjectFunction("Set::get_ItemCount", Sc_Set_GetItemCount);
    ccAddExternalObjectFunction("Set::GetItemsAsArray", Sc_Set_GetItemAsArray);
}
