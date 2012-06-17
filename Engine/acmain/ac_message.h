#ifndef __AC_MESSAGE_H
#define __AC_MESSAGE_H

void _DisplaySpeechCore(int chid, char *displbuf);
void _DisplayThoughtCore(int chid, const char *displbuf);
void Display(char*texx, ...);
int wgetfontheight(int font);
void draw_text_window(int*xins,int*yins,int*xx,int*yy,int*wii,int ovrheight=0, int ifnum=-1);
void get_message_text (int msnum, char *buffer, char giveErr = 1);
void DisplayMessage(int msnum);
void set_default_glmsg (int msgnum, const char* val);
int user_to_internal_skip_speech(int userval);
// Pass yy = -1 to find Y co-ord automatically
// allowShrink = 0 for none, 1 for leftwards, 2 for rightwards
// pass blocking=2 to create permanent overlay
int _display_main(int xx,int yy,int wii,char*todis,int blocking,int usingfont,int asspch, int isThought, int allowShrink, bool overlayPositionFixed);

extern block screenop;

#endif // __AC_MESSAGE_H