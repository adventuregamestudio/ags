//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// Jump point search (JPS) A* pathfinder implementation by Martin Sedlak.
//
//=============================================================================
#include <algorithm>
#include "ac/route_finder_impl.h"
#include "ac/route_finder_jps.inl"
#include "gfx/bitmap.h"

using namespace AGS::Common;

// #define DEBUG_PATHFINDER

namespace AGS
{
namespace Engine
{

JPSRouteFinder::JPSRouteFinder()
    : nav(*new Navigation())
{
}

JPSRouteFinder::~JPSRouteFinder()
{
    delete &nav;
}

void JPSRouteFinder::OnSetWalkableArea()
{
}

void JPSRouteFinder::SyncNavWalkablearea()
{
    // FIXME: this is dumb, but...
    nav.Resize(_walkablearea->GetWidth(), _walkablearea->GetHeight());

    for (int y = 0; y < _walkablearea->GetHeight(); y++)
        nav.SetMapRow(y, _walkablearea->GetScanLine(y));
}

bool JPSRouteFinder::CanSeeFromImpl(int srcx, int srcy, int dstx, int dsty, int *lastcx, int *lastcy)
{
    bool result = false;
    int last_valid_x = srcx, last_valid_y = srcy;
    if ((srcx != dstx) || (srcy != dsty))
    {
        SyncNavWalkablearea();
        result = !nav.TraceLine(srcx, srcy, dstx, dsty, last_valid_x, last_valid_y);
    }
    if (lastcx)
        *lastcx = last_valid_x;
    if (lastcy)
        *lastcy = last_valid_y;
    return result;
}

bool JPSRouteFinder::FindRouteJPS(std::vector<Point> &nav_path, int fromx, int fromy, int destx, int desty)
{
    if (!_walkablearea)
        return false;

    SyncNavWalkablearea();

    path.clear();
    cpath.clear();

    if (nav.NavigateRefined(fromx, fromy, destx, desty, path, cpath) == Navigation::NAV_UNREACHABLE)
        return false;

    nav_path.clear();

    for (int i = 0; i < cpath.size(); i++)
    {
        int x, y;
        nav.UnpackSquare(cpath[i], x, y);
        nav_path.emplace_back( x, y );
    }

    return true;
}

bool JPSRouteFinder::FindRouteImpl(std::vector<Point> &nav_path, int srcx, int srcy, int dstx, int dsty,
    bool exact_dest, bool ignore_walls)
{
    nav_path.clear();

    if (ignore_walls || CanSeeFromImpl(srcx, srcy, dstx, dsty))
    {
        nav_path.emplace_back( srcx, srcy );
        nav_path.emplace_back( dstx, dsty );
    }
    else
    {
        if ((exact_dest) && (_walkablearea->GetPixel(dstx, dsty) == 0))
            return false; // clicked on a wall

        FindRouteJPS(nav_path, srcx, srcy, dstx, dsty);
    }

    if (nav_path.empty())
        return false;

    // Ensure it has at least 2 points (start-end), necessary for the move algorithm
    if (nav_path.size() == 1)
        nav_path.push_back(nav_path[0]);

#ifdef DEBUG_PATHFINDER
    AGS::Common::Debug::Printf("Route from %d,%d to %d,%d - %d stages", srcx,srcy,xx,yy,(int)nav_path.size());
#endif
    return true;
}

} // namespace Engine
} // namespace AGS
