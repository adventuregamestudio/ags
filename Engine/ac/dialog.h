
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__DIALOG_H
#define __AGS_EE_AC__DIALOG_H

#include "ac/dynobj/scriptdialog.h"

int  Dialog_GetID(ScriptDialog *sd);
int  Dialog_GetOptionCount(ScriptDialog *sd);
int  Dialog_GetShowTextParser(ScriptDialog *sd);
const char* Dialog_GetOptionText(ScriptDialog *sd, int option);
int  Dialog_DisplayOptions(ScriptDialog *sd, int sayChosenOption);
int  Dialog_GetOptionState(ScriptDialog *sd, int option);
int  Dialog_HasOptionBeenChosen(ScriptDialog *sd, int option);
void Dialog_SetOptionState(ScriptDialog *sd, int option, int newState);
void Dialog_Start(ScriptDialog *sd);

void do_conversation(int dlgnum);
int  show_dialog_options(int dlgnum, int sayChosenOption, bool runGameLoopsInBackground) ;

#endif // __AGS_EE_AC__DIALOG_H
