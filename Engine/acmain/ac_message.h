#ifndef __AC_MESSAGE_H
#define __AC_MESSAGE_H

#include "gui/guimain.h"

int wgetfontheight(int font);
int wgettextwidth_compensate(const char *tex, int font);
void set_default_glmsg (int msgnum, const char* val);
void do_corner(int sprn,int xx1,int yy1,int typx,int typy);
int get_but_pic(GUIMain*guo,int indx);
void draw_button_background(int xx1,int yy1,int xx2,int yy2,GUIMain*iep);
// Calculate the width that the left and right border of the textwindow
// GUI take up
int get_textwindow_border_width (int twgui);
// get the hegiht of the text window's top border
int get_textwindow_top_border_height (int twgui);
void draw_text_window(int*xins,int*yins,int*xx,int*yy,int*wii,int ovrheight, int ifnum);
void draw_text_window_and_bar(int*xins,int*yins,int*xx,int*yy,int*wii,int ovrheight=0, int ifnum=-1);
void wouttext_outline(int xxp, int yyp, int usingfont, char*texx);

int GetTextDisplayTime (char *text, int canberel=0);
void wouttext_aligned (int usexp, int yy, int oriwid, int usingfont, const char *text, int align);
int user_to_internal_skip_speech(int userval);
bool ShouldAntiAliasText();

int _display_main(int xx,int yy,int wii,char*todis,int blocking,int usingfont,int asspch, int isThought, int allowShrink, bool overlayPositionFixed);
void _display_at(int xx,int yy,int wii,char*todis,int blocking,int asspch, int isThought, int allowShrink, bool overlayPositionFixed);
void DisplayAt(int xxp,int yyp,int widd,char*texx, ...);
void DisplayAtY (int ypos, char *texx);
void Display(char*texx, ...);
void _DisplaySpeechCore(int chid, char *displbuf);
void __sc_displayspeech(int chid,char*texx, ...);
void DisplayTopBar(int ypos, int ttexcol, int backcol, char *title, char*texx, ...);
// Display a room/global message in the bar
void DisplayMessageBar(int ypos, int ttexcol, int backcol, char *title, int msgnum);
void _DisplayThoughtCore(int chid, const char *displbuf);
void DisplayThought(int chid, const char*texx, ...);
void replace_tokens(char*srcmes,char*destm, int maxlen = 99999);
char *get_global_message (int msnum);
void get_message_text (int msnum, char *buffer, char giveErr = 1);
void GetMessageText (int msg, char *buffer);


void DisplayMessageAtY(int msnum, int ypos);
void DisplayMessage(int msnum);

extern block screenop;

#endif // __AC_MESSAGE_H