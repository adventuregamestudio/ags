
#include "acmain/ac_maindefines.h"


void SetScreenTransition(int newtrans) {
    if ((newtrans < 0) || (newtrans > FADE_LAST))
        quit("!SetScreenTransition: invalid transition type");

    play.fade_effect = newtrans;

    DEBUG_CONSOLE("Screen transition changed");
}

void SetNextScreenTransition(int newtrans) {
    if ((newtrans < 0) || (newtrans > FADE_LAST))
        quit("!SetNextScreenTransition: invalid transition type");

    play.next_screen_transition = newtrans;

    DEBUG_CONSOLE("SetNextScreenTransition engaged");
}

void SetFadeColor(int red, int green, int blue) {
    if ((red < 0) || (red > 255) || (green < 0) || (green > 255) ||
        (blue < 0) || (blue > 255))
        quit("!SetFadeColor: Red, Green and Blue must be 0-255");

    play.fade_to_red = red;
    play.fade_to_green = green;
    play.fade_to_blue = blue;
}


void FadeIn(int sppd) {
    EndSkippingUntilCharStops();

    if (play.fast_forward)
        return;

    my_fade_in(palette,sppd);
}

