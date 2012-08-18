
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__GLOBALSCREEN_H
#define __AGS_EE_AC__GLOBALSCREEN_H

int  GetMaxScreenHeight ();
void FlipScreen(int amount);
void ShakeScreen(int severe);
void ShakeScreenBackground (int delay, int amount, int length);
void TintScreen(int red, int grn, int blu);
void my_fade_out(int spdd);
void SetScreenTransition(int newtrans);
void SetNextScreenTransition(int newtrans);
void SetFadeColor(int red, int green, int blue);
void FadeIn(int sppd);

#endif // __AGS_EE_AC__GLOBALSCREEN_H
