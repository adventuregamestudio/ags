
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__GLOBALBUTTON_H
#define __AGS_EE_AC__GLOBALBUTTON_H

void SetButtonText(int guin,int objn,char*newtx);
void AnimateButton(int guin, int objn, int view, int loop, int speed, int repeat);
int  GetButtonPic(int guin, int objn, int ptype);
void SetButtonPic(int guin,int objn,int ptype,int slotn);

#endif // __AGS_EE_AC__GLOBALBUTTON_H
