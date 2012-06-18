
#include "acrun/ac_scriptobject.h"
#include "acmain/ac_scriptdialogoptionsrendering.h"

void do_conversation(int dlgnum);
int show_dialog_options(int dlgnum, int sayChosenOption, bool runGameLoopsInBackground) ;
void RunDialog(int tum);
void SetDialogOption(int dlg,int opt,int onoroff);
int Dialog_GetID(ScriptDialog *sd);
int Dialog_GetOptionCount(ScriptDialog *sd);
int Dialog_GetShowTextParser(ScriptDialog *sd);
const char* Dialog_GetOptionText(ScriptDialog *sd, int option);
int Dialog_DisplayOptions(ScriptDialog *sd, int sayChosenOption);
int Dialog_GetOptionState(ScriptDialog *sd, int option);
int Dialog_HasOptionBeenChosen(ScriptDialog *sd, int option);
void Dialog_SetOptionState(ScriptDialog *sd, int option, int newState);
void Dialog_Start(ScriptDialog *sd);

int DialogOptionsRendering_GetX(ScriptDialogOptionsRendering *dlgOptRender);
void DialogOptionsRendering_SetX(ScriptDialogOptionsRendering *dlgOptRender, int newX);
int DialogOptionsRendering_GetY(ScriptDialogOptionsRendering *dlgOptRender);
void DialogOptionsRendering_SetY(ScriptDialogOptionsRendering *dlgOptRender, int newY);
int DialogOptionsRendering_GetWidth(ScriptDialogOptionsRendering *dlgOptRender);
void DialogOptionsRendering_SetWidth(ScriptDialogOptionsRendering *dlgOptRender, int newWidth);
int DialogOptionsRendering_GetHeight(ScriptDialogOptionsRendering *dlgOptRender);
void DialogOptionsRendering_SetHeight(ScriptDialogOptionsRendering *dlgOptRender, int newHeight);
int DialogOptionsRendering_GetParserTextboxX(ScriptDialogOptionsRendering *dlgOptRender);
void DialogOptionsRendering_SetParserTextboxX(ScriptDialogOptionsRendering *dlgOptRender, int newX);
int DialogOptionsRendering_GetParserTextboxY(ScriptDialogOptionsRendering *dlgOptRender);
void DialogOptionsRendering_SetParserTextboxY(ScriptDialogOptionsRendering *dlgOptRender, int newY);
int DialogOptionsRendering_GetParserTextboxWidth(ScriptDialogOptionsRendering *dlgOptRender);
void DialogOptionsRendering_SetParserTextboxWidth(ScriptDialogOptionsRendering *dlgOptRender, int newWidth);
ScriptDialog* DialogOptionsRendering_GetDialogToRender(ScriptDialogOptionsRendering *dlgOptRender);
ScriptDrawingSurface* DialogOptionsRendering_GetSurface(ScriptDialogOptionsRendering *dlgOptRender);
int DialogOptionsRendering_GetActiveOptionID(ScriptDialogOptionsRendering *dlgOptRender);
void DialogOptionsRendering_SetActiveOptionID(ScriptDialogOptionsRendering *dlgOptRender, int activeOptionID);

int Game_GetDialogCount();
void SetDialogOption(int dlg,int opt,int onoroff);
int GetDialogOption (int dlg, int opt);
int Game_GetDialogCount();
void StopDialog();


extern int longestline;

// Old dialog support
extern unsigned char** old_dialog_scripts;
extern char** old_speech_lines;

extern int said_speech_line; // used while in dialog to track whether screen needs updating
extern int said_text;

