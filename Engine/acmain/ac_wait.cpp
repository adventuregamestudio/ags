
#include <stdio.h>
#include "wgt2allg.h"
#include "ali3d.h"
#include "acmain/ac_maindefines.h"
#include "acmain/ac_wait.h"
#include "ac/ac_common.h"
#include "acrun/ac_runninggame.h"
#include "acmain/ac_main.h"

void scrWait(int nloops) {
    if (nloops < 1)
        quit("!Wait: must wait at least 1 loop");

    play.wait_counter = nloops;
    play.key_skip_wait = 0;
    do_main_cycle(UNTIL_MOVEEND,(int)&play.wait_counter);
}

int WaitKey(int nloops) {
    if (nloops < 1)
        quit("!WaitKey: must wait at least 1 loop");

    play.wait_counter = nloops;
    play.key_skip_wait = 1;
    do_main_cycle(UNTIL_MOVEEND,(int)&play.wait_counter);
    if (play.wait_counter < 0)
        return 1;
    return 0;
}

int WaitMouseKey(int nloops) {
    if (nloops < 1)
        quit("!WaitMouseKey: must wait at least 1 loop");

    play.wait_counter = nloops;
    play.key_skip_wait = 3;
    do_main_cycle(UNTIL_MOVEEND,(int)&play.wait_counter);
    if (play.wait_counter < 0)
        return 1;
    return 0;
}
