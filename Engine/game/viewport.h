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

#include "util/geometry.h"
#include "util/scaling.h"

struct RoomCamera
{
    // Actual position and orthographic size
    Rect Position;
    // Automatic scaling used to resize the camera's picture to the viewport
    float ScaleX;
    float ScaleY;
    // Locked or following player automatically
    bool Locked;
};

struct Viewport
{
    Rect Position;
    // TODO: Camera reference (when supporting multiple cameras)
    // Coordinate tranform between camera and viewport
    // TODO: need to add rotate conversion to let script API support that
    AGS::Engine::PlaneScaling Transform;
};

#endif // __AGS_EE_AC__VIEWPORT_H
