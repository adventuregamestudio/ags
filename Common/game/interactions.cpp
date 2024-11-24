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
#include "game/interactions.h"
#include <algorithm>
#include <string.h>
#include "ac/common.h" // quit
#include "util/stream.h"
#include "util/string_utils.h"

namespace AGS
{
namespace Common
{

//-----------------------------------------------------------------------------
//
// InteractionEvents (modern interaction system).
//
//-----------------------------------------------------------------------------

std::unique_ptr<InteractionEvents> InteractionEvents::CreateFromStream_v361(Stream *in)
{
    std::unique_ptr<InteractionEvents> inter(new InteractionEvents());
    inter->Read_v361(in);
    return inter;
}

std::unique_ptr<InteractionEvents> InteractionEvents::CreateFromStream_v362(Stream *in)
{
    std::unique_ptr<InteractionEvents> inter(new InteractionEvents());
    inter->Read_v362(in);
    return inter;
}

void InteractionEvents::Read_v361(Stream *in)
{
    Events.clear();
    const size_t evt_count = in->ReadInt32();
    for (size_t i = 0; i < evt_count; ++i)
    {
        Events.push_back( { String::FromStream(in) } );
    }
}

HError InteractionEvents::Read_v362(Stream *in)
{
    Events.clear();
    InteractionEventsVersion ver = (InteractionEventsVersion)in->ReadInt32();
    if (ver != kInterEvents_v362)
        return new Error(String::FromFormat("InteractionEvents version not supported: %d", ver));

    ScriptModule = StrUtil::ReadString(in);
    const size_t evt_count = in->ReadInt32();
    for (size_t i = 0; i < evt_count; ++i)
    {
        Events.push_back( { StrUtil::ReadString(in) } );
    }
    return HError::None();
}

void InteractionEvents::Write_v361(Stream *out) const
{
    out->WriteInt32(Events.size());
    for (const auto &evt : Events)
    {
        evt.FunctionName.Write(out);
    }
}

void InteractionEvents::Write_v362(Stream *out) const
{
    out->WriteInt32(kInterEvents_v362);
    StrUtil::WriteString(ScriptModule, out);
    out->WriteInt32(Events.size());
    for (const auto &evt : Events)
    {
        StrUtil::WriteString(evt.FunctionName, out);
    }
}

} // namespace Common
} // namespace AGS
