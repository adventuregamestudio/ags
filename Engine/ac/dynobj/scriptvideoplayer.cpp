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
#include "ac/dynobj/scriptvideoplayer.h"
#include "ac/dynobj/dynobj_manager.h"
#include "media/video/video.h"
#include "util/stream.h"

using namespace AGS::Common;

const char *ScriptVideoPlayer::GetType()
{
    return "VideoPlayer";
}

int ScriptVideoPlayer::Dispose(void* /*address*/, bool /*force*/)
{
    if (_id >= 0)
    {
        video_stop(_id);
    }

    delete this;
    return 1;
}

size_t ScriptVideoPlayer::CalcSerializeSize(const void* /*address*/)
{
    return sizeof(int32_t);
}

void ScriptVideoPlayer::Serialize(const void* /*address*/, Stream *out)
{
    out->WriteInt32(_id);
}

void ScriptVideoPlayer::Unserialize(int index, Stream *in, size_t /*data_sz*/)
{
    _id = in->ReadInt32();
    ccRegisterUnserializedObject(index, this, this);
}
