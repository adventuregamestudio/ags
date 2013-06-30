//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================

#include "game/dialogtopicinfo.h"

namespace AGS
{
namespace Common
{

DialogTopicInfo::DialogTopicInfo()
{
    Flags = 0;
    OptionCount = 0;
    StartUpEntryPoint = 0;
    OldCodeSize = 0;
}

void DialogTopicInfo::ReadFromFile(Common::Stream *in, DialogVersion version)
{
    if (version == kDialogVersion_pre340)
    {
        Options.SetLength(LEGACY_MAX_DIALOG_TOPIC_OPTIONS);
        for (int i = 0; i < LEGACY_MAX_DIALOG_TOPIC_OPTIONS; ++i)
        {
            Options[i].Name.ReadCount(in, 150);
        }
        for (int i = 0; i < LEGACY_MAX_DIALOG_TOPIC_OPTIONS; ++i)
        {
            Options[i].Flags = in->ReadInt32();
        }
        in->ReadInt32(); // optionscripts
        for (int i = 0; i < LEGACY_MAX_DIALOG_TOPIC_OPTIONS; ++i)
        {
            Options[i].EntryPoint = in->ReadInt16();
        }
        StartUpEntryPoint = in->ReadInt16();
        OldCodeSize = in->ReadInt16();
        OptionCount = in->ReadInt32();
        Flags = in->ReadInt32();
        Options.SetLength(OptionCount);
    }
    else
    {
        Flags = in->ReadInt32();
        OptionCount = in->ReadInt32();
        Options.SetLength(OptionCount);
        for (int i = 0; i < OptionCount; ++i)
        {
            Options[i].Name.Read(in);
            Options[i].Flags = in->ReadInt32();
            Options[i].EntryPoint = in->ReadInt16();
        }
        StartUpEntryPoint = in->ReadInt16();
        OldCodeSize = in->ReadInt16();
    }
}

void DialogTopicInfo::WriteToFile(Common::Stream *out)
{
    out->WriteInt32(Flags);
    out->WriteInt32(OptionCount);
    for (int i = 0; i < OptionCount; ++i)
    {
        Options[i].Name.Write(out);
        out->WriteInt32(Options[i].Flags);
        out->WriteInt16(Options[i].EntryPoint);
    }
    out->WriteInt16(StartUpEntryPoint);
    out->WriteInt16(OldCodeSize);
}

} // namespace Common
} // namespace AGS
