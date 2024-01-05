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
#include "ac/dynobj/scriptviewframe.h"
#include "ac/dynobj/dynobj_manager.h"
#include "util/stream.h"

using namespace AGS::Common;

int ScriptViewFrame::Dispose(void* /*address*/, bool /*force*/) {
    // always dispose a ViewFrame
    delete this;
    return 1;
}

const char *ScriptViewFrame::GetType() {
    return "ViewFrame";
}

size_t ScriptViewFrame::CalcSerializeSize(const void* /*address*/)
{
    return sizeof(int32_t) * 3;
}

void ScriptViewFrame::Serialize(const void* /*address*/, Stream *out) {
    out->WriteInt32(view);
    out->WriteInt32(loop);
    out->WriteInt32(frame);
}

void ScriptViewFrame::Unserialize(int index, Stream *in, size_t /*data_sz*/) {
    view = in->ReadInt32();
    loop = in->ReadInt32();
    frame = in->ReadInt32();
    ccRegisterUnserializedObject(index, this, this);
}

ScriptViewFrame::ScriptViewFrame(int p_view, int p_loop, int p_frame) {
    view = p_view;
    loop = p_loop;
    frame = p_frame;
}

ScriptViewFrame::ScriptViewFrame() {
    view = -1;
    loop = -1;
    frame = -1;
}
