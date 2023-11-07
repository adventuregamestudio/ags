//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "ac/draw.h"
#include "ac/gamestate.h"
#include "debug/debug_log.h"
#include "game/roomstruct.h"
#include "game/viewport.h"

using namespace AGS::Common;

extern RoomStruct thisroom;

void Camera::SetID(int id)
{
    _id = id;
}

// Returns Room camera position and size inside the room (in room coordinates)
const Rect &Camera::GetRect() const
{
    return _position;
}

// Sets explicit room camera's orthographic size
void Camera::SetSize(const Size cam_size)
{
    // TODO: currently we don't support having camera larger than room background
    // (or rather - looking outside of the room background); look into this later
    const Size real_room_sz = Size(thisroom.Width, thisroom.Height);
    Size real_size = Size::Clamp(cam_size, Size(1, 1), real_room_sz);
    if (_position.GetWidth() == real_size.Width && _position.GetHeight() == real_size.Height)
        return;

    _position.SetWidth(real_size.Width);
    _position.SetHeight(real_size.Height);
    // readjust in case went off-room after size changed
    int x = Math::Clamp(_position.Left, 0, thisroom.Width - _position.GetWidth());
    int y = Math::Clamp(_position.Top, 0, thisroom.Height - _position.GetHeight());
    if (_position.Left != x || _position.Top != y)
    {
        _position.MoveTo(Point(x, y));
        _hasChangedPosition = true;
    }
    AdjustTransformations();
    _hasChangedSize = true;
}

// Puts room camera to the new location in the room
void Camera::SetAt(int x, int y)
{
    x = Math::Clamp(x, 0, thisroom.Width - _position.GetWidth());
    y = Math::Clamp(y, 0, thisroom.Height - _position.GetHeight());
    if (_position.Left == x && _position.Top == y)
        return;

    _position.MoveTo(Point(x, y));
    AdjustTransformations();
    _hasChangedPosition = true;
}

float Camera::GetRotation() const
{
    return _rotation;
}

void Camera::SetRotation(float degrees)
{
    _rotation = Math::ClampAngle360(degrees);
    AdjustTransformations();
    _hasChangedSize = true;
}

void Camera::AdjustTransformations()
{
    for (auto vp = _viewportRefs.begin(); vp != _viewportRefs.end(); ++vp)
    {
        auto locked_vp = vp->lock();
        if (locked_vp)
            locked_vp->AdjustTransformation();
    }
}

// Tells if camera is currently locked at custom position
bool Camera::IsLocked() const
{
    return _locked;
}

// Locks room camera at its current position
void Camera::Lock()
{
    debug_script_log("Room camera locked");
    _locked = true;
}

// Similar to SetAt, but also locks camera preventing it from following player character
void Camera::LockAt(int x, int y)
{
    debug_script_log("Room camera locked to %d,%d", x, y);
    SetAt(x, y);
    _locked = true;
}

// Releases camera lock, letting it follow player character
void Camera::Release()
{
    _locked = false;
    debug_script_log("Room camera released back to engine control");
}

// Link this camera to a new viewport; this does not unlink any linked ones
void Camera::LinkToViewport(ViewportRef viewport)
{
    auto new_locked = viewport.lock();
    if (!new_locked)
        return;
    for (auto vp = _viewportRefs.begin(); vp != _viewportRefs.end(); ++vp)
    {
        auto old_locked = vp->lock();
        if (old_locked->GetID() == new_locked->GetID())
            return;
    }
    _viewportRefs.push_back(viewport);
}

// Unlinks this camera from a given viewport; does nothing if link did not exist
void Camera::UnlinkFromViewport(int id)
{
    for (auto vp = _viewportRefs.begin(); vp != _viewportRefs.end(); ++vp)
    {
        auto locked = vp->lock();
        if (locked && locked->GetID() == id)
        {
            _viewportRefs.erase(vp);
            return;
        }
    }
}

const std::vector<ViewportRef> &Camera::GetLinkedViewports() const
{
    return _viewportRefs;
}

void Viewport::SetID(int id)
{
    _id = id;
}

void Viewport::SetRect(const Rect &rc)
{
    // TODO: consider allowing size 0,0, in which case viewport is considered not visible
    Size fix_size = rc.GetSize().IsNull() ? Size(1, 1) : rc.GetSize();
    Rect new_pos = RectWH(rc.Left, rc.Top, fix_size.Width, fix_size.Height);
    if (new_pos == _position)
        return;
    _position = new_pos;
    AdjustTransformation();
    _hasChangedPosition = true;
    _hasChangedSize = true;
}

void Viewport::SetSize(const Size sz)
{
    // TODO: consider allowing size 0,0, in which case viewport is considered not visible
    Size fix_size = sz.IsNull() ? Size(1, 1) : sz;
    if (_position.GetWidth() == fix_size.Width && _position.GetHeight() == fix_size.Height)
        return;
    _position = RectWH(_position.Left, _position.Top, fix_size.Width, fix_size.Height);
    AdjustTransformation();
    _hasChangedSize = true;
}

void Viewport::SetAt(int x, int y)
{
    if (_position.Left == x && _position.Top == y)
        return;
    _position.MoveTo(Point(x, y));
    AdjustTransformation();
    _hasChangedPosition = true;
}

void Viewport::SetVisible(bool on)
{
    _visible = on;
    _hasChangedVisible = true;
}

void Viewport::SetZOrder(int zorder)
{
    _zorder = zorder;
    _hasChangedVisible = true;
}

void Viewport::AdjustTransformation()
{
    auto locked_cam = _camera.lock();
    if (locked_cam)
    {
        auto cam_rc = locked_cam->GetRect();
        float scale_x = (float)_position.GetWidth() / (float)cam_rc.GetWidth();
        float scale_y = (float)_position.GetHeight() / (float)cam_rc.GetHeight();
        float rotate = locked_cam->GetRotation();

        glm::mat4 mat_v2c = glmex::make_transform2d(cam_rc.Left, cam_rc.Top,
            1.f / scale_x, 1.f / scale_y, -Math::DegreesToRadians(rotate), -0.5 * cam_rc.GetWidth(), -0.5 * cam_rc.GetHeight());
        _v2cTransform = glmex::translate(mat_v2c, -_position.Left, -_position.Top);

        glm::mat4 mat_c2v = glmex::translate(_position.Left, _position.Top);
        _c2vTransform = glmex::inv_transform2d(mat_c2v, -cam_rc.Left, -cam_rc.Top,
            scale_x, scale_y, Math::DegreesToRadians(rotate), 0.5 * cam_rc.GetWidth(), 0.5 * cam_rc.GetHeight());;
    }
}

PCamera Viewport::GetCamera() const
{
    return _camera.lock();
}

void Viewport::LinkCamera(PCamera cam)
{
    _camera = cam;
    AdjustTransformation();
}

VpPoint Viewport::RoomToScreen(int roomx, int roomy, bool clip) const
{
    auto cam = _camera.lock();
    if (!cam)
        return std::make_pair(Point(), -1);
    glm::vec4 v = _c2vTransform * glmex::vec4(roomx, roomy);
    Point screen_pt(v.x, v.y);
    if (clip && !_position.IsInside(screen_pt))
        return std::make_pair(Point(), -1);
    return std::make_pair(screen_pt, _id);
}

VpPoint Viewport::ScreenToRoom(int scrx, int scry, bool clip) const
{
    Point screen_pt(scrx, scry);
    if (clip && !_position.IsInside(screen_pt))
        return std::make_pair(Point(), -1);
    auto cam = _camera.lock();
    if (!cam)
        return std::make_pair(Point(), -1);

    glm::vec4 v = _v2cTransform * glmex::vec4(scrx, scry);
    Point p(v.x, v.y);
    return std::make_pair(p, _id);
}
