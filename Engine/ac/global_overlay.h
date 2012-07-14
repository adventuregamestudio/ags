
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__GLOBALOVERLAY_H
#define __AGS_EE_AC__GLOBALOVERLAY_H

void RemoveOverlay(int ovrid);
int  CreateGraphicOverlay(int xx,int yy,int slott,int trans);
int  CreateTextOverlayCore(int xx, int yy, int wii, int fontid, int clr, const char *tex, int allowShrink);
int  CreateTextOverlay(int xx,int yy,int wii,int fontid,int clr,char*texx, ...);
void SetTextOverlay(int ovrid,int xx,int yy,int wii,int fontid,int clr,char*texx,...);
void MoveOverlay(int ovrid, int newx,int newy);
int  IsOverlayValid(int ovrid);

#endif // __AGS_EE_AC__GLOBALOVERLAY_H
