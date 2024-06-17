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
#ifndef __AC_ROUTE_FINDER_IMPL_LEGACY
#define __AC_ROUTE_FINDER_IMPL_LEGACY

#include "ac/route_finder.h"

// Forward declaration
namespace AGS { namespace Common { class Bitmap; }}
struct MoveList;

namespace AGS
{
namespace Engine
{

// LegacyRouteFinder: a flood-fill search pathfinder.
class LegacyRouteFinder : public MaskRouteFinder
{
public:
    LegacyRouteFinder();
    ~LegacyRouteFinder();

    void Configure(GameDataVersion game_ver) override;
    // Traces a straight line between two points, returns if it's fully passable;
    // optionally assigns last found passable position.
    bool CanSeeFrom(int srcx, int srcy, int dstx, int dsty, int *lastcx = nullptr, int *lastcy = nullptr) override;
    // Search for a route between (srcx,y) and (destx,y), and calculate the MoveList using given speeds.
    // exact_dest - tells to fail if the destination is inside the wall and cannot be reached;
    //              otherwise pathfinder will try to find the closest possible end point.
    // ignore_walls - tells to ignore impassable areas (builds a straight line path).
    bool FindRoute(std::vector<Point> &path, int srcx, int srcy, int dstx, int dsty,
        bool exact_dest = false, bool ignore_walls = false) override;
    // Assign a walkable mask;
    // Note that this may make routefinder to generate additional data, taking more time.
    void SetWalkableArea(const AGS::Common::Bitmap *walkablearea) override;

    // Configuration for the pathfinder
    struct PathfinderConfig
    {
        const int MaxGranularity = 3;

        // Short sweep is performed in certain radius around requested destination,
        // when searching for a nearest walkable area in the vicinity
        const int ShortSweepRadius = 50;
        int ShortSweepGranularity = 3; // variable, depending on loaded game version
        // Full sweep is performed over a whole walkable area
        const int FullSweepGranularity = 5;
    };

private:
    const AGS::Common::Bitmap *wallscreen = nullptr;
    PathfinderConfig _pfc;
};

} // namespace Engine
} // namespace AGS

#endif // __AC_ROUTE_FINDER_IMPL_LEGACY
