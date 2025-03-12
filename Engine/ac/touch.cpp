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
#include "ac/touch.h"
#include <vector>
#include "ac/common.h"
#include "ac/gamestate.h"
#include "ac/dynobj/dynobj_manager.h"
#include "ac/dynobj/scripttouchpointer.h"
#include "debug/debug_log.h"
#include "device/mousew32.h"
#include "main/graphics_mode.h"

// A state of an arbitrary pointer performing a touch
struct TouchPointer
{
    static const int NoIndex = -1;
    int PointerID = NoIndex;
    TouchPhase Phase = TouchPhase::Undefined;
    Point Position = Point(INT32_MIN, INT32_MIN); // default to offscreen
    int ScriptObjHandle = 0;

    TouchPointer() = default;
    TouchPointer(int pointer_id, TouchPhase phase, const Point &pos, int script_handle = 0)
        : PointerID(pointer_id), Phase(phase), Position(pos), ScriptObjHandle(script_handle)
    {}

    void SetUndefinedState()
    {
        Phase = TouchPhase::Undefined;
        Position = Point(-1, -1);
    }
};

std::vector<TouchPointer> g_TouchPointers;

static void DetachScriptTouchPointer(TouchPointer &tp)
{
    if (tp.ScriptObjHandle > 0)
    {
        ScriptTouchPointer *stp = static_cast<ScriptTouchPointer *>(ccGetObjectAddressFromHandle(tp.ScriptObjHandle));
        if (stp)
        {
            stp->Invalidate();
            ccReleaseObjectReference(tp.ScriptObjHandle);
        }
        tp.ScriptObjHandle = 0;
    }
}

static void OnTouchPointer(int pointer_id, TouchPhase phase, const Point &position)
{
    assert(pointer_id >= 0);
    if (pointer_id < 0)
        return;

    g_TouchPointers.resize(std::max(static_cast<size_t>(pointer_id) + 1, g_TouchPointers.size()));
    // NOTE: if we decide to detach script object when the pointer gets
    // to a particular state (e.g. "up", or "inactive"), then here is the
    // place to do so. See DetachScriptTouchPointer().
    const int old_handle = g_TouchPointers[pointer_id].ScriptObjHandle;
    g_TouchPointers[pointer_id] = TouchPointer(
        pointer_id, phase, position, old_handle
    );
}

void OnTouchPointerDown(int pointer_id, const Point &position)
{
    OnTouchPointer(pointer_id, TouchPhase::Down, Mouse::SysToGamePos(position));
}

void OnTouchPointerMotion(int pointer_id, const Point &position)
{
    OnTouchPointer(pointer_id, TouchPhase::Motion, Mouse::SysToGamePos(position));
}

void OnTouchPointerUp(int pointer_id, const Point &position)
{
    OnTouchPointer(pointer_id, TouchPhase::Up, Mouse::SysToGamePos(position));
}

void ResetAllTouchPointers()
{
    for (auto &tp : g_TouchPointers)
    {
        tp.SetUndefinedState();
    }
}

void RemoveAllTouchPointers()
{
    for (auto &tp : g_TouchPointers)
    {
        DetachScriptTouchPointer(tp);
    }
    g_TouchPointers.clear();
}

int Touch_GetTouchPointerCount()
{
    return static_cast<int>(g_TouchPointers.size());
}

ScriptTouchPointer *Touch_GetTouchPointers(int pointer_id)
{
    if (pointer_id < 0 || static_cast<size_t>(pointer_id) >= g_TouchPointers.size())
        return nullptr;

    if (g_TouchPointers[pointer_id].ScriptObjHandle > 0)
    {
        return static_cast<ScriptTouchPointer*>(ccGetObjectAddressFromHandle(g_TouchPointers[pointer_id].ScriptObjHandle));
    }

    auto *tpointer = new ScriptTouchPointer(pointer_id);
    const int handle = ccRegisterManagedObject(tpointer, tpointer);
    ccAddObjectReference(handle); // add internal ref
    g_TouchPointers[pointer_id].ScriptObjHandle = handle;
    return tpointer;
}

bool ValidateTouchPointer(ScriptTouchPointer *tp, const char *apiname)
{
    if (tp->GetPointerID() < 0 || static_cast<size_t>(tp->GetPointerID()) >= g_TouchPointers.size())
    {
        debug_script_warn("%s: TouchPointer is not valid", apiname);
        return false;
    }
    return true;
}

int TouchPointer_GetID(ScriptTouchPointer *tp)
{
    if (!ValidateTouchPointer(tp, "TouchPointer.ID"))
        return -1;
    return tp->GetPointerID();
}

bool TouchPointer_GetIsDown(ScriptTouchPointer *tp)
{
    if (!ValidateTouchPointer(tp, "TouchPointer.IsDown"))
        return false;
    return g_TouchPointers[tp->GetPointerID()].Phase != TouchPhase::Up;
}

int TouchPointer_GetX(ScriptTouchPointer *tp)
{
    if (!ValidateTouchPointer(tp, "TouchPointer.X"))
        return 0;
    return g_TouchPointers[tp->GetPointerID()].Position.X;
}

int TouchPointer_GetY(ScriptTouchPointer *tp)
{
    if (!ValidateTouchPointer(tp, "TouchPointer.Y"))
        return 0;
    return g_TouchPointers[tp->GetPointerID()].Position.Y;
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "script/script_api.h"
#include "script/script_runtime.h"

RuntimeScriptValue Sc_Touch_GetTouchPointerCount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Touch_GetTouchPointerCount);
}

RuntimeScriptValue Sc_Touch_GetTouchPointers(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT(ScriptTouchPointer, Touch_GetTouchPointers);
}

RuntimeScriptValue Sc_TouchPointer_GetID(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptTouchPointer, TouchPointer_GetID);
}

RuntimeScriptValue Sc_TouchPointer_GetIsDown(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptTouchPointer, TouchPointer_GetIsDown);
}

RuntimeScriptValue Sc_TouchPointer_GetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptTouchPointer, TouchPointer_GetX);
}

RuntimeScriptValue Sc_TouchPointer_GetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptTouchPointer, TouchPointer_GetY);
}

void RegisterTouchAPI()
{
    ScFnRegister touch_api[] = {
            { "Touch::get_TouchPointerCount",   API_FN_PAIR(Touch_GetTouchPointerCount) },
            { "Touch::geti_TouchPointers",      API_FN_PAIR(Touch_GetTouchPointers) },

            { "TouchPointer::get_ID",           API_FN_PAIR(TouchPointer_GetID) },
            { "TouchPointer::get_IsDown",       API_FN_PAIR(TouchPointer_GetIsDown) },
            { "TouchPointer::get_X",            API_FN_PAIR(TouchPointer_GetX) },
            { "TouchPointer::get_Y",            API_FN_PAIR(TouchPointer_GetY) },
    };

    ccAddExternalFunctions(touch_api);
}
