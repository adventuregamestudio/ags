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
    return play.GetRoomCamera(scam->GetID())->GetRect().Left;
}

void Camera_SetX(ScriptCamera *scam, int x)
{
    auto cam = play.GetRoomCamera(scam->GetID());
    cam->LockAt(x, cam->GetRect().Top);
}

int Camera_GetY(ScriptCamera *scam)
{
    return play.GetRoomCamera(scam->GetID())->GetRect().Top;
}

void Camera_SetY(ScriptCamera *scam, int y)
{
    auto cam = play.GetRoomCamera(scam->GetID());
    cam->LockAt(cam->GetRect().Left, y);
}

int Camera_GetWidth(ScriptCamera *scam)
{
    return play.GetRoomCamera(scam->GetID())->GetRect().GetWidth();
}

void Camera_SetWidth(ScriptCamera *scam, int width)
{
    auto cam = play.GetRoomCamera(scam->GetID());
    cam->SetSize(Size(width, cam->GetRect().GetHeight()));
}

int Camera_GetHeight(ScriptCamera *scam)
{
    return play.GetRoomCamera(scam->GetID())->GetRect().GetHeight();
}

void Camera_SetHeight(ScriptCamera *scam, int height)
{
    auto cam = play.GetRoomCamera(scam->GetID());
    cam->SetSize(Size(cam->GetRect().GetWidth(), height));
}

bool Camera_GetAutoTracking(ScriptCamera *scam)
{
    return !play.GetRoomCamera(scam->GetID())->IsLocked();
}

void Camera_SetAutoTracking(ScriptCamera *scam, bool on)
{
    auto cam = play.GetRoomCamera(scam->GetID());
    if (on)
        cam->Release();
    else
        cam->Lock();
}

void Camera_SetAt(ScriptCamera *scam, int x, int y)
{
    play.GetRoomCamera(scam->GetID())->LockAt(x, y);
}

void Camera_SetSize(ScriptCamera *scam, int width, int height)
{
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
    return play.GetRoomViewport(scv->GetID()).Left;
}

void Viewport_SetX(ScriptViewport *scv, int x)
{
    Rect view = play.GetRoomViewport(scv->GetID());
    view.MoveToX(x);
    play.SetRoomViewport(scv->GetID(), view);
}

int Viewport_GetY(ScriptViewport *scv)
{
    return play.GetRoomViewport(scv->GetID()).Top;
}

void Viewport_SetY(ScriptViewport *scv, int y)
{
    Rect view = play.GetRoomViewport(scv->GetID());
    view.MoveToY(y);
    play.SetRoomViewport(scv->GetID(), view);
}

int Viewport_GetWidth(ScriptViewport *scv)
{
    return play.GetRoomViewport(scv->GetID()).GetWidth();
}

void Viewport_SetWidth(ScriptViewport *scv, int width)
{
    Rect view = play.GetRoomViewport(scv->GetID());
    view.SetWidth(width);
    play.SetRoomViewport(scv->GetID(), view);
}

int Viewport_GetHeight(ScriptViewport *scv)
{
    return play.GetRoomViewport(scv->GetID()).GetHeight();
}

void Viewport_SetHeight(ScriptViewport *scv, int height)
{
    Rect view = play.GetRoomViewport(scv->GetID());
    view.SetHeight(height);
    play.SetRoomViewport(scv->GetID(), view);
}

ScriptCamera* Viewport_GetCamera(ScriptViewport *scv)
{
    auto view = play.GetRoomViewportObj(scv->GetID());
    auto cam = view->GetCamera();
    if (!cam)
        return nullptr;
    return play.GetScriptCamera(cam->GetID());
}

void Viewport_SetCamera(ScriptViewport *scv, ScriptCamera *scam)
{
    auto view = play.GetRoomViewportObj(scv->GetID());
    auto cam = play.GetRoomCamera(scam->GetID());
    if (view != nullptr && cam != nullptr)
    {
        view->LinkCamera(cam);
        cam->LinkToViewport(view);
    }
}

bool Viewport_GetVisible(ScriptViewport *scv)
{
    auto view = play.GetRoomViewportObj(scv->GetID());
    if (view != nullptr)
        return view->IsVisible();
    return false;
}

void Viewport_SetVisible(ScriptViewport *scv, bool on)
{
    auto view = play.GetRoomViewportObj(scv->GetID());
    if (view != nullptr)
        view->SetVisible(on);
}

int Viewport_GetZOrder(ScriptViewport *scv)
{
    auto view = play.GetRoomViewportObj(scv->GetID());
    if (view != nullptr)
        return view->GetZOrder();
    return 0;
}

void Viewport_SetZOrder(ScriptViewport *scv, int zorder)
{
    auto view = play.GetRoomViewportObj(scv->GetID());
    if (view != nullptr)
        view->SetZOrder(zorder);
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
    play.SetRoomViewport(scv->GetID(), RectWH(x, y, width, height));
}

ScriptUserObject *Viewport_ScreenToRoomPoint(ScriptViewport *scv, int scrx, int scry, bool clipViewport)
{
    VpPoint vpt = play.ScreenToRoom(scrx, scry, scv->GetID(), clipViewport);
    if (vpt.second < 0)
        return nullptr;
    return ScriptStructHelpers::CreatePoint(vpt.first.X, vpt.first.Y);
}

ScriptUserObject *Viewport_RoomToScreenPoint(ScriptViewport *scv, int roomx, int roomy, bool clipViewport)
{
    const Rect &view = play.GetRoomViewport(scv->GetID());
    Point pt = play.RoomToScreen(roomx, roomy);
    if (clipViewport && !view.IsInside(pt.X, pt.Y))
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
    API_OBJCALL_OBJAUTO_PINT2_PBOOL(ScriptViewport, ScriptUserObject, Viewport_RoomToScreenPoint);
}



void RegisterViewportAPI()
{
    ccAddExternalStaticFunction("Camera::Create", Sc_Camera_Create);
    ccAddExternalObjectFunction("Camera::Delete", Sc_Camera_Delete);
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

    ccAddExternalStaticFunction("Viewport::Create", Sc_Viewport_Create);
    ccAddExternalObjectFunction("Viewport::Delete", Sc_Viewport_Delete);
    ccAddExternalObjectFunction("Viewport::get_X", Sc_Viewport_GetX);
    ccAddExternalObjectFunction("Viewport::set_X", Sc_Viewport_SetX);
    ccAddExternalObjectFunction("Viewport::get_Y", Sc_Viewport_GetY);
    ccAddExternalObjectFunction("Viewport::set_Y", Sc_Viewport_SetY);
    ccAddExternalObjectFunction("Viewport::get_Width", Sc_Viewport_GetWidth);
    ccAddExternalObjectFunction("Viewport::set_Width", Sc_Viewport_SetWidth);
    ccAddExternalObjectFunction("Viewport::get_Height", Sc_Viewport_GetHeight);
    ccAddExternalObjectFunction("Viewport::set_Height", Sc_Viewport_SetHeight);
    ccAddExternalObjectFunction("Viewport::get_Camera", Sc_Viewport_GetCamera);
    ccAddExternalObjectFunction("Viewport::set_Camera", Sc_Viewport_SetCamera);
    ccAddExternalObjectFunction("Viewport::get_Visible", Sc_Viewport_GetVisible);
    ccAddExternalObjectFunction("Viewport::set_Visible", Sc_Viewport_SetVisible);
    ccAddExternalObjectFunction("Viewport::get_ZOrder", Sc_Viewport_GetZOrder);
    ccAddExternalObjectFunction("Viewport::set_ZOrder", Sc_Viewport_SetZOrder);
    ccAddExternalObjectFunction("Viewport::GetAtScreenXY", Sc_Viewport_GetAtScreenXY);
    ccAddExternalObjectFunction("Viewport::SetPosition", Sc_Viewport_SetPosition);
    ccAddExternalObjectFunction("Viewport::ScreenToRoomPoint", Sc_Viewport_ScreenToRoomPoint);
    ccAddExternalObjectFunction("Viewport::RoomToScreenPoint", Sc_Viewport_RoomToScreenPoint);
}
