
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__GLOBALWALKABLEAREA_H
#define __AGS_EE_AC__GLOBALWALKABLEAREA_H

int   GetScalingAt (int x, int y);
void  SetAreaScaling(int area, int min, int max);
void  RemoveWalkableArea(int areanum);
void  RestoreWalkableArea(int areanum);
int   GetWalkableAreaAt(int xxx,int yyy);

#endif // __AGS_EE_AC__GLOBALWALKABLEAREA_H
