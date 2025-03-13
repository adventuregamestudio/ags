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
#include "ac/game.h"
#include "ac/route_finder.h"
#include "util/stream.h"

using namespace AGS::Common;
using namespace AGS::Engine;

ScriptMotionPath::ScriptMotionPath(uint32_t mlist_id)
    : _moveListID(mlist_id)
{
}

bool ScriptMotionPath::IsValid() const
{
    return _moveListID > 0;
}

MoveList *ScriptMotionPath::GetMoveList() const
{
    return _moveListID == 0 ?
        nullptr :
        get_movelist(_moveListID);
}

void ScriptMotionPath::Invalidate()
{
    _moveListID = 0u;
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
    _moveListID = in->ReadInt32();
    ccRegisterUnserializedObject(index, this, this);
}

size_t ScriptMotionPath::CalcSerializeSize(const void *address)
{
    return sizeof(uint32_t);
}

void ScriptMotionPath::Serialize(const void *address, Stream *out)
{
    out->WriteInt32(_moveListID);
}

DynObjectRef ScriptMotionPath::Create(uint32_t mlist_id)
{
    ScriptMotionPath *motion_path = new ScriptMotionPath(mlist_id);
    int handle = ccRegisterManagedObject(motion_path, motion_path);
    return DynObjectRef(handle, motion_path, motion_path);
}
