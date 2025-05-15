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
// Viewport and Camera script API.
//
//=============================================================================

#include "ac/dynobj/scriptcamera.h"
#include "ac/dynobj/scriptviewport.h"
#include "ac/dynobj/scriptuserobject.h"
#include "ac/draw.h"
#include "ac/gamestate.h"
#include "debug/debug_log.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS::Common;

//=============================================================================
//
// Camera script API.
//
//=============================================================================

ScriptCamera* Camera_Create()
{
    auto cam = play.CreateRoomCamera();
    if (!cam)
        return NULL;
    return play.RegisterRoomCamera(cam->GetID());
}

void Camera_Delete(ScriptCamera *scam)
{
    play.DeleteRoomCamera(scam->GetID());
}

int Camera_GetX(ScriptCamera *scam)
{
    if (scam->GetID() < 0) { debug_script_warn("Camera.X: trying to use deleted camera"); return 0; }
    return play.GetRoomCamera(scam->GetID())->GetRect().Left;
}

void Camera_SetX(ScriptCamera *scam, int x)
{
    if (scam->GetID() < 0) { debug_script_warn("Camera.X: trying to use deleted camera"); return; }
    auto cam = play.GetRoomCamera(scam->GetID());
    cam->LockAt(x, cam->GetRect().Top);
}

int Camera_GetY(ScriptCamera *scam)
{
    if (scam->GetID() < 0) { debug_script_warn("Camera.Y: trying to use deleted camera"); return 0; }
    return play.GetRoomCamera(scam->GetID())->GetRect().Top;
}

void Camera_SetY(ScriptCamera *scam, int y)
{
    if (scam->GetID() < 0) { debug_script_warn("Camera.Y: trying to use deleted camera"); return; }
    auto cam = play.GetRoomCamera(scam->GetID());
    cam->LockAt(cam->GetRect().Left, y);
}

int Camera_GetWidth(ScriptCamera *scam)
{
    if (scam->GetID() < 0) { debug_script_warn("Camera.Width: trying to use deleted camera"); return 0; }
    return play.GetRoomCamera(scam->GetID())->GetRect().GetWidth();
}

void Camera_SetWidth(ScriptCamera *scam, int width)
{
    if (scam->GetID() < 0) { debug_script_warn("Camera.Width: trying to use deleted camera"); return; }
    auto cam = play.GetRoomCamera(scam->GetID());
    cam->SetSize(Size(width, cam->GetRect().GetHeight()));
}

int Camera_GetHeight(ScriptCamera *scam)
{
    if (scam->GetID() < 0) { debug_script_warn("Camera.Height: trying to use deleted camera"); return 0; }
    return play.GetRoomCamera(scam->GetID())->GetRect().GetHeight();
}

void Camera_SetHeight(ScriptCamera *scam, int height)
{
    if (scam->GetID() < 0) { debug_script_warn("Camera.Height: trying to use deleted camera"); return; }
    auto cam = play.GetRoomCamera(scam->GetID());
    cam->SetSize(Size(cam->GetRect().GetWidth(), height));
}

float Camera_GetRotation(ScriptCamera *scam)
{
    if (scam->GetID() < 0) { debug_script_warn("Camera.Height: trying to use deleted camera"); return 0; }
    return play.GetRoomCamera(scam->GetID())->GetRotation();
}

void Camera_SetRotation(ScriptCamera *scam, float rotation)
{
    if (scam->GetID() < 0) { debug_script_warn("Camera.Height: trying to use deleted camera"); return; }
    auto cam = play.GetRoomCamera(scam->GetID());
    cam->SetRotation(rotation);
}

bool Camera_GetAutoTracking(ScriptCamera *scam)
{
    if (scam->GetID() < 0) { debug_script_warn("Camera.AutoTracking: trying to use deleted camera"); return false; }
    return !play.GetRoomCamera(scam->GetID())->IsLocked();
}

void Camera_SetAutoTracking(ScriptCamera *scam, bool on)
{
    if (scam->GetID() < 0) { debug_script_warn("Camera.AutoTracking: trying to use deleted camera"); return; }
    auto cam = play.GetRoomCamera(scam->GetID());
    if (on)
        cam->Release();
    else
        cam->Lock();
}

void Camera_SetAt(ScriptCamera *scam, int x, int y)
{
    if (scam->GetID() < 0) { debug_script_warn("Camera.SetAt: trying to use deleted camera"); return; }
    play.GetRoomCamera(scam->GetID())->LockAt(x, y);
}

void Camera_SetSize(ScriptCamera *scam, int width, int height)
{
    if (scam->GetID() < 0) { debug_script_warn("Camera.SetSize: trying to use deleted camera"); return; }
    play.GetRoomCamera(scam->GetID())->SetSize(Size(width, height));
}

RuntimeScriptValue Sc_Camera_Create(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO(ScriptCamera, Camera_Create);
}

RuntimeScriptValue Sc_Camera_Delete(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptCamera, Camera_Delete);
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

RuntimeScriptValue Sc_Camera_GetRotation(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(ScriptCamera, Camera_GetRotation);
}

RuntimeScriptValue Sc_Camera_SetRotation(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(ScriptCamera, Camera_SetRotation);
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

ScriptViewport* Viewport_Create()
{
    auto view = play.CreateRoomViewport();
    if (!view)
        return NULL;
    return play.RegisterRoomViewport(view->GetID());
}

void Viewport_Delete(ScriptViewport *scv)
{
    play.DeleteRoomViewport(scv->GetID());
}

int Viewport_GetX(ScriptViewport *scv)
{
    if (scv->GetID() < 0) { debug_script_warn("Viewport.X: trying to use deleted viewport"); return 0; }
    return play.GetRoomViewport(scv->GetID())->GetRect().Left;
}

void Viewport_SetX(ScriptViewport *scv, int x)
{
    if (scv->GetID() < 0) { debug_script_warn("Viewport.X: trying to use deleted viewport"); return; }
    auto view = play.GetRoomViewport(scv->GetID());
    view->SetAt(x, view->GetRect().Top);
}

int Viewport_GetY(ScriptViewport *scv)
{
    if (scv->GetID() < 0) { debug_script_warn("Viewport.Y: trying to use deleted viewport"); return 0; }
    return play.GetRoomViewport(scv->GetID())->GetRect().Top;
}

void Viewport_SetY(ScriptViewport *scv, int y)
{
    if (scv->GetID() < 0) { debug_script_warn("Viewport.Y: trying to use deleted viewport"); return; }
    auto view = play.GetRoomViewport(scv->GetID());
    view->SetAt(view->GetRect().Left, y);
}

int Viewport_GetWidth(ScriptViewport *scv)
{
    if (scv->GetID() < 0) { debug_script_warn("Viewport.Width: trying to use deleted viewport"); return 0; }
    return play.GetRoomViewport(scv->GetID())->GetRect().GetWidth();
}

void Viewport_SetWidth(ScriptViewport *scv, int width)
{
    if (scv->GetID() < 0) { debug_script_warn("Viewport.Width: trying to use deleted viewport"); return; }
    auto view = play.GetRoomViewport(scv->GetID());
    view->SetSize(Size(width, view->GetRect().GetHeight()));
}

int Viewport_GetHeight(ScriptViewport *scv)
{
    if (scv->GetID() < 0) { debug_script_warn("Viewport.Height: trying to use deleted viewport"); return 0; }
    return play.GetRoomViewport(scv->GetID())->GetRect().GetHeight();
}

void Viewport_SetHeight(ScriptViewport *scv, int height)
{
    if (scv->GetID() < 0) { debug_script_warn("Viewport.Height: trying to use deleted viewport"); return; }
    auto view = play.GetRoomViewport(scv->GetID());
    view->SetSize(Size(view->GetRect().GetWidth(), height));
}

ScriptCamera* Viewport_GetCamera(ScriptViewport *scv)
{
    if (scv->GetID() < 0) { debug_script_warn("Viewport.Camera: trying to use deleted viewport"); return nullptr; }
    auto view = play.GetRoomViewport(scv->GetID());
    auto cam = view->GetCamera();
    if (!cam)
        return nullptr;
    return play.GetScriptCamera(cam->GetID());
}

void Viewport_SetCamera(ScriptViewport *scv, ScriptCamera *scam)
{
    if (scv->GetID() < 0) { debug_script_warn("Viewport.Camera: trying to use deleted viewport"); return; }
    if (scam != nullptr && scam->GetID() < 0) { debug_script_warn("Viewport.Camera: trying to link deleted camera"); return; }
    auto view = play.GetRoomViewport(scv->GetID());
    // unlink previous camera
    auto cam = view->GetCamera();
    if (cam)
        cam->UnlinkFromViewport(view->GetID());
    // link new one
    if (scam != nullptr)
    {
        cam = play.GetRoomCamera(scam->GetID());
        view->LinkCamera(cam);
        cam->LinkToViewport(view);
    }
    else
    {
        view->LinkCamera(nullptr);
    }
}

bool Viewport_GetVisible(ScriptViewport *scv)
{
    if (scv->GetID() < 0) { debug_script_warn("Viewport.Visible: trying to use deleted viewport"); return false; }
    return play.GetRoomViewport(scv->GetID())->IsVisible();
}

void Viewport_SetVisible(ScriptViewport *scv, bool on)
{
    if (scv->GetID() < 0) { debug_script_warn("Viewport.Visible: trying to use deleted viewport"); return; }
    play.GetRoomViewport(scv->GetID())->SetVisible(on);
}

int Viewport_GetZOrder(ScriptViewport *scv)
{
    if (scv->GetID() < 0) { debug_script_warn("Viewport.ZOrder: trying to use deleted viewport"); return 0; }
    return play.GetRoomViewport(scv->GetID())->GetZOrder();
}

void Viewport_SetZOrder(ScriptViewport *scv, int zorder)
{
    if (scv->GetID() < 0) { debug_script_warn("Viewport.ZOrder: trying to use deleted viewport"); return; }
    play.GetRoomViewport(scv->GetID())->SetZOrder(zorder);
    play.InvalidateViewportZOrder();
}

int Viewport_GetShader(ScriptViewport *scv)
{
    if (scv->GetID() < 0) { debug_script_warn("Viewport.Shader: trying to use deleted viewport"); return 0; }
    return play.GetRoomViewport(scv->GetID())->GetShaderID();
}

void Viewport_SetShader(ScriptViewport *scv, int shader_id)
{
    if (scv->GetID() < 0) { debug_script_warn("Viewport.Shader: trying to use deleted viewport"); return; }
    play.GetRoomViewport(scv->GetID())->SetShaderID(shader_id);
}

ScriptViewport* Viewport_GetAtScreenXY(int x, int y)
{
    PViewport view = play.GetRoomViewportAt(x, y);
    if (!view)
        return nullptr;
    return play.GetScriptViewport(view->GetID());
}

void Viewport_SetPosition(ScriptViewport *scv, int x, int y, int width, int height)
{
    if (scv->GetID() < 0) { debug_script_warn("Viewport.SetPosition: trying to use deleted viewport"); return; }
    play.GetRoomViewport(scv->GetID())->SetRect(RectWH(x, y, width, height));
}

ScriptUserObject *Viewport_ScreenToRoomPoint(ScriptViewport *scv, int scrx, int scry, bool clipViewport)
{
    if (scv->GetID() < 0) { debug_script_warn("Viewport.ScreenToRoomPoint: trying to use deleted viewport"); return nullptr; }
    VpPoint vpt = play.GetRoomViewport(scv->GetID())->ScreenToRoom(scrx, scry, clipViewport);
    if (vpt.second < 0)
        return nullptr;
    return ScriptStructHelpers::CreatePoint(vpt.first.X, vpt.first.Y);
}

ScriptUserObject *Viewport_RoomToScreenPoint(ScriptViewport *scv, int roomx, int roomy, bool clipViewport)
{
    if (scv->GetID() < 0) { debug_script_warn("Viewport.RoomToScreenPoint: trying to use deleted viewport"); return nullptr; }
    Point pt = play.RoomToScreen(roomx, roomy);
    if (clipViewport && !play.GetRoomViewport(scv->GetID())->GetRect().IsInside(pt.X, pt.Y))
        return nullptr;
    return ScriptStructHelpers::CreatePoint(pt.X, pt.Y);
}

RuntimeScriptValue Sc_Viewport_Create(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO(ScriptViewport, Viewport_Create);
}

RuntimeScriptValue Sc_Viewport_Delete(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptViewport, Viewport_Delete);
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

RuntimeScriptValue Sc_Viewport_SetCamera(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(ScriptViewport, Viewport_SetCamera, ScriptCamera);
}

RuntimeScriptValue Sc_Viewport_GetVisible(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptViewport, Viewport_GetVisible);
}

RuntimeScriptValue Sc_Viewport_SetVisible(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PBOOL(ScriptViewport, Viewport_SetVisible);
}

RuntimeScriptValue Sc_Viewport_GetZOrder(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptViewport, Viewport_GetZOrder);
}

RuntimeScriptValue Sc_Viewport_SetZOrder(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptViewport, Viewport_SetZOrder);
}

RuntimeScriptValue Sc_Viewport_GetShader(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptViewport, Viewport_GetShader);
}

RuntimeScriptValue Sc_Viewport_SetShader(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptViewport, Viewport_SetShader);
}

RuntimeScriptValue Sc_Viewport_GetAtScreenXY(const RuntimeScriptValue *params, int32_t param_count)
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
    API_OBJCALL_OBJAUTO_PINT2_PBOOL(ScriptViewport, ScriptUserObject, Viewport_RoomToScreenPoint);
}



void RegisterViewportAPI()
{
    ScFnRegister camera_api[] = {
        { "Camera::Create",             API_FN_PAIR(Camera_Create) },
        { "Camera::Delete",             API_FN_PAIR(Camera_Delete) },
        { "Camera::get_X",              API_FN_PAIR(Camera_GetX) },
        { "Camera::set_X",              API_FN_PAIR(Camera_SetX) },
        { "Camera::get_Y",              API_FN_PAIR(Camera_GetY) },
        { "Camera::set_Y",              API_FN_PAIR(Camera_SetY) },
        { "Camera::get_Width",          API_FN_PAIR(Camera_GetWidth) },
        { "Camera::set_Width",          API_FN_PAIR(Camera_SetWidth) },
        { "Camera::get_Height",         API_FN_PAIR(Camera_GetHeight) },
        { "Camera::set_Height",         API_FN_PAIR(Camera_SetHeight) },
        { "Camera::get_Rotation",       API_FN_PAIR(Camera_GetRotation) },
        { "Camera::set_Rotation",       API_FN_PAIR(Camera_SetRotation) },
        { "Camera::get_AutoTracking",   API_FN_PAIR(Camera_GetAutoTracking) },
        { "Camera::set_AutoTracking",   API_FN_PAIR(Camera_SetAutoTracking) },
        { "Camera::SetAt",              API_FN_PAIR(Camera_SetAt) },
        { "Camera::SetSize",            API_FN_PAIR(Camera_SetSize) },
    };

    ccAddExternalFunctions(camera_api);

    ScFnRegister viewport_api[] = {
        { "Viewport::Create",           API_FN_PAIR(Viewport_Create) },
        { "Viewport::Delete",           API_FN_PAIR(Viewport_Delete) },
        { "Viewport::get_X",            API_FN_PAIR(Viewport_GetX) },
        { "Viewport::set_X",            API_FN_PAIR(Viewport_SetX) },
        { "Viewport::get_Y",            API_FN_PAIR(Viewport_GetY) },
        { "Viewport::set_Y",            API_FN_PAIR(Viewport_SetY) },
        { "Viewport::get_Width",        API_FN_PAIR(Viewport_GetWidth) },
        { "Viewport::set_Width",        API_FN_PAIR(Viewport_SetWidth) },
        { "Viewport::get_Height",       API_FN_PAIR(Viewport_GetHeight) },
        { "Viewport::set_Height",       API_FN_PAIR(Viewport_SetHeight) },
        { "Viewport::get_Camera",       API_FN_PAIR(Viewport_GetCamera) },
        { "Viewport::set_Camera",       API_FN_PAIR(Viewport_SetCamera) },
        { "Viewport::get_Visible",      API_FN_PAIR(Viewport_GetVisible) },
        { "Viewport::set_Visible",      API_FN_PAIR(Viewport_SetVisible) },
        { "Viewport::get_ZOrder",       API_FN_PAIR(Viewport_GetZOrder) },
        { "Viewport::set_ZOrder",       API_FN_PAIR(Viewport_SetZOrder) },
        { "Viewport::GetAtScreenXY",    API_FN_PAIR(Viewport_GetAtScreenXY) },
        { "Viewport::SetPosition",      API_FN_PAIR(Viewport_SetPosition) },
        { "Viewport::ScreenToRoomPoint", API_FN_PAIR(Viewport_ScreenToRoomPoint) },
        { "Viewport::RoomToScreenPoint", API_FN_PAIR(Viewport_RoomToScreenPoint) },

        { "Viewport::get_Shader",       API_FN_PAIR(Viewport_GetShader) },
        { "Viewport::set_Shader",       API_FN_PAIR(Viewport_SetShader) },
    };

    ccAddExternalFunctions(viewport_api);
}
