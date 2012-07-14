
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__CDAUDIO_H
#define __AGS_EE_AC__CDAUDIO_H

// CD Player functions
// flags returned with cd_getstatus
#define CDS_DRIVEOPEN    0x0001  // tray is open
#define CDS_DRIVELOCKED  0x0002  // tray locked shut by software
#define CDS_AUDIOSUPPORT 0x0010  // supports audio CDs
#define CDS_DRIVEEMPTY   0x0800  // no CD in drive

int     init_cd_player() ;
int     cd_manager(int cmdd,int datt) ;

#endif // __AGS_EE_AC__CDAUDIO_H
