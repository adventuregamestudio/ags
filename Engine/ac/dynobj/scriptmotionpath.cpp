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
#include "ac/dynobj/scriptmotionpath.h"
#include "ac/dynobj/dynobj_manager.h"
#include "ac/route_finder.h"

using namespace AGS::Common;
using namespace AGS::Engine;

ScriptMotionPath::ScriptMotionPath(MoveList *mlist, uint32_t mlist_id)
    : _moveList(mlist)
    , _moveListInternalID(mlist_id)
{
}

ScriptMotionPath::ScriptMotionPath(std::unique_ptr<MoveList> &&mlist)
    : _ownMoveList(std::move(mlist))
    , _moveList(_ownMoveList.get())
    , _moveListInternalID(UINT32_MAX)
{
}

const char *ScriptMotionPath::GetType()
{
    return "MotionPath";
}

int ScriptMotionPath::Dispose(void *address, bool force)
{
    delete this;
    return 1;
}

void ScriptMotionPath::Unserialize(int index, Stream *in, size_t data_sz)
{
    // TODO
    ccRegisterUnserializedObject(index, this, this);
}

size_t ScriptMotionPath::CalcSerializeSize(const void *address)
{
    // TODO
    return 0;
}

void ScriptMotionPath::Serialize(const void *address, Stream *out)
{
    // TODO
}

ScriptMotionPath *ScriptMotionPath::Create(const std::vector<Point> &path,
    float speedx, float speedy, const RunPathParams &run_params)
{
    std::unique_ptr<MoveList> mlist(new MoveList());
    // TODO: support input speeds as floats
    Pathfinding::CalculateMoveList(*mlist, path, speedx, speedy, 0, run_params);

    ScriptMotionPath *motion_path = new ScriptMotionPath(std::move(mlist));
    ccRegisterManagedObject(motion_path, motion_path);
    return motion_path;
}

ScriptMotionPath *ScriptMotionPath::Create(const std::vector<Point> &path,
    const std::vector<float> &speedxs, const std::vector<float> &speedys, const RunPathParams &run_params)
{
    // TODO
    return nullptr;
}
