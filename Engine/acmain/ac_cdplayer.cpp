
#include <stdio.h>
#include "acmain/ac_maindefines.h"
#include "acmain/ac_cdplayer.h"
#include "acrun/ac_platformdriver.h"

int use_cdplayer=0;
bool triedToUseCdAudioCommand = false;
int need_to_stop_cd=0;

int init_cd_player() 
{
  use_cdplayer=0;
  return platform->InitializeCDPlayer();
}

int cd_manager(int cmdd,int datt) 
{
  if (!triedToUseCdAudioCommand)
  {
    triedToUseCdAudioCommand = true;
    init_cd_player();
  }
  if (cmdd==0) return use_cdplayer;
  if (use_cdplayer==0) return 0;  // ignore other commands

  return platform->CDPlayerCommand(cmdd, datt);
}
