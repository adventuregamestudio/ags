
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__GLOBALROOM_H
#define __AGS_EE_AC__GLOBALROOM_H

void SetAmbientTint (int red, int green, int blue, int opacity, int luminance);
void NewRoom(int nrnum);
void NewRoomEx(int nrnum,int newx,int newy);
void NewRoomNPC(int charid, int nrnum, int newx, int newy);
void ResetRoom(int nrnum);
int  HasPlayerBeenInRoom(int roomnum);
void CallRoomScript (int value);
int  HasBeenToRoom (int roomnum);
int GetRoomProperty (const char *property);
void GetRoomPropertyText (const char *property, char *bufer);

void SetBackgroundFrame(int frnum);
int GetBackgroundFrame() ;

#endif // __AGS_EE_AC__GLOBALROOM_H
