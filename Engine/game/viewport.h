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
// Definition for the game viewports and cameras.
//
//=============================================================================
#ifndef __AGS_EE_AC__VIEWPORT_H
#define __AGS_EE_AC__VIEWPORT_H

#include <memory>
#include <vector>
#include "util/geometry.h"
#include "util/scaling.h"

class Camera;
class Viewport;

typedef std::shared_ptr<Camera> PCamera;
typedef std::shared_ptr<Viewport> PViewport;
typedef std::weak_ptr<Camera> CameraRef;
typedef std::weak_ptr<Viewport> ViewportRef;

// TODO: move to utility header
// From https://stackoverflow.com/questions/45507041/how-to-check-if-weak-ptr-is-empty-non-assigned
// Tests that weak_ptr is empty (was not initialized with valid reference)
template <typename T>
bool is_uninitialized(std::weak_ptr<T> const& weak) {
    using wt = std::weak_ptr<T>;
    return !weak.owner_before(wt{}) && !wt{}.owner_before(weak);
}


// Camera defines a "looking eye" inside the room, its position and size.
// It does not render anywhere on its own, instead it is linked to a viewport
// and latter draws what that camera sees.
// One camera may be linked to multiple viewports.
// Camera does not move on its own, this is performed by separate behavior
// algorithm. But it provides "lock" property that tells if its position is
// currently owned by user script.
class Camera
{
public:
    // Gets camera ID (serves as an index)
    inline int GetID() const { return _id; }
    // Sets new camera ID
    void SetID(int id);
    // Returns Room camera position and size inside the room (in room coordinates)
    const Rect &GetRect() const;
    // Sets explicit room camera's orthographic size
    void SetSize(const Size &cam_size);
    // Puts room camera to the new location in the room
    void SetAt(int x, int y);
    // Tells if camera is currently locked at custom position
    bool IsLocked() const;
    // Locks room camera at its current position
    void Lock();
    // Similar to SetAt, but also locks camera preventing it from following player character
    void LockAt(int x, int y);
    // Releases camera lock, letting it follow player character
    void Release();

    // Link this camera to a new viewport; this does not unlink any linked ones
    void LinkToViewport(ViewportRef viewport);
    // Unlinks this camera from a given viewport; does nothing if link did not exist
    void UnlinkFromViewport(int id);
    // Get the array of linked viewport references
    const std::vector<ViewportRef> &GetLinkedViewports() const;

    // Tells if this camera has changed recently
    inline bool HasChanged() const { return _hasChanged; }
    // Clears the changed flag
    inline void ClearChangedFlag() { _hasChanged = false; }

private:
    int _id = -1;
    // Actual position and orthographic size
    Rect _position;
    // Locked or following player automatically
    bool _locked = false;
    // Linked viewport refs, used to notify viewports of camera changes
    std::vector<ViewportRef> _viewportRefs;
    // Flag that tells whether this camera has changed recently
    bool _hasChanged = false;;
};


// Viewport class defines a rectangular area on game screen where the contents
// of a Camera are rendered.
// Viewport may have one linked camera at a time.
class Viewport
{
public:
    // Gets viewport ID (serves as an index)
    inline int GetID() const { return _id; }
    // Sets new viewport ID
    void SetID(int id);
    // Returns viewport's position on screen
    inline const Rect &GetRect() const { return _position; }
    // Returns viewport's room-to-screen transformation
    inline const AGS::Engine::PlaneScaling &GetTransform() const { return _transform; }
    // Set viewport's position on screen
    void SetRect(const Rect &rc);

    // Tells whether viewport content is rendered on screen
    bool IsVisible() const { return _visible; }
    // Changes viewport visibility
    void SetVisible(bool on);

    // Calculates room-to-viewport coordinate conversion.
    void AdjustTransformation();
    // Returns linked camera
    PCamera GetCamera() const;
    // Links new camera to this viewport, overriding existing link;
    // pass nullptr to leave viewport without a camera link
    void LinkCamera(PCamera cam);

    // Tells if this viewport has changed recently
    inline bool HasChanged() const { return _hasChanged; }
    // Clears the changed flag
    inline void ClearChangedFlag() { _hasChanged = false; }

private:
    int _id = -1;
    // Position of the viewport on screen
    Rect _position;
    // TODO: Camera reference (when supporting multiple cameras)
    // Coordinate tranform between camera and viewport
    // TODO: need to add rotate conversion to let script API support that;
    // (maybe use full 3D matrix for that)
    AGS::Engine::PlaneScaling _transform;
    // Linked camera reference
    CameraRef _camera;
    bool _visible = true;
    // Flag that tells whether this viewport has changed recently
    bool _hasChanged = false;;
};

#endif // __AGS_EE_AC__VIEWPORT_H
