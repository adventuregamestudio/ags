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
#include "ac/dynobj/scriptmotionpath.h"
#include "ac/dynobj/scriptpathfinder.h"
#include "ac/dynobj/scriptuserobject.h"
#include "ac/object.h"
#include "ac/route_finder.h"
#include "ac/spritecache.h"
#include "debug/debug_log.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern SpriteCache spriteset;

//=============================================================================
//
// Pathfinder API.
//
//=============================================================================

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
        return false;

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

//=============================================================================
//
// MotionPath API.
//
//=============================================================================

ScriptMotionPath *MotionPath_Create(void *path_arr, float speedx, float speedy, int repeat, int direction)
{
    std::vector<Point> path;
    if (!path_arr || !ScriptStructHelpers::ResolveArrayOfPoints(path_arr, path))
    {
        debug_script_warn("MotionPath.Create: path is null, or failed to resolve array of points");
        return nullptr;
    }
    ValidateAnimParams("MotionPath.Create", repeat, direction);
    return ScriptMotionPath::Create(path, speedx, speedy, RunPathParams(repeat, direction == 0));
}

ScriptMotionPath *MotionPath_Create2(void *path_arr, void *speedx_arr, void *speedy_arr, int repeat, int direction)
{
    std::vector<Point> path;
    std::vector<float> speedxs, speedys;
    if (!path_arr || !ScriptStructHelpers::ResolveArrayOfPoints(path_arr, path))
    {
        debug_script_warn("MotionPath.Create: path is null, or failed to resolve array of points");
        return nullptr;
    }
    if (!speedx_arr || !DynamicArrayHelpers::ResolveFloatArray(speedx_arr, speedxs))
    {
        debug_script_warn("MotionPath.Create: speedx array is null, or failed to resolve array of points");
        return nullptr;
    }
    if (!speedy_arr || !DynamicArrayHelpers::ResolveFloatArray(speedy_arr, speedys))
    {
        debug_script_warn("MotionPath.Create: speedy array is null, or failed to resolve array of points");
        return nullptr;
    }
    if (speedxs.size() < path.size() - 1 || speedys.size() < path.size() - 1)
    {
        debug_script_warn("MotionPath.Create: speeds arrays are smaller than the number of path's stages");
        return nullptr;
    }
    ValidateAnimParams("MotionPath.Create", repeat, direction);
    std::vector<Pointf> speeds;
    for (size_t i = 0; i < speedxs.size() && i < speedys.size(); ++i)
        speeds.emplace_back(speedxs[i], speedys[i]);
    return ScriptMotionPath::Create(path, speeds, RunPathParams(repeat, direction == 0));
}

void *MotionPath_GetPath(ScriptMotionPath *mpath)
{
    return ScriptStructHelpers::CreateArrayOfPoints(mpath->GetMoveList()->pos).Obj;
}

void MotionPath_StepBack(ScriptMotionPath *mpath)
{
    mpath->GetMoveList()->Backward();
}

void MotionPath_StepForward(ScriptMotionPath *mpath)
{
    mpath->GetMoveList()->Forward();
}

void MotionPath_Reset(ScriptMotionPath *mpath, int stage, float progress)
{
    mpath->GetMoveList()->ResetToStage(stage, progress);
}

int MotionPath_GetIsCompleted(ScriptMotionPath *mpath)
{
    return mpath->GetMoveList()->IsDone();
}

int MotionPath_GetStageCount(ScriptMotionPath *mpath)
{
    return mpath->GetMoveList()->GetNumStages();
}

int MotionPath_GetDirection(ScriptMotionPath *mpath)
{
    return mpath->GetMoveList()->GetRunParams().Forward ?
        FORWARDS : BACKWARDS;
}

int MotionPath_GetRepeatStyle(ScriptMotionPath *mpath)
{
    return mpath->GetMoveList()->GetRunParams().Repeat;
}

int MotionPath_GetWalkWhere(ScriptMotionPath *mpath)
{
    return mpath->GetMoveList()->IsStageDirect() ?
        ANYWHERE : WALKABLE_AREAS;
}

int MotionPath_GetStageX(ScriptMotionPath *mpath, int index)
{
    return mpath->GetMoveList()->GetStagePos(index).X;
}

int MotionPath_GetStageY(ScriptMotionPath *mpath, int index)
{
    return mpath->GetMoveList()->GetStagePos(index).Y;
}

float MotionPath_GetSpeedX(ScriptMotionPath *mpath, int index)
{
    return mpath->GetMoveList()->GetStageSpeed(index).X;
}

float MotionPath_GetSpeedY(ScriptMotionPath *mpath, int index)
{
    return mpath->GetMoveList()->GetStageSpeed(index).Y;
}

int MotionPath_GetStage(ScriptMotionPath *mpath)
{
    return mpath->GetMoveList()->GetStage();
}

float MotionPath_GetProgress(ScriptMotionPath *mpath)
{
    return mpath->GetMoveList()->GetStageProgress();
}

int MotionPath_GetPositionX(ScriptMotionPath *mpath)
{
    return mpath->GetMoveList()->GetCurrentPos().X;
}

int MotionPath_GetPositionY(ScriptMotionPath *mpath)
{
    return mpath->GetMoveList()->GetCurrentPos().Y;
}

RuntimeScriptValue Sc_MotionPath_Create(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_POBJ_PFLOAT2_PINT2(ScriptMotionPath, MotionPath_Create, void);
}

RuntimeScriptValue Sc_MotionPath_Create2(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_POBJ3_PINT2(ScriptMotionPath, MotionPath_Create2, void, void, void);
}

RuntimeScriptValue Sc_MotionPath_GetPath(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptMotionPath, void, globalDynamicArray, MotionPath_GetPath);
}

RuntimeScriptValue Sc_MotionPath_StepBack(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptMotionPath, MotionPath_StepBack);
}

RuntimeScriptValue Sc_MotionPath_StepForward(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptMotionPath, MotionPath_StepForward);
}

RuntimeScriptValue Sc_MotionPath_Reset(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT_PFLOAT(ScriptMotionPath, MotionPath_Reset);
}

RuntimeScriptValue Sc_MotionPath_GetDirection(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptMotionPath, MotionPath_GetDirection);
}

RuntimeScriptValue Sc_MotionPath_GetRepeatStyle(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptMotionPath, MotionPath_GetRepeatStyle);
}

RuntimeScriptValue Sc_MotionPath_GetWalkWhere(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptMotionPath, MotionPath_GetWalkWhere);
}

RuntimeScriptValue Sc_MotionPath_GetIsCompleted(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptMotionPath, MotionPath_GetIsCompleted);
}

RuntimeScriptValue Sc_MotionPath_GetStageCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptMotionPath, MotionPath_GetStageCount);
}

RuntimeScriptValue Sc_MotionPath_GetStageX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT(ScriptMotionPath, MotionPath_GetStageX);
}

RuntimeScriptValue Sc_MotionPath_GetStageY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT(ScriptMotionPath, MotionPath_GetStageY);
}

RuntimeScriptValue Sc_MotionPath_GetSpeedX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT_PINT(ScriptMotionPath, MotionPath_GetSpeedX);
}

RuntimeScriptValue Sc_MotionPath_GetSpeedY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT_PINT(ScriptMotionPath, MotionPath_GetSpeedY);
}

RuntimeScriptValue Sc_MotionPath_GetStage(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptMotionPath, MotionPath_GetStage);
}

RuntimeScriptValue Sc_MotionPath_GetProgress(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(ScriptMotionPath, MotionPath_GetProgress);
}

RuntimeScriptValue Sc_MotionPath_GetPositionX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptMotionPath, MotionPath_GetPositionX);
}

RuntimeScriptValue Sc_MotionPath_GetPositionY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptMotionPath, MotionPath_GetPositionY);
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

        { "MotionPath::Create",         API_FN_PAIR(MotionPath_Create) },
        { "MotionPath::Create2",        API_FN_PAIR(MotionPath_Create2) },
        { "MotionPath::GetPath",        API_FN_PAIR(MotionPath_GetPath) },
        { "MotionPath::StepBack",       API_FN_PAIR(MotionPath_StepBack) },
        { "MotionPath::StepForward",    API_FN_PAIR(MotionPath_StepForward) },
        { "MotionPath::Reset",          API_FN_PAIR(MotionPath_Reset) },
        { "MotionPath::get_Direction",  API_FN_PAIR(MotionPath_GetDirection) },
        { "MotionPath::get_RepeatStyle",API_FN_PAIR(MotionPath_GetRepeatStyle) },
        { "MotionPath::get_WalkWhere",  API_FN_PAIR(MotionPath_GetWalkWhere) },
        { "MotionPath::get_IsCompleted",API_FN_PAIR(MotionPath_GetIsCompleted) },
        { "MotionPath::get_StageCount", API_FN_PAIR(MotionPath_GetStageCount) },
        { "MotionPath::geti_StageX",    API_FN_PAIR(MotionPath_GetStageX) },
        { "MotionPath::geti_StageY",    API_FN_PAIR(MotionPath_GetStageY) },
        { "MotionPath::geti_SpeedX",    API_FN_PAIR(MotionPath_GetSpeedX) },
        { "MotionPath::geti_SpeedY",    API_FN_PAIR(MotionPath_GetSpeedY) },
        { "MotionPath::get_Stage",      API_FN_PAIR(MotionPath_GetStage) },
        { "MotionPath::get_Progress",   API_FN_PAIR(MotionPath_GetProgress) },
        { "MotionPath::get_PositionX",  API_FN_PAIR(MotionPath_GetPositionX) },
        { "MotionPath::get_PositionY",  API_FN_PAIR(MotionPath_GetPositionY) },
    };

    ccAddExternalFunctions(pathfinder_api);
}
