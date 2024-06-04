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
#ifndef __AGS_EN_AC__ROUTEFINDER_IMPL_H
#define __AGS_EN_AC__ROUTEFINDER_IMPL_H

#include "ac/movelist.h"
#include "ac/route_finder.h"
#include "util/geometry.h"

namespace AGS
{
namespace Engine
{

class Navigation;

// JPSRouteFinder: a jump point search (JPS) A* pathfinder by Martin Sedlak.
class JPSRouteFinder : public MaskRouteFinder
{
public:
    JPSRouteFinder();
    ~JPSRouteFinder();

    // Traces a straight line between two points, returns if it's fully passable;
    // optionally assigns last found passable position.
    bool CanSeeFrom(int srcx, int srcy, int dstx, int dsty, int *lastcx = nullptr, int *lastcy = nullptr) override;
    // Search for a route between (srcx,y) and (destx,y), and returns a vector of Points.
    // exact_dest - tells to fail if the destination is inside the wall and cannot be reached;
    //              otherwise pathfinder will try to find the closest possible end point.
    // ignore_walls - tells to ignore impassable areas (builds a straight line path).
    bool FindRoute(std::vector<Point> &nav_path, int srcx, int srcy, int dstx, int dsty,
        bool exact_dest = false, bool ignore_walls = false) override;
    // Assign a walkable mask;
    // Note that this may make routefinder to generate additional data, taking more time.
    void SetWalkableArea(const AGS::Common::Bitmap *walkablearea) override;

private:
    void SyncNavWalkablearea();
    bool FindRouteJPS(std::vector<Point> &nav_path, int fromx, int fromy, int destx, int desty);

    const size_t MAXNAVPOINTS = MAXNEEDSTAGES;
    Navigation &nav; // declare as reference, because we must hide real Navigation decl here
    const Bitmap *walkablearea = nullptr;
    std::vector<int> path, cpath;
};

} // namespace Engine
} // namespace AGS

#endif // __AGS_EN_AC__ROUTEFINDER_IMPL_H