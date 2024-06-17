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
// New jump point search (JPS) A* pathfinder by Martin Sedlak.
//
//=============================================================================
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

void JPSRouteFinder::Configure(GameDataVersion /*game_ver*/)
{
}

void JPSRouteFinder::SetWalkableArea(const Bitmap *walkablearea) 
{
    this->walkablearea = walkablearea;
}

void JPSRouteFinder::SyncNavWalkablearea()
{
    // FIXME: this is dumb, but...
    nav.Resize(walkablearea->GetWidth(), walkablearea->GetHeight());

    for (int y=0; y<walkablearea->GetHeight(); y++)
        nav.SetMapRow(y, walkablearea->GetScanLine(y));
}

bool JPSRouteFinder::CanSeeFrom(int srcx, int srcy, int dstx, int dsty, int *lastcx, int *lastcy)
{
    if (!walkablearea)
        return false;

    int last_valid_x = srcx, last_valid_y = srcy;
    bool result = true;
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
    SyncNavWalkablearea();

    path.clear();
    cpath.clear();

    if (nav.NavigateRefined(fromx, fromy, destx, desty, path, cpath) == Navigation::NAV_UNREACHABLE)
        return false;

    nav_path.clear();

    // new behavior: cut path if too complex rather than abort with error message
    int count = std::min<int>((int)cpath.size(), MAXNAVPOINTS);

    for (int i = 0; i<count; i++)
    {
        int x, y;
        nav.UnpackSquare(cpath[i], x, y);
        nav_path.emplace_back( x, y );
    }

    return true;
}

bool JPSRouteFinder::FindRoute(std::vector<Point> &nav_path, int srcx, int srcy, int dstx, int dsty,
    bool exact_dest, bool ignore_walls)
{
    if (!walkablearea)
        return false;

    nav_path.clear();

    if (ignore_walls || CanSeeFrom(srcx, srcy, dstx, dsty))
    {
        nav_path.emplace_back( srcx, srcy );
        nav_path.emplace_back( dstx, dsty );
    }
    else
    {
        if ((exact_dest) && (walkablearea->GetPixel(dstx, dsty) == 0))
            return false; // clicked on a wall

        FindRouteJPS(nav_path, srcx, srcy, dstx, dsty);
    }

    if (nav_path.empty())
        return false;

    // Ensure it has at least 2 points (start-end), necessary for the move algorithm
    if (nav_path.size() == 1)
        nav_path.push_back(nav_path[0]);

    assert(nav_path.size() <= MAXNAVPOINTS);

#ifdef DEBUG_PATHFINDER
    AGS::Common::Debug::Printf("Route from %d,%d to %d,%d - %zu stages", srcx,srcy,xx,yy,nav_path.size());
#endif
    return true;
}

} // namespace Engine
} // namespace AGS
