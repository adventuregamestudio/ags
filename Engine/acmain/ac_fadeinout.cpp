
#include "acmain/ac_maindefines.h"



void FadeIn(int sppd) {
  EndSkippingUntilCharStops();

  if (play.fast_forward)
    return;

  my_fade_in(palette,sppd);
}

