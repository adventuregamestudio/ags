//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#ifndef __AGS_EN_AC__MOVELIST_H
#define __AGS_EN_AC__MOVELIST_H

#include "ac/runtime_defines.h" // RunPathParams
#include "game/savegame.h"
#include "util/geometry.h"

// Forward declaration
namespace AGS { namespace Common { class Stream; } }
using namespace AGS; // FIXME later

#define LEGACY_MAXMOVESTAGES 256
enum MoveListStageFlags
{
    kMoveStage_Direct = 0x01 // ignoring walkable areas
};


enum MoveListSvgVersion
{
    kMoveSvgVersion_Initial = 0, // [UNSUPPORTED] from 3.5.0 pre-alpha
    kMoveSvgVersion_350     = 1, // new pathfinder, arbitrary number of stages
    kMoveSvgVersion_36109   = 2, // skip empty lists, progress as float
    kMoveSvgVersion_36208   = 3060208, // flags per stage
    kMoveSvgVersion_400     = 4000000, // fixed->floats, positions are int32
    kMoveSvgVersion_40006   = 4000006, // extra running params (repeat, dir)
    kMoveSvgVersion_40016   = 4000016, // sync with kMoveSvgVersion_36208
};

class MoveList
{
public:
    // TODO: protect all these fields!

    // Waypoints, per stage
    std::vector<Point> pos;
    // permove contain number of pixels done per a single step
    // along x and y axes; i.e. this is a movement vector, per path stage
    std::vector<Pointf> permove;
    // Flags per stage (see MoveListStageFlags)
    std::vector<uint8_t> stageflags;
    uint32_t onstage = 0; // current path stage
    Point   from; // current stage's starting position
    // Steps made during current stage;
    // distance passed is calculated as permove[onstage] * onpart;
    // made a fractional value to let recalculate movelist dynamically
    float   onpart = 0.f;
    uint8_t doneflag = 0u; // currently unused, but reserved
    RunPathParams run_params;

    bool IsEmpty() const { return pos.empty(); }
    uint32_t GetNumStages() const { return pos.size(); }
    const Point &GetLastPos() const { return pos.back(); }
    int GetCurrentStageFlags() const { return onstage < stageflags.size() ? stageflags[onstage] : 0; }
    bool IsStageDirect() const { return (GetCurrentStageFlags() & kMoveStage_Direct) != 0; }

    // Gets a movelist's step length, in coordinate units
    // (normally the coord unit is a game pixel)
    float GetStepLength() const;
    // Gets a fraction of a coordinate unit that is in progress of stepping over;
    // (normally the coord unit is a game pixel)
    float GetPixelUnitFraction() const;
    // Sets a step progress to this fraction of a coordinate unit
    void  SetPixelUnitFraction(float frac);

    // Set MoveList to the done state; note this does not free the path itself
    void Complete();
    // Reset MoveList to the beginning, account for RunPathParams
    void ResetToBegin();
    // Progress to the next stage, account for RunPathParams;
    // returns if there's a new stage available
    bool NextStage();

    AGS::Engine::HSaveError ReadFromSavegame(Common::Stream *in, int32_t cmp_ver);
    void WriteToSavegame(Common::Stream *out) const;

private:
    // Handle end of path, either stop or reset to beginning, as per RunPathParams
    bool OnPathCompleted();
};

#endif // __AGS_EN_AC__MOVELIST_H
