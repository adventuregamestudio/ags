
#include "acmain/ac_maindefines.h"
#include "acmain/ac_cutscene.h"


void start_skipping_cutscene () {
    play.fast_forward = 1;
    // if a drop-down icon bar is up, remove it as it will pause the game
    if (ifacepopped>=0)
        remove_popup_interface(ifacepopped);

    // if a text message is currently displayed, remove it
    if (is_text_overlay > 0)
        remove_screen_overlay(OVER_TEXTMSG);

}

void check_skip_cutscene_keypress (int kgn) {

    if ((play.in_cutscene > 0) && (play.in_cutscene != 3)) {
        if ((kgn != 27) && ((play.in_cutscene == 1) || (play.in_cutscene == 5)))
            ;
        else
            start_skipping_cutscene();
    }

}


// Helper functions used by StartCutscene/EndCutscene, but also
// by SkipUntilCharacterStops
void initialize_skippable_cutscene() {
  play.end_cutscene_music = -1;
}

void stop_fast_forwarding() {
  // when the skipping of a cutscene comes to an end, update things
  play.fast_forward = 0;
  setpal();
  if (play.end_cutscene_music >= 0)
    newmusic(play.end_cutscene_music);

  // Restore actual volume of sounds
  for (int aa = 0; aa < MAX_SOUND_CHANNELS; aa++)
  {
    if ((channels[aa] != NULL) && (!channels[aa]->done) && 
        (channels[aa]->volAsPercentage == 0) &&
        (channels[aa]->originalVolAsPercentage > 0)) 
    {
      channels[aa]->volAsPercentage = channels[aa]->originalVolAsPercentage;
      channels[aa]->set_volume((channels[aa]->volAsPercentage * 255) / 100);
    }
  }

  update_music_volume();
}

void SkipUntilCharacterStops(int cc) {
  if (!is_valid_character(cc))
    quit("!SkipUntilCharacterStops: invalid character specified");
  if (game.chars[cc].room!=displayed_room)
    quit("!SkipUntilCharacterStops: specified character not in current room");

  // if they are not currently moving, do nothing
  if (!game.chars[cc].walking)
    return;

  if (play.in_cutscene)
    quit("!SkipUntilCharacterStops: cannot be used within a cutscene");

  initialize_skippable_cutscene();
  play.fast_forward = 2;
  play.skip_until_char_stops = cc;
}

void EndSkippingUntilCharStops() {
  // not currently skipping, so ignore
  if (play.skip_until_char_stops < 0)
    return;

  stop_fast_forwarding();
  play.skip_until_char_stops = -1;
}




// skipwith decides how it can be skipped:
// 1 = ESC only
// 2 = any key
// 3 = mouse button
// 4 = mouse button or any key
// 5 = right click or ESC only
void StartCutscene (int skipwith) {
  if (play.in_cutscene)
    quit("!StartCutscene: already in a cutscene");

  if ((skipwith < 1) || (skipwith > 5))
    quit("!StartCutscene: invalid argument, must be 1 to 5.");

  // make sure they can't be skipping and cutsceneing at the same time
  EndSkippingUntilCharStops();

  play.in_cutscene = skipwith;
  initialize_skippable_cutscene();
}

int EndCutscene () {
  if (play.in_cutscene == 0)
    quit("!EndCutscene: not in a cutscene");

  int retval = play.fast_forward;
  play.in_cutscene = 0;
  // Stop it fast-forwarding
  stop_fast_forwarding();

  // make sure that the screen redraws
  invalidate_screen();

  // Return whether the player skipped it
  return retval;
}

int Game_GetSkippingCutscene()
{
  if (play.fast_forward)
  {
    return 1;
  }
  return 0;
}

int Game_GetInSkippableCutscene()
{
  if (play.in_cutscene)
  {
    return 1;
  }
  return 0;
}
