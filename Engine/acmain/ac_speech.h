#ifndef __AC_SPEECH_H
#define __AC_SPEECH_H

#include "ac/characterinfo.h"

void SetTextWindowGUI (int guinum);
int DisplaySpeechBackground(int charid,char*speel);
int play_speech(int charid,int sndid);
void stop_speech();
void SetSpeechVolume(int newvol);
void SetSpeechFont (int fontnum);
void SetNormalFont (int fontnum);

void __scr_play_speech(int who, int which);

void SetVoiceMode (int newmod);
int IsVoxAvailable();
int IsMusicVoxAvailable ();
void SetSpeechStyle (int newstyle);
void _displayspeech(char*texx, int aschar, int xx, int yy, int widd, int isThought);
int get_character_currently_talking();

void DisplaySpeech(char*texx, int aschar);
void DisplaySpeechAt (int xx, int yy, int wii, int aschar, char*spch);

void SetSkipSpeech (int newval);

// Sierra-style speech settings
extern int face_talking,facetalkview,facetalkwait,facetalkframe;
extern int facetalkloop, facetalkrepeat, facetalkAllowBlink;
extern int facetalkBlinkLoop;
extern CharacterInfo *facetalkchar;

#endif // __AC_SPEECH_H