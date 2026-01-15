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

private:
    // Update the implementation after a new walkable area is set
    void OnSetWalkableArea() override;
    // CanSeeFrom implementation
    bool CanSeeFromImpl(int srcx, int srcy, int dstx, int dsty, int *lastcx = nullptr, int *lastcy = nullptr)  override;
    // FindRoute implementation
    bool FindRouteImpl(std::vector<Point> &path, int srcx, int srcy, int dstx, int dsty,
        bool exact_dest, bool ignore_walls)  override;

    void SyncNavWalkablearea();
    bool FindRouteJPS(std::vector<Point> &nav_path, int fromx, int fromy, int destx, int desty);

    Navigation &nav; // declare as reference, because we must hide real Navigation decl here
    std::vector<int> path, cpath;
};

} // namespace Engine
} // namespace AGS

#endif // __AGS_EN_AC__ROUTEFINDER_IMPL_H