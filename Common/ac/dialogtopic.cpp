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

void DialogTopic::ReadFromFile_v363(Stream *in)
{
    // Dialog topic settings
    ScriptName = StrUtil::ReadString(in);
    Flags = in->ReadInt32();
    in->ReadInt32(); // reserved
    in->ReadInt32();
    in->ReadInt32();

    // Options
    uint32_t option_count = in->ReadInt32();
    Options.resize(option_count);
    for (auto &opt : Options)
    {
        opt.Name = StrUtil::ReadString(in);
        opt.Flags = in->ReadInt32();
        in->ReadInt32(); // reserved
        in->ReadInt32();
        in->ReadInt32();
    }
}

void DialogTopic::ReadOptionFromSavegame(DialogOption &opt, Stream *in, DialogTopicSvgVersion svg_ver)
{
    opt.Flags = in->ReadInt32();
}

void DialogTopic::ReadFromSavegame(Common::Stream *in, DialogTopicSvgVersion svg_ver, uint32_t *read_opt_count)
{
    // FIXME: assert count
    if (svg_ver <= kDialogTopicSvgVer_Initial)
    {
        uint32_t opt_idx = 0;
        for (; opt_idx < LEGACY_MAXTOPICOPTIONS && opt_idx < Options.size(); ++opt_idx)
            Options[opt_idx].Flags = in->ReadInt32();
        // skip if there's any mismatched count (?)
        for (; opt_idx < LEGACY_MAXTOPICOPTIONS; ++opt_idx)
            in->ReadInt32();

        if (read_opt_count)
            *read_opt_count = std::min<uint32_t>(Options.size(), LEGACY_MAXTOPICOPTIONS);
    }
    else
    {
        uint32_t option_records = in->ReadInt32();
        uint32_t opt_idx = 0;
        for (opt_idx = 0; opt_idx < option_records && opt_idx < Options.size(); ++opt_idx)
            ReadOptionFromSavegame(Options[opt_idx], in, svg_ver);
        // skip if there's any mismatched count (?)
        DialogOption dummy;
        for (; opt_idx < option_records; ++opt_idx)
            ReadOptionFromSavegame(dummy, in, svg_ver);

        if (read_opt_count)
            *read_opt_count = option_records;
    }
}

void DialogTopic::WriteToSavegame(Common::Stream *out) const
{
    out->WriteInt32(Options.size());
    for (auto &opt : Options)
        out->WriteInt32(opt.Flags);
}
