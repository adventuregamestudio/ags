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
// Viewport and Camera script API.
//
//=============================================================================

#include "ac/dynobj/scriptcamera.h"
#include "ac/dynobj/scriptviewport.h"
#include "ac/dynobj/scriptuserobject.h"
#include "ac/draw.h"
#include "ac/gamestate.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS::Common;

//=============================================================================
//
// Camera script API.
//
//=============================================================================

int Camera_GetX(ScriptCamera *)
{
    int x = play.GetRoomCameraObj().Position.Left;
    return divide_down_coordinate(x);
}

void Camera_SetX(ScriptCamera *, int x)
{
    x = multiply_up_coordinate(x);
    play.LockRoomCameraAt(x, play.GetRoomCameraObj().Position.Top);
}

int Camera_GetY(ScriptCamera *)
{
    int y = play.GetRoomCameraObj().Position.Top;
    return divide_down_coordinate(y);
}

void Camera_SetY(ScriptCamera *, int y)
{
    y = multiply_up_coordinate(y);
    play.LockRoomCameraAt(play.GetRoomCameraObj().Position.Left, y);
}

int Camera_GetWidth(ScriptCamera *)
{
    int width = play.GetRoomCameraObj().Position.GetWidth();
    return divide_down_coordinate(width);
}

void Camera_SetWidth(ScriptCamera *, int width)
{
    width = multiply_up_coordinate(width);
    play.SetRoomCameraSize(Size(width, play.GetRoomCamera().GetHeight()));
}

int Camera_GetHeight(ScriptCamera *)
{
    int height = play.GetRoomCameraObj().Position.GetHeight();
    return divide_down_coordinate(height);
}

void Camera_SetHeight(ScriptCamera *, int height)
{
    height = multiply_up_coordinate(height);
    play.SetRoomCameraSize(Size(play.GetRoomCamera().GetWidth(), height));
}

bool Camera_GetAutoTracking(ScriptCamera *)
{
    return !play.IsRoomCameraLocked();
}

void Camera_SetAutoTracking(ScriptCamera *, bool on)
{
    if (on)
        play.ReleaseRoomCamera(); 
    else
        play.LockRoomCamera();
}

void Camera_SetAt(ScriptCamera *, int x, int y)
{
    multiply_up_coordinates(&x, &y);
    play.LockRoomCameraAt(x, y);
}

void Camera_SetSize(ScriptCamera *, int width, int height)
{
    multiply_up_coordinates(&width, &height);
    play.SetRoomCameraSize(Size(width, height));
}

RuntimeScriptValue Sc_Camera_GetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptCamera, Camera_GetX);
}

RuntimeScriptValue Sc_Camera_SetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptCamera, Camera_SetX);
}

RuntimeScriptValue Sc_Camera_GetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptCamera, Camera_GetY);
}

RuntimeScriptValue Sc_Camera_SetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptCamera, Camera_SetY);
}

RuntimeScriptValue Sc_Camera_GetWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptCamera, Camera_GetWidth);
}

RuntimeScriptValue Sc_Camera_SetWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptCamera, Camera_SetWidth);
}

RuntimeScriptValue Sc_Camera_GetHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptCamera, Camera_GetHeight);
}

RuntimeScriptValue Sc_Camera_SetHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptCamera, Camera_SetHeight);
}

RuntimeScriptValue Sc_Camera_GetAutoTracking(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptCamera, Camera_GetAutoTracking);
}

RuntimeScriptValue Sc_Camera_SetAutoTracking(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PBOOL(ScriptCamera, Camera_SetAutoTracking);
}

RuntimeScriptValue Sc_Camera_SetAt(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(ScriptCamera, Camera_SetAt);
}

RuntimeScriptValue Sc_Camera_SetSize(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(ScriptCamera, Camera_SetSize);
}


//=============================================================================
//
// Viewport script API.
//
//=============================================================================

int Viewport_GetX(ScriptViewport *view)
{
    int x = play.GetRoomViewport().Left;
    return divide_down_coordinate(x);
}

void Viewport_SetX(ScriptViewport *, int x)
{
    x = multiply_up_coordinate(x);
    Rect view = play.GetRoomViewport();
    view.MoveToX(x);
    play.SetRoomViewport(view);
}

int Viewport_GetY(ScriptViewport *)
{
    int y = play.GetRoomViewport().Top;
    return divide_down_coordinate(y);
}

void Viewport_SetY(ScriptViewport *, int y)
{
    y = multiply_up_coordinate(y);
    Rect view = play.GetRoomViewport();
    view.MoveToY(y);
    play.SetRoomViewport(view);
}

int Viewport_GetWidth(ScriptViewport *)
{
    int width = play.GetRoomViewport().GetWidth();
    return divide_down_coordinate(width);
}

void Viewport_SetWidth(ScriptViewport *, int width)
{
    width = multiply_up_coordinate(width);
    Rect view = play.GetRoomViewport();
    view.SetWidth(width);
    play.SetRoomViewport(view);
}

int Viewport_GetHeight(ScriptViewport *)
{
    int height = play.GetRoomViewport().GetHeight();
    return divide_down_coordinate(height);
}

void Viewport_SetHeight(ScriptViewport *, int height)
{
    height = multiply_up_coordinate(height);
    Rect view = play.GetRoomViewport();
    view.SetHeight(height);
    play.SetRoomViewport(view);
}

ScriptCamera* Viewport_GetCamera(ScriptViewport *)
{
    ScriptCamera *camera = new ScriptCamera();
    ccRegisterManagedObject(camera, camera);
    return camera;
}

ScriptViewport* Viewport_GetAtScreenXY(int x, int y)
{
    multiply_up_coordinates(&x, &y);

    const Rect &view = play.GetRoomViewport();
    if (!view.IsInside(x, y))
        return NULL;

    ScriptViewport *viewport = new ScriptViewport();
    ccRegisterManagedObject(viewport, viewport);
    return viewport;
}

void Viewport_SetPosition(ScriptViewport *, int x, int y, int width, int height)
{
    multiply_up_coordinates(&x, &y);
    multiply_up_coordinates(&width, &height);
    play.SetRoomViewport(RectWH(x, y, width, height));
}

ScriptUserObject *Viewport_ScreenToRoomPoint(ScriptViewport *, int scrx, int scry, bool clipViewport)
{
    multiply_up_coordinates(&scrx, &scry);

    VpPoint vpt = play.ScreenToRoom(scrx, scry, clipViewport);
    if (vpt.second < 0)
        return NULL;

    divide_down_coordinates(vpt.first.X, vpt.first.Y);
    return ScriptStructHelpers::CreatePoint(vpt.first.X, vpt.first.Y);
}

ScriptUserObject *Sc_Viewport_RoomToScreenPoint(ScriptViewport *, int roomx, int roomy, bool clipViewport)
{
    multiply_up_coordinates(&roomx, &roomy);

    const Rect &view = play.GetRoomViewport();
    Point pt = play.RoomToScreen(roomx, roomy);
    if (clipViewport && !view.IsInside(pt.X, pt.Y))
        return NULL;

    divide_down_coordinates(pt.X, pt.Y);
    return ScriptStructHelpers::CreatePoint(pt.X, pt.Y);
}

RuntimeScriptValue Sc_Viewport_GetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptViewport, Viewport_GetX);
}

RuntimeScriptValue Sc_Viewport_SetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptViewport, Viewport_SetX);
}

RuntimeScriptValue Sc_Viewport_GetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptViewport, Viewport_GetY);
}

RuntimeScriptValue Sc_Viewport_SetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptViewport, Viewport_SetY);
}

RuntimeScriptValue Sc_Viewport_GetWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptViewport, Viewport_GetWidth);
}

RuntimeScriptValue Sc_Viewport_SetWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptViewport, Viewport_SetWidth);
}

RuntimeScriptValue Sc_Viewport_GetHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptViewport, Viewport_GetHeight);
}

RuntimeScriptValue Sc_Viewport_SetHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptViewport, Viewport_SetHeight);
}

RuntimeScriptValue Sc_Viewport_GetCamera(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJAUTO(ScriptViewport, ScriptCamera, Viewport_GetCamera);
}

RuntimeScriptValue Sc_Viewport_GetAtScreenXY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT2(ScriptViewport, Viewport_GetAtScreenXY);
}

RuntimeScriptValue Sc_Viewport_SetPosition(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT4(ScriptViewport, Viewport_SetPosition);
}

RuntimeScriptValue Sc_Viewport_ScreenToRoomPoint(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJAUTO_PINT2_PBOOL(ScriptViewport, ScriptUserObject, Viewport_ScreenToRoomPoint);
}

RuntimeScriptValue Sc_Viewport_RoomToScreenPoint(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJAUTO_PINT2_PBOOL(ScriptViewport, ScriptUserObject, Sc_Viewport_RoomToScreenPoint);
}



void RegisterViewportAPI()
{
    ccAddExternalObjectFunction("Camera::get_X", Sc_Camera_GetX);
    ccAddExternalObjectFunction("Camera::set_X", Sc_Camera_SetX);
    ccAddExternalObjectFunction("Camera::get_Y", Sc_Camera_GetY);
    ccAddExternalObjectFunction("Camera::set_Y", Sc_Camera_SetY);
    ccAddExternalObjectFunction("Camera::get_Width", Sc_Camera_GetWidth);
    ccAddExternalObjectFunction("Camera::set_Width", Sc_Camera_SetWidth);
    ccAddExternalObjectFunction("Camera::get_Height", Sc_Camera_GetHeight);
    ccAddExternalObjectFunction("Camera::set_Height", Sc_Camera_SetHeight);
    ccAddExternalObjectFunction("Camera::get_AutoTracking", Sc_Camera_GetAutoTracking);
    ccAddExternalObjectFunction("Camera::set_AutoTracking", Sc_Camera_SetAutoTracking);
    ccAddExternalObjectFunction("Camera::SetAt", Sc_Camera_SetAt);
    ccAddExternalObjectFunction("Camera::SetSize", Sc_Camera_SetSize);

    ccAddExternalObjectFunction("Viewport::get_X", Sc_Viewport_GetX);
    ccAddExternalObjectFunction("Viewport::set_X", Sc_Viewport_SetX);
    ccAddExternalObjectFunction("Viewport::get_Y", Sc_Viewport_GetY);
    ccAddExternalObjectFunction("Viewport::set_Y", Sc_Viewport_SetY);
    ccAddExternalObjectFunction("Viewport::get_Width", Sc_Viewport_GetWidth);
    ccAddExternalObjectFunction("Viewport::set_Width", Sc_Viewport_SetWidth);
    ccAddExternalObjectFunction("Viewport::get_Height", Sc_Viewport_GetHeight);
    ccAddExternalObjectFunction("Viewport::set_Height", Sc_Viewport_SetHeight);
    ccAddExternalObjectFunction("Viewport::get_Camera", Sc_Viewport_GetCamera);
    ccAddExternalObjectFunction("Viewport::GetAtScreenXY", Sc_Viewport_GetAtScreenXY);
    ccAddExternalObjectFunction("Viewport::SetPosition", Sc_Viewport_SetPosition);
    ccAddExternalObjectFunction("Viewport::ScreenToRoomPoint", Sc_Viewport_ScreenToRoomPoint);
    ccAddExternalObjectFunction("Viewport::RoomToScreenPoint", Sc_Viewport_RoomToScreenPoint);
}
