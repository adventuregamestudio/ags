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
#include "ac/route_finder.h"
#include "ac/route_finder_impl.h"
#include "gfx/bitmap.h"
#include "util/memory_compat.h"

namespace AGS
{
namespace Engine
{

bool MaskRouteFinder::CanSeeFrom(int srcx, int srcy, int dstx, int dsty, int *lastcx, int *lastcy)
{
    if (!_walkablearea)
        return false;

    // convert input to the mask coords
    srcx /= _coordScale;
    srcy /= _coordScale;
    dstx /= _coordScale;
    dsty /= _coordScale;

    int last_valid_x, last_valid_y;
    bool result = CanSeeFromImpl(srcx, srcy, dstx, dsty, &last_valid_x, &last_valid_y);

    // convert output from the mask coords
    if (lastcx)
        *lastcx = last_valid_x * _coordScale;
    if (lastcy)
        *lastcy = last_valid_y * _coordScale;
    return result;
}

bool MaskRouteFinder::FindRoute(std::vector<Point> &path, int srcx, int srcy, int dstx, int dsty,
    bool exact_dest, bool ignore_walls)
{
    if (!_walkablearea)
        return false;

    // convert input to the mask coords
    srcx /= _coordScale;
    srcy /= _coordScale;
    dstx /= _coordScale;
    dsty /= _coordScale;

    if (!FindRouteImpl(path, srcx, srcy, dstx, dsty, exact_dest, ignore_walls))
        return false;
    
    // convert output from the mask coords
    for (auto &pt : path)
        pt *= _coordScale;
    return true;
}

bool MaskRouteFinder::IsWalkableAt(int x, int y)
{
    if (!_walkablearea)
        return false;

    return _walkablearea->GetPixel(x / _coordScale, y / _coordScale) > 0;
}

bool MaskRouteFinder::FindNearestWalkablePoint(const Point &from_pt, Point &dst_pt)
{
    if (!_walkablearea)
        return false;

    bool result = Pathfinding::FindNearestWalkablePoint(_walkablearea, from_pt / _coordScale, dst_pt);
    dst_pt *= _coordScale;
    return result;
}

void MaskRouteFinder::SetWalkableArea(const Bitmap *walkablearea, int coord_scale)
{
    _walkablearea = walkablearea;
    assert(coord_scale > 0);
    _coordScale = std::max(1, coord_scale);
    OnSetWalkableArea();
}


namespace Pathfinding
{

using namespace MoveListOp;

std::unique_ptr<MaskRouteFinder> CreateDefaultMaskPathfinder()
{
    return std::make_unique<JPSRouteFinder>();
}

// Find route using a provided IRouteFinder, and calculate the MoveList using move speeds
bool FindRoute(MoveList &mls, IRouteFinder *finder, int srcx, int srcy, int dstx, int dsty,
    float move_speed_x, float move_speed_y, bool exact_dest, bool ignore_walls,
    const RunPathParams &run_params)
{
    std::vector<Point> path;
    if (!finder->FindRoute(path, srcx, srcy, dstx, dsty, exact_dest, ignore_walls))
        return false;

    return CalculateMoveList(mls, path, move_speed_x, move_speed_y, ignore_walls ? kMoveStage_Direct : 0, run_params);
}

bool FindNearestWalkablePoint(const Bitmap *mask, const Point &from_pt, Point &dst_pt,
    const int range, const int step, const bool use_method362)
{
    return FindNearestWalkablePoint(mask, from_pt, dst_pt, RectWH(mask->GetSize()), range, step, use_method362);
}

bool FindNearestWalkablePoint(const Bitmap *mask, const Point &from_pt, Point &dst_pt,
    const Rect &limits, const int range, const int step, const bool use_method362)
{
    assert(mask->GetColorDepth() == 8);
    const Rect mask_limits = IntersectRects(limits, RectWH(mask->GetSize()));
    const Rect use_limits = (range <= 0) ? mask_limits :
        IntersectRects(mask_limits, RectWH(from_pt.X - range / 2, from_pt.Y - range / 2, range * 2, range * 2));

    if (use_limits.IsEmpty())
        return false;

    // Quickly test if starting point is already on a walkable pixel
    if (use_limits.IsInside(from_pt) && mask->GetPixel(from_pt.X, from_pt.Y) > 0)
    {
        dst_pt = from_pt;
        return true;
    }

    uint64_t nearest_sqdist = UINT64_MAX;
    Point nearest_pt(-1, -1);
    const int mask_line_len = mask->GetPitch();
    const uint8_t *mask_ptr = mask->GetData();

    // Scan outwards from the starting point, rectangle by rectangle,
    // and measure distance between start and found point. The one with the smallest
    // distance will be accepted as a result.
    // As soon as we find the first walkable point, we limit the max scan range
    // to its distance, because we won't need anything beyond that.
    // We store distances as "squared distance" in order to avoid sqrt() call in a loop
    if (use_method362)
    {
        int max_range = std::max(
            std::max(from_pt.X - use_limits.Left, use_limits.Right - from_pt.X),
            std::max(from_pt.Y - use_limits.Top, use_limits.Bottom - from_pt.Y));
        for (int cur_range = 1; cur_range <= max_range; cur_range += step)
        {
            const int scan_fromx = std::max(use_limits.Left,   from_pt.X - cur_range);
            const int scan_tox   = std::min(use_limits.Right,  from_pt.X + cur_range);
            const int scan_fromy = std::max(use_limits.Top,    from_pt.Y - cur_range);
            const int scan_toy   = std::min(use_limits.Bottom, from_pt.Y + cur_range);
            // X outer and Y internal loop was a historical order of search,
            // kept for backwards compatibility (this is not too important, but just in case)
            for (int x = scan_fromx; x <= scan_tox; x += step)
            {
                const int y_step = (x == scan_fromx || x == scan_tox) ? step : cur_range * 2;
                for (int y = scan_fromy; y <= scan_toy; y += y_step)
                {
                    if (mask_ptr[y * mask_line_len + x] == 0)
                        continue;

                    uint64_t sqdist = (x - from_pt.X) * (x - from_pt.X) + (y - from_pt.Y) * (y - from_pt.Y);
                    if (sqdist < nearest_sqdist)
                    {
                        max_range = std::sqrt(sqdist);
                        nearest_sqdist = sqdist;
                        nearest_pt = Point(x, y);
                    }
                }
            }
        }
    }
    // Old-style scanning, kept for matching character position snap in pre-3.6.2 games.
    // This scans a rectangle centered on the given coordinates, starting from top-left
    // and going column by column (rather than row by row).
    else
    {
        for (int x = use_limits.Left; x <= use_limits.Right; x += step)
        {
            for (int y = use_limits.Top; y <= use_limits.Bottom; y += step)
            {
                if (mask_ptr[y * mask_line_len + x] == 0)
                    continue;

                uint64_t sqdist = (x - from_pt.X) * (x - from_pt.X) + (y - from_pt.Y) * (y - from_pt.Y);
                if (sqdist < nearest_sqdist)
                {
                    nearest_sqdist = sqdist;
                    nearest_pt = Point(x, y);
                }
            }
        }
    }

    if (nearest_sqdist < UINT64_MAX)
    {
        dst_pt = nearest_pt;
        return true;
    }
    return false;
}

} // namespace Pathfinding

} // namespace Engine
} // namespace AGS
