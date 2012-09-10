
#include "ac/global_dialog.h"
#include "util/wgt2allg.h"
#include "ac/common.h"
#include "ac/dialog.h"
#include "ac/dialogtopic.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "debug/debug_log.h"
#include "script/script.h"

extern GameSetupStruct game;
extern GameState play;
extern DialogTopic *dialog;

void RunDialog(int tum) {
    if ((tum<0) | (tum>=game.numdialog))
        quit("!RunDialog: invalid topic number specified");

    can_run_delayed_command();

    if (play.stop_dialog_at_end != DIALOG_NONE) {
        if (play.stop_dialog_at_end == DIALOG_RUNNING)
            play.stop_dialog_at_end = DIALOG_NEWTOPIC + tum;
        else
            quit("!NewRoom: two NewRoom/RunDiaolg/StopDialog requests within dialog");
        return;
    }

    if (inside_script) 
        curscript->queue_action(ePSARunDialog, tum, "RunDialog");
    else
        do_conversation(tum);
}


void StopDialog() {
  if (play.stop_dialog_at_end == DIALOG_NONE) {
    debug_log("StopDialog called, but was not in a dialog");
    DEBUG_CONSOLE("StopDialog called but no dialog");
    return;
  }
  play.stop_dialog_at_end = DIALOG_STOP;
}

void SetDialogOption(int dlg,int opt,int onoroff) {
  if ((dlg<0) | (dlg>=game.numdialog))
    quit("!SetDialogOption: Invalid topic number specified");
  if ((opt<1) | (opt>dialog[dlg].numoptions))
    quit("!SetDialogOption: Invalid option number specified");
  opt--;

  dialog[dlg].optionflags[opt]&=~DFLG_ON;
  if ((onoroff==1) & ((dialog[dlg].optionflags[opt] & DFLG_OFFPERM)==0))
    dialog[dlg].optionflags[opt]|=DFLG_ON;
  else if (onoroff==2)
    dialog[dlg].optionflags[opt]|=DFLG_OFFPERM;
}

int GetDialogOption (int dlg, int opt) {
  if ((dlg<0) | (dlg>=game.numdialog))
    quit("!GetDialogOption: Invalid topic number specified");
  if ((opt<1) | (opt>dialog[dlg].numoptions))
    quit("!GetDialogOption: Invalid option number specified");
  opt--;

  if (dialog[dlg].optionflags[opt] & DFLG_OFFPERM)
    return 2;
  if (dialog[dlg].optionflags[opt] & DFLG_ON)
    return 1;
  return 0;
}
