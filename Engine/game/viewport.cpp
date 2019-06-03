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
void Camera::SetSize(const Size &cam_size)
{
    // TODO: currently we don't support having camera larger than room background
    // (or rather - looking outside of the room background); look into this later
    const Size real_room_sz = Size(thisroom.Width, thisroom.Height);
    Size real_size = Size::Clamp(cam_size, Size(1, 1), real_room_sz);

    _position.SetWidth(real_size.Width);
    _position.SetHeight(real_size.Height);
    for (auto vp = _viewportRefs.begin(); vp != _viewportRefs.end(); ++vp)
    {
        auto locked_vp = vp->lock();
        if (locked_vp)
            locked_vp->AdjustTransformation();
    }
    _hasChanged = true;
}

// Puts room camera to the new location in the room
void Camera::SetAt(int x, int y)
{
    int cw = _position.GetWidth();
    int ch = _position.GetHeight();
    int room_width = thisroom.Width;
    int room_height = thisroom.Height;
    x = Math::Clamp(x, 0, room_width - cw);
    y = Math::Clamp(y, 0, room_height - ch);
    _position.MoveTo(Point(x, y));
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
    _position = rc;
    AdjustTransformation();
    _hasChanged = true;
}

void Viewport::SetVisible(bool on)
{
    _visible = on;
    _hasChanged = true;
}

void Viewport::SetZOrder(int zorder)
{
    _zorder = zorder;
    _hasChanged = true;
    play.InvalidateViewportZOrder();
}

void Viewport::AdjustTransformation()
{
    auto locked_cam = _camera.lock();
    if (locked_cam)
        _transform.Init(locked_cam->GetRect().GetSize(), _position);
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
