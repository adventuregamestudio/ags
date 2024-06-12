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

#include <array>
#include "ac/dynobj/scriptuserobject.h"
#include "ac/dynobj/cc_dynamicarray.h"
#include "device/mousew32.h"
#include "touch.h"
#include "ac/common.h"

enum class touch_state {
    up,
    motion,
    down
};

struct touch_point {
    int x;
    int y;
    bool down;
};

struct tp {
    static const int MAX_POINTERS = 10;
    int point_count = 0;
    std::array<touch_point, MAX_POINTERS> touch_points = {};
} _tp;

void on_touch_pointer(int pointer_id, Point position, touch_state state)
{
    _tp.point_count = std::max(pointer_id+1, _tp.point_count);
    _tp.touch_points[pointer_id].x = position.X;
    _tp.touch_points[pointer_id].y = position.Y;
    _tp.touch_points[pointer_id].down = state != touch_state::up;
}

void on_touch_pointer_down(int pointer_id, Point position)
{
    on_touch_pointer(pointer_id, position, touch_state::down);
}

void on_touch_pointer_motion(int pointer_id, Point position)
{
    on_touch_pointer(pointer_id, position, touch_state::motion);
}

void on_touch_pointer_up(int pointer_id, Point position)
{
    on_touch_pointer(pointer_id, position, touch_state::up);
}

DynObjectRef create_touchpoint(int n)
{
    if (n < 0 || n >= tp::MAX_POINTERS)
        quit("!Touch: invalid index for touchpoint");

    touch_point p = _tp.touch_points[n];
    return ScriptStructHelpers::CreateTouchPointRef(n, p.x, p.y, p.down);
}

void* Touch_GetTouchPoints()
{
    std::vector<DynObjectRef> objs{};

    for(int i=0; i< _tp.point_count; i++){
        objs.push_back(create_touchpoint(i));
    }

    DynObjectRef arr = DynamicArrayHelpers::CreateScriptArray(std::move(objs));
    return arr.Obj;
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "script/script_api.h"
#include "script/script_runtime.h"

// Point*[] ()
RuntimeScriptValue Sc_Touch_GetTouchPoints(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ(ScriptUserObject, globalDynamicArray, Touch_GetTouchPoints);
}

void RegisterTouchAPI()
{
    ScFnRegister touch_api[] = {
            { "Touch::GetTouchPoints^0",               API_FN_PAIR(Touch_GetTouchPoints) },
    };

    ccAddExternalFunctions(touch_api);
}
