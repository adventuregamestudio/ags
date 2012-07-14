#ifndef DOS_VERSION
#error This file should only be compiled on the DOS build
#endif

// ************ DOS ************

#include "acplatfm.h"

#include <conio.h>

extern void setupmain();
char cd_driveletters[26],cddrive;

struct AGSDOS : AGSPlatformDriver {
  virtual int  CDPlayerCommand(int cmdd, int datt);
  virtual void Delay(int millis);
  virtual void DisplayAlert(const char*, ...);
  virtual unsigned long GetDiskFreeSpaceMB();
  virtual const char* GetNoMouseErrorString();
  virtual eScriptSystemOSID GetSystemOSID();
  virtual void GetSystemTime(ScriptDateTime*) ;
  virtual int  InitializeCDPlayer();
  virtual void PlayVideo(const char* name, int skip, int flags);
  virtual void PostAllegroExit();
  virtual int  RunSetup();
  virtual void ShutdownCDPlayer();
  virtual void WriteConsole(const char*, ...);
  virtual void YieldCPU();
  virtual void InitialiseAbufAtStartup();
  virtual void FinishedUsingGraphicsMode();
};

int AGSDOS::CDPlayerCommand(int cmdd, int datt) {
  if (cmdd==1)  // query if playing audio
    return cd_isplayingaudio(cddrive);
  else if (cmdd==6)
    cd_eject(cddrive);
  else if (cmdd==7)
    cd_uneject(cddrive);
  else if (cmdd==8)
    return numcddrives;
  else if (cmdd==9) {
    if ((datt<1) | (datt>numcddrives)) return 0;
    cddrive=cd_driveletters[datt-1];
    }
  else if ((cd_getstatus(cddrive) & (CDS_DRIVEOPEN | CDS_DRIVEEMPTY))!=0)
    return 0;
  else if (cmdd==2) { // play track
    cd_playtrack(cddrive,datt);
    need_to_stop_cd=1;
    }
  else if (cmdd==3) // stop music
    cd_stopmusic(cddrive);
  else if (cmdd==4) // resume music
    cd_resumemusic(cddrive);
  else if (cmdd==5)  // get num tracks
    return cd_getlasttrack(cddrive);
  else
    quit("unknown CD command");

  return 0;
}

void AGSDOS::DisplayAlert(const char *text, ...) {
  char displbuf[2000];
  va_list ap;
  va_start(ap, text);
  vsprintf(displbuf, text, ap);
  va_end(ap);
  
  this->WriteConsole(displbuf);
  getch();
}

void AGSDOS::Delay(int millis) {
  delay(millis);
}

unsigned long AGSDOS::GetDiskFreeSpaceMB() {
  struct diskfree_t df;
  unsigned long freebytes=0;
  if ( !_dos_getdiskfree(0, &df) ) {
       freebytes = (unsigned long)df.avail_clusters *
                   (unsigned long)df.bytes_per_sector *
                   (unsigned long)df.sectors_per_cluster;
  }

  return freebytes / 1000000;
}

const char* AGSDOS::GetNoMouseErrorString() {
  return "This game requires a mouse. You need to load a DOS mouse driver to\n"
      "play this game. Try typing 'MOUSE'.\n";
}

eScriptSystemOSID AGSDOS::GetSystemOSID() {
  return eOS_DOS;
}

void AGSDOS::GetSystemTime(ScriptDateTime *sdt) {
  struct time tt1;
  struct date dd1;
  gettime(&tt1);
  getdate(&dd1);
  sdt->hour = tt1.ti_hour;
  sdt->minute = tt1.ti_min;
  sdt->second = tt1.ti_sec;
  sdt->day = dd1.da_day;
  sdt->month = dd1.da_mon;
  sdt->year = dd1.da_year;
}

int AGSDOS::InitializeCDPlayer() {
  numcddrives=cd_installed();
  if (numcddrives==0) return -1;
  if (cd_getversion()<0x020A) return -2;
  cd_getdriveletters(cd_driveletters);
  cddrive=cd_driveletters[0];
  use_cdplayer=1;
  return 0;
}

void AGSDOS::PlayVideo(const char *name, int skip, int flags) {
  // do nothing
}

void AGSDOS::PostAllegroExit() {
  // do nothing
}

int AGSDOS::RunSetup() {
  setupmain();
  return 0;
}

void AGSDOS::ShutdownCDPlayer() {
  cd_exit();
}

void AGSDOS::WriteConsole(const char *text, ...) {
  char displbuf[2000];
  va_list ap;
  va_start(ap, text);
  vsprintf(displbuf, text, ap);
  va_end(ap);
  
  printf("%s", displbuf);
}

void AGSDOS::YieldCPU() {
  yield_timeslice();
}

void AGSDOS::InitialiseAbufAtStartup()
{
  // sort the mouse out in the DOS ver
  set_gfx_mode(GFX_VGA,320,200,320,200);
  // disable Ctrl-C exiting (ctrl-break will still work)
  __djgpp_set_ctrl_c(0);
  abuf = screen;
}

void AGSDOS::FinishedUsingGraphicsMode()
{
  set_gfx_mode(GFX_TEXT,80,25,0,0);
}

AGSPlatformDriver* AGSPlatformDriver::GetDriver() {
  if (instance == NULL)
    instance = new AGSDOS();
  return instance;
}
