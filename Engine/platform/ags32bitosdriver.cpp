
#include "platform/32bit/ags32bitosdriver.h"
#include "util/wgt2allg.h"
#include "ac/common.h"

#if !defined(BSD_VERSION) && (defined(LINUX_VERSION) || defined(WINDOWS_VERSION))
#include "libcda.h"
#endif

// ********* NON-DOS BASE CLASS ******

void AGS32BitOSDriver::GetSystemTime(ScriptDateTime *sdt) {
    struct tm *newtime;
    time_t long_time;

    time( &long_time );
    newtime = localtime( &long_time );

    sdt->hour = newtime->tm_hour;
    sdt->minute = newtime->tm_min;
    sdt->second = newtime->tm_sec;
    sdt->day = newtime->tm_mday;
    sdt->month = newtime->tm_mon + 1;
    sdt->year = newtime->tm_year + 1900;
}

void AGS32BitOSDriver::YieldCPU() {
    this->Delay(1);
}


// ********** CD Player Functions common to Win and Linux ********

#if !defined(ANDROID_VERSION) && !defined(PSP_VERSION) && !defined(DOS_VERSION) && !defined(BSD_VERSION) && !defined(MAC_VERSION)

// from ac_cdplayer
extern int use_cdplayer;
extern int need_to_stop_cd;

int numcddrives=0;

int cd_player_init() {
    int erro = cd_init();
    if (erro) return -1;
    numcddrives=1;
    use_cdplayer=1;
    return 0;
}

int cd_player_control(int cmdd, int datt) {
    // WINDOWS & LINUX VERSION
    if (cmdd==1) {
        if (cd_current_track() > 0) return 1;
        return 0;
    }
    else if (cmdd==2) {
        cd_play_from(datt);
        need_to_stop_cd=1;
    }
    else if (cmdd==3) 
        cd_pause();
    else if (cmdd==4) 
        cd_resume();
    else if (cmdd==5) {
        int first,last;
        if (cd_get_tracks(&first,&last)==0)
            return (last-first)+1;
        else return 0;
    }
    else if (cmdd==6)
        cd_eject();
    else if (cmdd==7)
        cd_close();
    else if (cmdd==8)
        return numcddrives;
    else if (cmdd==9) ;
    else quit("!CDAudio: Unknown command code");

    return 0;
}

#endif  // !defined(DJGPP) && !defined(BSD_VERSION) && !defined(MAC_VERSION)
