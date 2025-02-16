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
// Pathfinder script API.
//
//=============================================================================
#include "ac/dynobj/dynobj_manager.h"
#include "ac/dynobj/cc_dynamicarray.h"
#include "ac/dynobj/scriptpathfinder.h"
#include "ac/dynobj/scriptuserobject.h"
#include "ac/route_finder.h"
#include "ac/spritecache.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern SpriteCache spriteset;

void *Pathfinder_FindPath(ScriptPathfinder *pathfind, int srcx, int srcy, int dstx, int dsty)
{
    if (!pathfind->GetRouteFinder())
        return nullptr;

    pathfind->SyncPathfinder(); // sync with the source

    std::vector<Point> path;
    pathfind->GetRouteFinder()->FindRoute(path, srcx, srcy, dstx, dsty);
    if (path.empty())
        return nullptr;

    return ScriptStructHelpers::CreateArrayOfPoints(path).Obj;
}

ScriptUserObject *Pathfinder_Trace(ScriptPathfinder *pathfind, int srcx, int srcy, int dstx, int dsty)
{
    if (!pathfind->GetRouteFinder())
        return nullptr;

    pathfind->SyncPathfinder(); // sync with the source

    int lastx = srcx, lasty = srcy;
    pathfind->GetRouteFinder()->CanSeeFrom(srcx, srcy, dstx, dsty, &lastx, &lasty);
    return ScriptStructHelpers::CreatePoint(lastx, lasty);
}

bool Pathfinder_IsWalkableAt(ScriptPathfinder *pathfind, int x, int y)
{
    if (!pathfind->GetRouteFinder())
        return nullptr;

    pathfind->SyncPathfinder(); // sync with the source
    return pathfind->GetRouteFinder()->IsWalkableAt(x, y);
}

ScriptUserObject *Pathfinder_NearestWalkablePoint(ScriptPathfinder *pathfind, int x, int y)
{
    if (!pathfind->GetRouteFinder())
        return nullptr;

    pathfind->SyncPathfinder(); // sync with the source
    Point result;
    if (!pathfind->GetRouteFinder()->FindNearestWalkablePoint(Point(x, y), result))
        return nullptr;
    return ScriptStructHelpers::CreatePoint(result.X, result.Y);
}

ScriptMaskPathfinder *MaskPathfinder_Create(int mask_sprite)
{
    return ScriptMaskPathfinder::CreateFromMaskSprite(mask_sprite);
}

void MaskPathfinder_SetMask(ScriptMaskPathfinder *pathfind, int mask_sprite)
{
    pathfind->SetMaskSprite(mask_sprite);
}

//=============================================================================

RuntimeScriptValue Sc_Pathfinder_FindPath(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT4(ScriptPathfinder, void, globalDynamicArray, Pathfinder_FindPath);
}

RuntimeScriptValue Sc_Pathfinder_Trace(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJAUTO_PINT4(ScriptPathfinder, ScriptUserObject, Pathfinder_Trace);
}

RuntimeScriptValue Sc_Pathfinder_IsWalkableAt(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_PINT2(ScriptPathfinder, Pathfinder_IsWalkableAt);
}

RuntimeScriptValue Sc_Pathfinder_NearestWalkablePoint(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJAUTO_PINT2(ScriptPathfinder, ScriptUserObject, Pathfinder_NearestWalkablePoint);
}

RuntimeScriptValue Sc_MaskPathfinder_Create(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT(ScriptMaskPathfinder, MaskPathfinder_Create);
}

RuntimeScriptValue Sc_MaskPathfinder_SetMask(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptMaskPathfinder, MaskPathfinder_SetMask);
}

void RegisterPathfinderAPI()
{
    ScFnRegister pathfinder_api[] = {
        { "Pathfinder::FindPath",       API_FN_PAIR(Pathfinder_FindPath) },
        { "Pathfinder::Trace",          API_FN_PAIR(Pathfinder_Trace) },
        { "Pathfinder::IsWalkableAt",   API_FN_PAIR(Pathfinder_IsWalkableAt) },
        { "Pathfinder::NearestWalkablePoint", API_FN_PAIR(Pathfinder_NearestWalkablePoint) },

        { "MaskPathfinder::Create",     API_FN_PAIR(MaskPathfinder_Create) },
        { "MaskPathfinder::SetMask",    API_FN_PAIR(MaskPathfinder_SetMask) },
    };

    ccAddExternalFunctions(pathfinder_api);
}
