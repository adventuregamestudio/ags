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
//
// A Dialog Topic (a set of conversation options).
//
//=============================================================================
#ifndef __AGS_CN_AC__DIALOGTOPIC_H
#define __AGS_CN_AC__DIALOGTOPIC_H

namespace AGS { namespace Common { class Stream; } }
using namespace AGS; // FIXME later

#define MAXTOPICOPTIONS     30
#define DFLG_ON             1  // currently enabled
#define DFLG_OFFPERM        2  // off forever (can't be trurned on)
#define DFLG_NOREPEAT       4  // character doesn't repeat it when clicked
#define DFLG_HASBEENCHOSEN  8  // dialog option is 'read'
#define DTFLG_SHOWPARSER    1  // show parser in this topic
#define DCHAR_NARRATOR  999
#define DCHAR_PLAYER    998
struct DialogTopic {
    char          optionnames[MAXTOPICOPTIONS][150];
    int           optionflags[MAXTOPICOPTIONS];
    short         entrypoints[MAXTOPICOPTIONS];
    short         startupentrypoint;
    short         codesize;
    int           numoptions;
    int           topicFlags;

    void ReadFromFile(Common::Stream *in);

    void ReadFromSavegame(Common::Stream *in);
    void WriteToSavegame(Common::Stream *out) const;
};

#endif // __AGS_CN_AC__DIALOGTOPIC_H