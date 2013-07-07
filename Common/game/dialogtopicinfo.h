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
// DialogTopicInfo, a class of static dialog topic data
//
//=============================================================================
#ifndef __AGS_CN_GAME__DIALOGTOPICINFO_H
#define __AGS_CN_GAME__DIALOGTOPICINFO_H

#include "util/array.h"
#include "util/string.h"

#define LEGACY_MAX_DIALOG_TOPIC_OPTIONS 30

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


namespace AGS
{
namespace Common
{

enum DialogVersion
{
    kDialogVersion_pre340,
    kDialogVersion_340_alpha,
    kDialogVersion_Current = kDialogVersion_340_alpha
};

enum DialogOptionFlags
{
    kDialogOption_IsOn               = 0x01, // currently enabled
    kDialogOption_IsPermanentlyOff   = 0x02, // off forever (can't be turned on)
    kDialogOption_NoRepeat           = 0x04, // character doesn't repeat it when clicked
    kDialogOption_HasBeenChosen      = 0x08 // dialog option is 'read'
};

enum DialogTopicFlags
{
    kDialogTopic_ShowParser          = 0x01 // show parser in this topic
};

struct DialogOption
{
    String  Name;
    int32_t Flags;
    int16_t EntryPoint;

    DialogOption()
    {
        Flags = 0;
        EntryPoint = 0;
    }
};

class DialogTopicInfo
{
public:
    DialogTopicInfo();

    void ReadFromFile(Common::Stream *in, DialogVersion version);
    void WriteToFile(Common::Stream *out);

// TODO: all members are currently public; hide them later
public:
    int32_t                     Flags;
    ObjectArray<DialogOption>   Options;
    int32_t                     OptionCount;
    int16_t                     StartUpEntryPoint;
    int16_t                     OldCodeSize;    // old-style dialog script code size    
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GAME__DIALOGTOPICINFO_H
