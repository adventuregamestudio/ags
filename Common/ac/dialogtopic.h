//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// A Dialog Topic (a set of conversation options).
//
//=============================================================================
#ifndef __AGS_CN_AC__DIALOGTOPIC_H
#define __AGS_CN_AC__DIALOGTOPIC_H

#include "util/stream.h"
#include "util/string.h"

namespace AGS { namespace Common { class Stream; } }
using namespace AGS; // FIXME later

// Dialog Topic flags
#define DTFLG_SHOWPARSER    0x0001  // show parser in this topic

// Dialog Options flags
#define DFLG_ON             0x0001  // currently enabled
#define DFLG_OFFPERM        0x0002  // off forever (can't be trurned on)
#define DFLG_NOREPEAT       0x0004  // character doesn't say it when clicked
#define DFLG_HASBEENCHOSEN  0x0008  // dialog option is 'read'

// Old-style dialog script commands and keywords
#define DCHAR_NARRATOR      999
#define DCHAR_PLAYER        998

#define LEGACY_MAXTOPICOPTIONS 30

// RoomStatus runtime save format
// TODO: move to the engine code
enum DialogTopicSvgVersion
{
    kDialogTopicSvgVer_Initial  = 0,
    kDialogTopicSvgVer_363      = 3060304, // new save format
    kDialogTopicSvgVer_40008    = 4000008, // custom properties
    kDialogTopicSvgVer_40026    = 4000026, // sync with kDialogTopicSvgVer_363
};

struct DialogTopic
{
    struct DialogOption
    {
        Common::String Text;
        int Flags = 0;
        int EntryPoint = -1; // old-style dialog script entry point
    };

    Common::String ScriptName;
    int           Flags = 0; // DialogTopic flags (DTFLG_*)
    std::vector<DialogOption> Options;
    int           StartEntryPoint = -1; // old-style dialog script entry point
    size_t        CodeSize = 0u; // old-style dialog script size

    uint32_t GetOptionCount() const { return Options.size(); }

    void ReadFromFile_v321(Common::Stream *in);
    void ReadFromFile_v363(Common::Stream *in);

    void ReadFromSavegame(Common::Stream *in, DialogTopicSvgVersion svg_ver, uint32_t *read_opt_count);
    void WriteToSavegame(Common::Stream *out) const;

private:
    void ReadOptionFromSavegame(DialogOption &opt, Common::Stream *in, DialogTopicSvgVersion svg_ver);
};

#endif // __AGS_CN_AC__DIALOGTOPIC_H
