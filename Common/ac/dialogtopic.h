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
// Conversation dialog struct.
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

#define LEGACY_MAXTOPICOPTIONS 30

// Dialog Options flags
#define DFLG_ON             0x00000001  // currently enabled
#define DFLG_OFFPERM        0x00000002  // off forever (can't be trurned on)
#define DFLG_NOREPEAT       0x00000004  // character doesn't say it when clicked
#define DFLG_HASBEENCHOSEN  0x00000008  // dialog option is 'read'
#define DFLG_TEXTSET        0x00000010  // option's text modified at runtime

// Old-style dialog script commands and keywords
#define DCMD_SAY            1
#define DCMD_OPTOFF         2
#define DCMD_OPTON          3
#define DCMD_RETURN         4
#define DCMD_STOPDIALOG     5
#define DCMD_OPTOFFFOREVER  6
#define DCMD_RUNTEXTSCRIPT  7
#define DCMD_GOTODIALOG     8
#define DCMD_PLAYSOUND      9
#define DCMD_ADDINV         10
#define DCMD_SETSPCHVIEW    11
#define DCMD_NEWROOM        12
#define DCMD_SETGLOBALINT   13
#define DCMD_GIVESCORE      14
#define DCMD_GOTOPREVIOUS   15
#define DCMD_LOSEINV        16
#define DCMD_ENDSCRIPT      0xff
#define DCHAR_NARRATOR      999
#define DCHAR_PLAYER        998

enum DialogTopicSvgVersion
{
    kDialogTopicSvgVer_Initial  = 0,
    kDialogTopicSvgVer_363      = 3060304
};

struct DialogTopic
{
    struct DialogOption
    {
        // Storing original text in case we load a save without modified text.
        // NOTE: had no better idea how to prevent unnecessary text writing in the saves at this time.
        Common::String OriginalText;
        Common::String Text;
        int Flags = 0; // DFLG_* flags
        int EntryPoint = -1; // old-style dialog script entry point
    };

    Common::String ScriptName;
    int           Flags = 0; // DialogTopic flags (DTFLG_*)
    std::vector<DialogOption> Options;
    int           StartEntryPoint = -1; // old-style dialog script entry point
    size_t        CodeSize = 0u; // old-style dialog script size
    // NOTE: optionscripts is an unknown data from before AGS 2.5
#ifdef OBSOLETE
    std::vector<uint8_t> optionscripts;
#endif

    uint32_t GetOptionCount() const { return Options.size(); }

    void ReadFromFile_v321(Common::Stream *in);
    void ReadFromFile_v363(Common::Stream *in);

    void ReadFromSavegame(Common::Stream *in, DialogTopicSvgVersion svg_ver, uint32_t *read_opt_count);
    void WriteToSavegame(Common::Stream *out) const;

private:
    void ReadOptionFromSavegame(DialogOption &opt, Common::Stream *in, DialogTopicSvgVersion svg_ver);
};


#endif // __AGS_CN_AC__DIALOGTOPIC_H