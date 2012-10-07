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
//
//
//=============================================================================
#ifndef __AGS_EE_SCRIPT__EXECUTINGSCRIPT_H
#define __AGS_EE_SCRIPT__EXECUTINGSCRIPT_H

#include "script/cc_instance.h"

enum PostScriptAction {
    ePSANewRoom,
    ePSAInvScreen,
    ePSARestoreGame,
    ePSARestoreGameDialog,
    ePSARunAGSGame,
    ePSARunDialog,
    ePSARestartGame,
    ePSASaveGame,
    ePSASaveGameDialog
};

#define MAX_QUEUED_SCRIPTS 4
#define MAX_QUEUED_ACTIONS 5
struct ExecutingScript {
    ccInstance *inst;
    PostScriptAction postScriptActions[MAX_QUEUED_ACTIONS];
    const char *postScriptActionNames[MAX_QUEUED_ACTIONS];
    char postScriptSaveSlotDescription[MAX_QUEUED_ACTIONS][100];
    int  postScriptActionData[MAX_QUEUED_ACTIONS];
    int  numPostScriptActions;
    char script_run_another[MAX_QUEUED_SCRIPTS][30];
    int  run_another_p1[MAX_QUEUED_SCRIPTS];
    int  run_another_p2[MAX_QUEUED_SCRIPTS];
    int  numanother;
    char forked;

    int queue_action(PostScriptAction act, int data, const char *aname);
    void run_another (char *namm, int p1, int p2);
    void init();
    ExecutingScript();
};

#endif // __AGS_EE_SCRIPT__EXECUTINGSCRIPT_H
