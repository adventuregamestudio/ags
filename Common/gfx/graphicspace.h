//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// GraphicSpace class defines an arbitrary object's position in 2D space.
//
// TODO: it seems like this class should be a part of the runtime engine,
// and not Common library. The only reason it's in Common atm is because
// its used in GUI classes. But they in turn should be separated into
// "data" structs and runtime classes.
// 
//=============================================================================
#ifndef __AGS_CN_GFX__GRAPHICSPACE_H
#define __AGS_CN_GFX__GRAPHICSPACE_H

#include <algorithm>
#include <vector>
#include "util/geometry.h"
#include "util/matrix.h"

namespace AGS
{
namespace Common
{

// GraphicSpace provides information about object's graphic location and basic shape on screen;
// this may be used for positioning when drawing, or e.g. hit and collision detection.
// GraphicSpace's 0,0 local coordinates do not correspond to the object's ORIGIN,
// but to the object's sprite's left-top corner.
//
// FIXME: there's an annoying inconsistency, where GS requires "origin" to be the pure
// object position without "graphical offsets", because it's used for interaction hit-tests,
// while IGraphicsDriver::DrawSprite (and other related helper functions) require "origin"
// to include graphical offsets (and scaled too). Revise this and find a way to keep
// all things clear and together in GraphicSpace, avoid separate graphical origin recalculations
// elsewhere and only read prepared GS members.
class GraphicSpace
{
public:
    GraphicSpace() {}
    // ox,oy    - position of the object's origin in world coordinates
    // origin   - object's origin, a relation of sprite to the object's pos [0;1]
    // src_size - source sprite size in local object's coordinates
    // dst_size - final scale of the source size
    // g_aabb   - graphical rectangle in local object's coordinates,
    //          defined as *relative* to the source sprite position;
    //          this is used to separate additional offsets for the object gfx,
    //          in case its image exceeds the "logical" rectangle.
    // rot      - rotation, clockwise, in degrees
    // pivot    - pivot of rotation, in *relative* position (center is default)
    // pivot_off - extra pivot offset, in local coordinates (zero is default)
    GraphicSpace(const int ox, const int oy, const Pointf &origin, const Size &src_size,
        const Size &dst_size, const Rect &g_aabb, const float rot,
        const Pointf pivot = Pointf(.5f, .5f), const Point pivot_off = Point())
    {
        const int src_w = src_size.Width;
        const int src_h = src_size.Height;
        const float sx = src_w != 0.f ? static_cast<float>(dst_size.Width) / src_w : 1.f;
        const float sy = src_h != 0.f ? static_cast<float>(dst_size.Height) / src_h : 1.f;
        Init(ox, oy, origin, src_size, dst_size, g_aabb, sx, sy, rot, pivot, pivot_off);
    }

    // ox,oy    - position of the object's origin in world coordinates
    // origin   - object's origin, a relation of sprite to the object's pos [0;1]
    // src_size - source sprite size in local object's coordinates
    // g_aabb   - graphical rectangle in local object's coordinates;
    //          this is used to separate additional offsets for the object gfx,
    //          in case its image exceeds the "logical" rectangle.
    // sx,sy    - scaling factors (along x and y axes)
    // rot      - rotation, clockwise, in degrees
    // pivot    - pivot of rotation, in *relative* position (center is default)
    // pivot_off - extra pivot offset, in local coordinates (zero is default)
    GraphicSpace(const int ox, const int oy, const Pointf &origin, const Size &src_size,
        const Rect &g_aabb, const float sx, const float sy, const float rot,
        const Pointf pivot = Pointf(.5f, .5f), const Point pivot_off = Point())
    {
        Init(ox, oy, origin, src_size,
            Size(static_cast<int>(src_size.Width * sx), static_cast<int>(src_size.Height * sy)),
            g_aabb, sx, sy, rot, pivot, pivot_off);
    }

    // Get position of the top-left corner of this object in the world coordinates;
    // useful when you need to know a position of already transformed sprites.
    inline Point TopLeft() const { return _AABB.GetLT(); }
    // Get axis-aligned bounding box, in the world coordinates
    inline const Rect &AABB() const { return _AABB; }

    // Converts world coordinate into local object space
    inline Point WorldToLocal(int x, int y) const
    {
        glm::vec4 v = W2LTransform * glmex::vec4(static_cast<float>(x), static_cast<float>(y));
        return Point(static_cast<int>(v.x), static_cast<int>(v.y)); // TODO: better rounding
    }

    // Converts local object coordinates into world space
    inline Point LocalToWorld(int x, int y) const
    {
        glm::vec4 v = L2WTransform * glmex::vec4(static_cast<float>(x), static_cast<float>(y));
        return Point(static_cast<int>(v.x), static_cast<int>(v.y)); // TODO: better rounding
    }

    // Fills a std::vector with 4 corner positions of AABB, in the clockwise order
    inline void GetAABBPoints(std::vector<Point> &points) const
    {
        points.resize(4);
        points[0] = _AABB.GetLT();
        points[1] = _AABB.GetRT();
        points[2] = _AABB.GetRB();
        points[3] = _AABB.GetLB();
    }

    // Fills a std::vector with 4 corner positions of the transformed object,
    // in world coordinates, in the clockwise order.
    // NOTE: GraphicSpace does not store object size, only transform, so we have to pass size as a argument
    inline void GetTransformedCorners(std::vector<Point> &points, const Size &obj_size) const
    {
        points.resize(4);
        points[0] = LocalToWorld(0, 0);
        points[1] = LocalToWorld(obj_size.Width - 1, 0);
        points[2] = LocalToWorld(obj_size.Width - 1, obj_size.Height - 1);
        points[3] = LocalToWorld(0, obj_size.Height - 1);
    }

private:
    void Init(const int ox, const int oy, const Pointf &origin, const Size &src_size,
        const Size &dst_size, const Rect &g_aabb, const float sx, const float sy,
        const float rot, const Pointf pivot, const Point pivot_off)
    {
        const int local_srcx = -static_cast<int>((src_size.Width - 1) * origin.X);
        const int local_srcy = -static_cast<int>((src_size.Height - 1) * origin.Y);
        const float world_x = static_cast<float>(ox) - (std::abs(dst_size.Width) - 1) * origin.X;
        const float world_y = static_cast<float>(oy) - (std::abs(dst_size.Height) - 1) * origin.Y;
        const float sx_inv = std::fabs(sx) < std::numeric_limits<float>::epsilon()
            ? 0.f : 1.f / sx;
        const float sy_inv = std::fabs(sy) < std::numeric_limits<float>::epsilon()
            ? 0.f : 1.f / sy;
        // Pivot is relative to the local coordinate center;
        // here is calculated in coordinates of the *destination* rectangle (post-scaling)
        const float pivotx = (dst_size.Width) * pivot.X + pivot_off.X * sx;
        const float pivoty = (dst_size.Height) * pivot.Y + pivot_off.Y * sy;
        // World->local transform
        W2LTransform = glmex::make_inv_transform2d(
            -world_x, -world_y, sx_inv, sy_inv,
            static_cast<float>(-Math::DegreesToRadians(rot)), pivotx, pivoty);
        // Local->world transform + AABB
        L2WTransform = glmex::make_transform2d(
            world_x, world_y, sx, sy,
            static_cast<float>(Math::DegreesToRadians(rot)), -pivotx, -pivoty);
        // Make the graphical AABB (note: have to translate to the local coord center first)
        //const Rect local_g_aabb = Rect::MoveBy(g_aabb, -local_srcx, -local_srcy);
        _AABB = glmex::full_transform(g_aabb, L2WTransform);
    }

    glm::mat4 W2LTransform = glm::mat4(1.0); // transform from world to local space
    glm::mat4 L2WTransform = glm::mat4(1.0); // transform from local to world space
    Rect _AABB; // axis-aligned bounding box
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GFX__GRAPHICSPACE_H
