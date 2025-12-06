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
#include "ac/dialogtopic.h"
#include "util/stream.h"
#include "util/string_utils.h"

#define LEGACY_MAXTOPICOPTIONS 30

using namespace AGS::Common;

void DialogTopic::ReadFromFile_v321(Stream *in)
{
    std::vector<DialogOption> options;
    options.resize(LEGACY_MAXTOPICOPTIONS);
    char name[150];
    for (size_t i = 0u; i < LEGACY_MAXTOPICOPTIONS; ++i)
    {
        in->Read(name, 150);
        options[i].Name.SetString(name, 150);
    }
    for (size_t i = 0u; i < LEGACY_MAXTOPICOPTIONS; ++i)
    {
        options[i].Flags = in->ReadInt32();
    }
    in->ReadInt32(); // unused, was optionscripts 32-bit pointer
    for (size_t i = 0u; i < LEGACY_MAXTOPICOPTIONS; ++i)
    {
        options[i].EntryPoint = in->ReadInt16();
    }
    StartEntryPoint = in->ReadInt16();
    CodeSize = in->ReadInt16();
    uint32_t option_count = in->ReadInt32();
    Flags = in->ReadInt32();

    option_count = std::min(option_count, options.size());
    Options.resize(option_count);
    std::copy_n(options.begin(), option_count, Options.begin());
}

void DialogTopic::ReadFromSavegame(Common::Stream *in, int cmp_ver)
{
    // FIXME: assert count
    uint32_t opt_idx = 0;
    for (; opt_idx < LEGACY_MAXTOPICOPTIONS && opt_idx < Options.size(); ++opt_idx)
        Options[opt_idx].Flags = in->ReadInt32();
    // skip if there's any mismatched count (?)
    for (; opt_idx < LEGACY_MAXTOPICOPTIONS; ++opt_idx)
        in->ReadInt32();
}

void DialogTopic::WriteToSavegame(Common::Stream *out) const
{
    uint32_t opt_idx = 0;
    for (; opt_idx < LEGACY_MAXTOPICOPTIONS && opt_idx < Options.size(); ++opt_idx)
        out->WriteInt32(Options[opt_idx].Flags);
    for (; opt_idx < LEGACY_MAXTOPICOPTIONS; ++opt_idx)
        out->WriteInt32(0);
}
