
#include "util/wgt2allg.h"
#include "ac/gamesetup.h"

GameSetup::GameSetup()
{
    digicard=DIGI_AUTODETECT; midicard=MIDI_AUTODETECT;
    mod_player=1; mp3_player=1;
    want_letterbox=0; windowed = 0;
    no_speech_pack = 0;
    refresh = 0;
    enable_antialiasing = 0;
    force_hicolor_mode = 0;
    disable_exception_handling = 0;
    enable_side_borders = 1;
    data_files_dir = NULL;
    main_data_filename = "ac2game.dat";
    base_width = 320;
    base_height = 200;
    gfxFilterID = NULL;
    gfxDriverID = NULL;
}
