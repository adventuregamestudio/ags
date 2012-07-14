//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__GLOBALDISPLAY_H
#define __AGS_EE_AC__GLOBALDISPLAY_H

void Display(char*texx, ...);
void DisplayAt(int xxp,int yyp,int widd,char*texx, ...);
void DisplayAtY (int ypos, char *texx);
void DisplayMessage(int msnum);
void DisplayMessageAtY(int msnum, int ypos);
void DisplayTopBar(int ypos, int ttexcol, int backcol, char *title, char*texx, ...);
// Display a room/global message in the bar
void DisplayMessageBar(int ypos, int ttexcol, int backcol, char *title, int msgnum);

void SetSpeechStyle (int newstyle);
void SetSkipSpeech (int newval);

#endif // __AGS_EE_AC__GLOBALDISPLAY_H
