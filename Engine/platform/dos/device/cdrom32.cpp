/*
  DJGPP CD-ROM library (c) 1999 Chris Jones
  Based on MYCDROM real mode MSCDEX interface (c) 1997 Chris Jones
  REQUIRES MSCDEX 2.10 OR HIGHER.
  Use cd_installed()  then  cd_getversion()  to make sure that it is v2.10.

  This is UNPUBLISHED PROPRIETARY SOURCE CODE;
  the contents of this file may not be disclosed to third parties,
  copied or duplicated in any form, in whole or in part, without
  prior express permission from Chris Jones.
*/

#include <dos.h>
#include <string.h>
#include <go32.h>
#include <dpmi.h>

// flags returned with cd_getstatus
#define CDS_DRIVEOPEN    0x0001 // tray is open
#define CDS_DRIVELOCKED  0x0002 // tray locked shut by software
#define CDS_AUDIOSUPPORT 0x0010 // supports audio CDs
#define CDS_DRIVEEMPTY   0x0800 // no CD in drive

// function definitions
int cd_installed();
int cd_getversion();
int cd_getdriveletters(char *);
void cd_driverinit(int);
void cd_driverclose(int);
long cd_getstatus(int);
void cd_playtrack(int, int);
void cd_stopmusic(int);
void cd_resumemusic(int);
void cd_eject(int);
void cd_uneject(int);
int cd_getlasttrack(int);
int cd_isplayingaudio(int);

#define SET_TB_ESBX {\
  rrr.x.es=__tb/16;\
  rrr.x.bx=__tb%16;\
  }
__dpmi_regs rrr;
int _cd_numdrives = 0;
int cd_lasterror = 0;

// cd_installed: check if MSCDEX is installed.
// Returns 0 if not, else number of CD-ROM drives
int cd_installed()
{
  if (_go32_info_block.size_of_transfer_buffer < 100)
    return 0;
  rrr.x.ax = 0x1500;
  __dpmi_int(0x2f, &rrr);
  if (rrr.x.ax != 0x15ff)
    return 0;
  _cd_numdrives = rrr.x.bx;
  return rrr.x.bx;
}

// cd_getversion: gets the MSCDEX version number
// Returns major in high byte, minor in low byte
// eg. 0x020A is v2.10
int cd_getversion()
{
  rrr.x.ax = 0x150C;
  rrr.x.bx = 0;
  __dpmi_int(0x2f, &rrr);
  if (rrr.x.bx == 0)
    return 0x0100;
  return rrr.x.bx;
}

// cd_getdriveletters: fills in the buffer with the number of each CD-ROM
// drive (0=A, 1=B, etc). BUFF must be 26 bytes (in case all drives are
// CD-ROMs!). Returns number of drives.
int cd_getdriveletters(char *buff)
{
  rrr.x.ax = 0x150D;
  SET_TB_ESBX __dpmi_int(0x2f, &rrr);
  dosmemget(__tb, 26, buff);
  return _cd_numdrives;
}

// the CD IOCTL structure
#pragma pack(1)
struct CD_DEVINFO
{
  unsigned char size;
  unsigned char subunit;
  unsigned char function;
  unsigned short status;
  unsigned char reserved[8];
    CD_DEVINFO()
  {
    size = sizeof(CD_DEVINFO);
    subunit = 0;
  }
};

#define PCKD __attribute__((packed))
struct CD_IOCTL
{
  unsigned char size PCKD;
  unsigned char subunit PCKD;
  unsigned char function PCKD;
  unsigned short status PCKD;
  unsigned char reserved[8] PCKD;
  unsigned char mediadescriptor PCKD;
  unsigned long adata PCKD;     // real mode pointer
  unsigned short adatasize PCKD;
  unsigned short startsector PCKD;
  unsigned long volid PCKD;
    CD_IOCTL()
  {
    subunit = 0;
    volid = 0;
    mediadescriptor = 0;
    startsector = 0;
  }
};

#pragma pack()
CD_DEVINFO cddi;
CD_IOCTL cdio;

// cd_sendrequest: Sends an IOCTL request to the driver.
// Pass the drive number
void cd_sendrequest(int drivlab, CD_DEVINFO * reqbuf)
{
  rrr.x.ax = 0x1510;
  rrr.x.cx = drivlab;
  SET_TB_ESBX dosmemput(reqbuf, reqbuf->size, __tb);
  __dpmi_int(0x2f, &rrr);
  dosmemget(__tb, reqbuf->size, reqbuf);
  cd_lasterror = reqbuf->status;
}

#define _use_tb (__tb+50)
void cd_ioctl(int drivlab, int funccode, char *addinfo)
{
  cdio.size = sizeof(cdio);     //0x10;
  cdio.subunit = 0;
  cdio.function = funccode;
  // use a 50-byte offset into transfer buffer, because sendrequest uses
  // the start of the tb
  cdio.adata = ((_use_tb / 16) << 16) & 0xffff0000;
  cdio.adata |= (_use_tb % 16);
//  printf("adata: %08X  tb: %08X\n",cdio.adata,_use_tb);
  cdio.adatasize = 20;
  dosmemput(addinfo, 20, _use_tb);
//  printf("at tb: %X %X\n",_farpeekb(_dos_ds,_use_tb),_farpeekb(_dos_ds,_use_tb+1));
  cd_sendrequest(drivlab, (CD_DEVINFO *) & cdio);
//  printf("afterat tb: %X %X\n",_farpeekb(_dos_ds,_use_tb),_farpeekb(_dos_ds,_use_tb+1));
  dosmemget(_use_tb, 20, addinfo);
//  printf("after in addinfo: %X %X\n",addinfo[0],addinfo[1]);
}

void cd_ioctloutput(int driv, char *addinfo)
{
  cd_ioctl(driv, 0x0c, addinfo);
}
void cd_ioctlinput(int driv, char *addinfo)
{
  cd_ioctl(driv, 3, addinfo);
}

// cd_driverinit: initializes the driver for all IOCTL functions
void cd_driverinit(int drivlab)
{
  cddi.function = 0x0D;
  cd_sendrequest(drivlab, &cddi);
}

// cd_driverclose: closes the driver (after finished using cd-audio functions)
void cd_driverclose(int drivlab)
{
  cddi.function = 0x0E;
  cd_sendrequest(drivlab, &cddi);
}

// cd_getstatus: returns combination of CDS_xxx flags
long cd_getstatus(int drivlab)
{
  char bufr[20] = { 6, 0, 0, 0, 0 };
  cd_ioctlinput(drivlab, bufr);
  long *statpt = (long *)&bufr[1];
  return statpt[0];
}

// cd_stopmusic: stops the currently playing CD Audio track
void cd_stopmusic(int drivlab)
{
  cddi.function = 0x85;
  cd_sendrequest(drivlab, &cddi);
}

// cd_resumemusic: resumes stopped music
void cd_resumemusic(int drivlab)
{
  cddi.function = 0x88;
  cd_sendrequest(drivlab, &cddi);
}

char ioctbuf[20];
// cd_eject: ejects the drive tray
void cd_eject(int drivlab)
{
  memset(ioctbuf, 0, 20);
  cd_ioctloutput(drivlab, ioctbuf);
}

// cd_uneject: draws back in the drive tray
void cd_uneject(int drivlab)
{
  memset(ioctbuf, 0, 20);
  ioctbuf[0] = 5;
  cd_ioctloutput(drivlab, ioctbuf);
}

int cd_getlastsector(int driv)
{
  memset(ioctbuf, 0, 20);
  ioctbuf[0] = 10;
  cd_ioctlinput(driv, ioctbuf);
  long *llpt = (long *)&ioctbuf[3];
  return llpt[0];
}

int cd_getlasttrack(int driv)
{
  cd_getlastsector(driv);
  return ioctbuf[2];
}

long cd_gettrackaddr(int drivv, int trak)
{
  memset(ioctbuf, 0, 20);
  ioctbuf[0] = 11;
  ioctbuf[1] = trak;
  cd_ioctlinput(drivv, ioctbuf);
  long *lptr = (long *)&ioctbuf[2];
  return lptr[0];
}

int cd_isplayingaudio(int drivv)
{
  cd_getstatus(drivv);
  if (cd_lasterror & 0x0200) {
    return 1;                   // playing
  }
  return 0;                     // not playing
}

void cd_playtrack(int drivv, int trak)
{
  long trakadr = cd_gettrackaddr(drivv, trak);
  long lastsec = cd_getlastsector(drivv) - trakadr;
  cdio.size = 22;
  cdio.function = 0x84;
  cdio.mediadescriptor = 1;
  long *adptr = (long *)&cdio.adata;
  adptr[0] = trakadr;
  adptr[1] = lastsec;
  cd_sendrequest(drivv, (CD_DEVINFO *) & cdio);
  cdio.mediadescriptor = 0;
}

#if 0
#include <stdio.h>
#include <conio.h>

int main()
{
  printf("testing 32-bit MSCDEX access...\n");
  if (cd_installed() == 0) {
    printf("MSCDEX not found.\n");
    return 1;
  }

  if (cd_getversion() < 0x020A) {
    printf("MSCDEX v2.10 required.\n");
    return 2;
  }

  char drbuf[26];
  cd_getdriveletters(drbuf);
  cd_driverinit(drbuf[0]);
  printf("CDROM is drive %c:\n", 'A' + drbuf[0]);
  printf("%d tracks.\n", cd_getlasttrack(drbuf[0]));
  printf("cd play status: %d\n", cd_isplayingaudio(drbuf[0]));

  long stats = cd_getstatus(drbuf[0]);
  printf("drive status: %08X\n", stats);

  if (stats & CDS_DRIVEOPEN)
    printf("drive is open\n");

  if (stats & CDS_DRIVEEMPTY)
    printf("drive is empty\n");

  printf("playing track...\n");
  cd_playtrack(drbuf[0], 2);
  printf("Bye!\n");
  cd_driverclose(drbuf[0]);
  return 0;
}

#endif
