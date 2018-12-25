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
#include "ac/draw.h"
#include "ac/gamestate.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

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

float Camera_GetScaleX(ScriptCamera *)
{
    return play.GetRoomCameraObj().ScaleX;
}

void Camera_SetScaleX(ScriptCamera *, float sx)
{
    return play.SetRoomCameraAutoSize(sx, play.GetRoomCameraObj().ScaleY);
}

float Camera_GetScaleY(ScriptCamera *)
{
    return play.GetRoomCameraObj().ScaleY;
}

void Camera_SetScaleY(ScriptCamera *, float sy)
{
    return play.SetRoomCameraAutoSize(play.GetRoomCameraObj().ScaleX, sy);
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

RuntimeScriptValue Sc_Camera_GetScaleX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(ScriptCamera, Camera_GetScaleX);
}

RuntimeScriptValue Sc_Camera_SetScaleX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(ScriptCamera, Camera_SetScaleX);
}

RuntimeScriptValue Sc_Camera_GetScaleY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(ScriptCamera, Camera_GetScaleY);
}

RuntimeScriptValue Sc_Camera_SetScaleY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(ScriptCamera, Camera_SetScaleY);
}

RuntimeScriptValue Sc_Camera_GetAutoTracking(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptCamera, Camera_GetAutoTracking);
}

RuntimeScriptValue Sc_Camera_SetAutoTracking(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PBOOL(ScriptCamera, Camera_SetAutoTracking);
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
    ccAddExternalObjectFunction("Camera::get_ScaleX", Sc_Camera_GetScaleX);
    ccAddExternalObjectFunction("Camera::set_ScaleX", Sc_Camera_SetScaleX);
    ccAddExternalObjectFunction("Camera::get_ScaleY", Sc_Camera_GetScaleY);
    ccAddExternalObjectFunction("Camera::set_ScaleY", Sc_Camera_SetScaleY);
    ccAddExternalObjectFunction("Camera::get_AutoTracking", Sc_Camera_GetAutoTracking);
    ccAddExternalObjectFunction("Camera::set_AutoTracking", Sc_Camera_SetAutoTracking);

    ccAddExternalObjectFunction("Viewport::get_X", Sc_Viewport_GetX);
    ccAddExternalObjectFunction("Viewport::set_X", Sc_Viewport_SetX);
    ccAddExternalObjectFunction("Viewport::get_Y", Sc_Viewport_GetY);
    ccAddExternalObjectFunction("Viewport::set_Y", Sc_Viewport_SetY);
    ccAddExternalObjectFunction("Viewport::get_Width", Sc_Viewport_GetWidth);
    ccAddExternalObjectFunction("Viewport::set_Width", Sc_Viewport_SetWidth);
    ccAddExternalObjectFunction("Viewport::get_Height", Sc_Viewport_GetHeight);
    ccAddExternalObjectFunction("Viewport::set_Height", Sc_Viewport_SetHeight);
    ccAddExternalObjectFunction("Viewport::get_Camera", Sc_Viewport_GetCamera);
}
