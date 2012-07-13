
#define IS_RECORD_UNIT
#include "wgt2allg.h"
#include "ac/ac_common.h"
#include "acmain/ac_maindefines.h"
#include "ac/audiodefines.h"
#include "ac/game.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_display.h"
#include "ac/global_game.h"
#include "ac/keycode.h"
#include "ac/mouse.h"
#include "ac/record.h"
#include "cs/cs_utils.h"
#include "gfx/gfxfilter.h"
#include "mousew32.h"

extern GameSetupStruct game;
extern GameState play;
extern int disable_mgetgraphpos;
extern int mousex,mousey;
extern unsigned long loopcounter,lastcounter;
extern volatile unsigned long globalTimerCounter;
extern SOUNDCLIP *channels[MAX_SOUND_CHANNELS+1];
extern GFXFilter *filter;
extern int pluginSimulatedClick;
extern int displayed_room;
extern char check_dynamic_sprites_at_exit;

char replayfile[MAX_PATH] = "record.dat";
int replay_time = 0;
unsigned long replay_last_second = 0;
int replay_start_this_time = 0;

short *recordbuffer = NULL;
int  recbuffersize = 0, recsize = 0;

const char *replayTempFile = "~replay.tmp";

int mouse_z_was = 0;

void write_record_event (int evnt, int dlen, short *dbuf) {

    recordbuffer[recsize] = play.gamestep;
    recordbuffer[recsize+1] = evnt;

    for (int i = 0; i < dlen; i++)
        recordbuffer[recsize + i + 2] = dbuf[i];
    recsize += dlen + 2;

    if (recsize >= recbuffersize - 100) {
        recbuffersize += 10000;
        recordbuffer = (short*)realloc (recordbuffer, recbuffersize * sizeof(short));
    }

    play.gamestep++;
}
void disable_replay_playback () {
    play.playback = 0;
    if (recordbuffer)
        free (recordbuffer);
    recordbuffer = NULL;
    disable_mgetgraphpos = 0;
}

void done_playback_event (int size) {
    recsize += size;
    play.gamestep++;
    if ((recsize >= recbuffersize) || (recordbuffer[recsize+1] == REC_ENDOFFILE))
        disable_replay_playback();
}

int rec_getch () {
    if (play.playback) {
        if ((recordbuffer[recsize] == play.gamestep) && (recordbuffer[recsize + 1] == REC_GETCH)) {
            int toret = recordbuffer[recsize + 2];
            done_playback_event (3);
            return toret;
        }
        // Since getch() waits for a key to be pressed, if we have no
        // record for it we're out of sync
        quit("out of sync in playback in getch");
    }
    int result = my_readkey();
    if (play.recording) {
        short buff[1] = {result};
        write_record_event (REC_GETCH, 1, buff);
    }

    return result;  
}

int rec_kbhit () {
    if ((play.playback) && (recordbuffer != NULL)) {
        // check for real keypresses to abort the replay
        if (keypressed()) {
            if (my_readkey() == 27) {
                disable_replay_playback();
                return 0;
            }
        }
        // now simulate the keypresses
        if ((recordbuffer[recsize] == play.gamestep) && (recordbuffer[recsize + 1] == REC_KBHIT)) {
            done_playback_event (2);
            return 1;
        }
        return 0;
    }
    int result = keypressed();
    if ((result) && (globalTimerCounter < play.ignore_user_input_until_time))
    {
        // ignoring user input
        my_readkey();
        result = 0;
    }
    if ((result) && (play.recording)) {
        write_record_event (REC_KBHIT, 0, NULL);
    }
    return result;  
}

char playback_keystate[KEY_MAX];

int rec_iskeypressed (int keycode) {

    if (play.playback) {
        if ((recordbuffer[recsize] == play.gamestep)
            && (recordbuffer[recsize + 1] == REC_KEYDOWN)
            && (recordbuffer[recsize + 2] == keycode)) {
                playback_keystate[keycode] = recordbuffer[recsize + 3];
                done_playback_event (4);
        }
        return playback_keystate[keycode];
    }

    int toret = key[keycode];

    if (play.recording) {
        if (toret != playback_keystate[keycode]) {
            short buff[2] = {keycode, toret};
            write_record_event (REC_KEYDOWN, 2, buff);
            playback_keystate[keycode] = toret;
        }
    }

    return toret;
}

int rec_isSpeechFinished () {
    if (play.playback) {
        if ((recordbuffer[recsize] == play.gamestep) && (recordbuffer[recsize + 1] == REC_SPEECHFINISHED)) {
            done_playback_event (2);
            return 1;
        }
        return 0;
    }

    if (!channels[SCHAN_SPEECH]->done) {
        return 0;
    }
    if (play.recording)
        write_record_event (REC_SPEECHFINISHED, 0, NULL);
    return 1;
}

int recbutstate[4] = {-1, -1, -1, -1};
int rec_misbuttondown (int but) {
    if (play.playback) {
        if ((recordbuffer[recsize] == play.gamestep)
            && (recordbuffer[recsize + 1] == REC_MOUSEDOWN)
            && (recordbuffer[recsize + 2] == but)) {
                recbutstate[but] = recordbuffer[recsize + 3];
                done_playback_event (4);
        }
        return recbutstate[but];
    }
    int result = misbuttondown (but);
    if (play.recording) {
        if (result != recbutstate[but]) {
            short buff[2] = {but, result};
            write_record_event (REC_MOUSEDOWN, 2, buff);
            recbutstate[but] = result;
        }
    }
    return result;
}

int rec_mgetbutton() {

    if ((play.playback) && (recordbuffer != NULL)) {
        if ((recordbuffer[recsize] < play.gamestep) && (play.gamestep < 32766))
            quit("Playback error: out of sync");
        if (loopcounter >= replay_last_second + 40) {
            replay_time ++;
            replay_last_second += 40;
        }
        if ((recordbuffer[recsize] == play.gamestep) && (recordbuffer[recsize + 1] == REC_MOUSECLICK)) {
            filter->SetMousePosition(recordbuffer[recsize+3], recordbuffer[recsize+4]);
            disable_mgetgraphpos = 0;
            mgetgraphpos ();
            disable_mgetgraphpos = 1;
            int toret = recordbuffer[recsize + 2];
            done_playback_event (5);
            return toret;
        }
        return NONE;
    }

    int result;

    if (pluginSimulatedClick > NONE) {
        result = pluginSimulatedClick;
        pluginSimulatedClick = NONE;
    }
    else {
        result = mgetbutton();
    }

    if ((result >= 0) && (globalTimerCounter < play.ignore_user_input_until_time))
    {
        // ignoring user input
        result = NONE;
    }

    if (play.recording) {
        if (result >= 0) {
            short buff[3] = {result, mousex, mousey};
            write_record_event (REC_MOUSECLICK, 3, buff);
        }
        if (loopcounter >= replay_last_second + 40) {
            replay_time ++;
            replay_last_second += 40;
        }
    }
    return result;
}

void rec_domouse (int what) {

    if (play.recording) {
        int mxwas = mousex, mywas = mousey;
        if (what == DOMOUSE_NOCURSOR)
            mgetgraphpos();
        else
            domouse(what);

        if ((mxwas != mousex) || (mywas != mousey)) {
            // don't divide down the co-ordinates, because we lose
            // the precision, and it might click the wrong thing
            // if eg. hi-res 71 -> 35 in record file -> 70 in playback
            short buff[2] = {mousex, mousey};
            write_record_event (REC_MOUSEMOVE, 2, buff);
        }
        return;
    }
    else if ((play.playback) && (recordbuffer != NULL)) {
        if ((recordbuffer[recsize] == play.gamestep) && (recordbuffer[recsize + 1] == REC_MOUSEMOVE)) {
            filter->SetMousePosition(recordbuffer[recsize+2], recordbuffer[recsize+3]);
            disable_mgetgraphpos = 0;
            if (what == DOMOUSE_NOCURSOR)
                mgetgraphpos();
            else
                domouse(what);
            disable_mgetgraphpos = 1;
            done_playback_event (4);
            return;
        }
    }
    if (what == DOMOUSE_NOCURSOR)
        mgetgraphpos();
    else
        domouse(what);
}

int check_mouse_wheel () {
    if ((play.playback) && (recordbuffer != NULL)) {
        if ((recordbuffer[recsize] == play.gamestep) && (recordbuffer[recsize + 1] == REC_MOUSEWHEEL)) {
            int toret = recordbuffer[recsize+2];
            done_playback_event (3);
            return toret;
        }
        return 0;
    }

    int result = 0;
    if ((mouse_z != mouse_z_was) && (game.options[OPT_MOUSEWHEEL] != 0)) {
        if (mouse_z > mouse_z_was)
            result = 1;
        else
            result = -1;
        mouse_z_was = mouse_z;
    }

    if ((play.recording) && (result)) {
        short buff[1] = {result};
        write_record_event (REC_MOUSEWHEEL, 1, buff);
    }

    return result;
}

void start_recording() {
    if (play.playback) {
        play.recording = 0;  // stop quit() crashing
        play.playback = 0;
        quit("!playback and recording of replay selected simultaneously");
    }

    srand (play.randseed);
    play.gamestep = 0;

    recbuffersize = 10000;
    recordbuffer = (short*)malloc (recbuffersize * sizeof(short));
    recsize = 0;
    memset (playback_keystate, -1, KEY_MAX);
    replay_last_second = loopcounter;
    replay_time = 0;
    strcpy (replayfile, "New.agr");
}

void start_replay_record () {
    FILE *ott = fopen(replayTempFile, "wb");
    save_game_data (ott, NULL);
    fclose (ott);
    start_recording();
    play.recording = 1;
}

void stop_recording() {
    if (!play.recording)
        return;

    write_record_event (REC_ENDOFFILE, 0, NULL);

    play.recording = 0;
    char replaydesc[100] = "";
    sc_inputbox ("Enter replay description:", replaydesc);
    sc_inputbox ("Enter replay filename:", replayfile);
    if (replayfile[0] == 0)
        strcpy (replayfile, "Untitled");
    if (strchr (replayfile, '.') != NULL)
        strchr (replayfile, '.')[0] = 0;
    strcat (replayfile, ".agr");

    FILE *ooo = fopen(replayfile, "wb");
    fwrite ("AGSRecording", 12, 1, ooo);
    fputstring (ACI_VERSION_TEXT, ooo);
    int write_version = 2;
    FILE *fsr = fopen(replayTempFile, "rb");
    if (fsr != NULL) {
        // There was a save file created
        write_version = 3;
    }
    putw (write_version, ooo);

    fputstring (game.gamename, ooo);
    putw (game.uniqueid, ooo);
    putw (replay_time, ooo);
    fputstring (replaydesc, ooo);  // replay description, maybe we'll use this later
    putw (play.randseed, ooo);
    if (write_version >= 3)
        putw (recsize, ooo);
    fwrite (recordbuffer, recsize, sizeof(short), ooo);
    if (fsr != NULL) {
        putw (1, ooo);  // yes there is a save present
        int lenno = filelength(fileno(fsr));
        char *tbufr = (char*)malloc (lenno);
        fread (tbufr, lenno, 1, fsr);
        fwrite (tbufr, lenno, 1, ooo);
        free (tbufr);
        fclose (fsr);
        unlink (replayTempFile);
    }
    else if (write_version >= 3) {
        putw (0, ooo);
    }
    fclose (ooo);

    free (recordbuffer);
    recordbuffer = NULL;
}

void start_playback()
{
    FILE *in = fopen(replayfile, "rb");
    if (in != NULL) {
        char buffer [100];
        fread (buffer, 12, 1, in);
        buffer[12] = 0;
        if (strcmp (buffer, "AGSRecording") != 0) {
            Display("ERROR: Invalid recorded data file");
            play.playback = 0;
        }
        else {
            fgetstring_limit (buffer, in, 12);
            if (buffer[0] != '2') 
                quit("!Replay file is from an old version of AGS");
            if (strcmp (buffer, "2.55.553") < 0)
                quit("!Replay file was recorded with an older incompatible version");

            if (strcmp (buffer, ACI_VERSION_TEXT)) {
                // Disable text as speech while displaying the warning message
                // This happens if the user's graphics card does BGR order 16-bit colour
                int oldalways = game.options[OPT_ALWAYSSPCH];
                game.options[OPT_ALWAYSSPCH] = 0;
                play.playback = 0;
                Display("Warning! replay is from a different version of AGS (%s) - it may not work properly.", buffer);
                play.playback = 1;
                srand (play.randseed);
                play.gamestep = 0;
                game.options[OPT_ALWAYSSPCH] = oldalways;
            }

            int replayver = getw(in);

            if ((replayver < 1) || (replayver > 3))
                quit("!Unsupported Replay file version");

            if (replayver >= 2) {
                fgetstring_limit (buffer, in, 99);
                int uid = getw (in);
                if ((strcmp (buffer, game.gamename) != 0) || (uid != game.uniqueid)) {
                    char msg[150];
                    sprintf (msg, "!This replay is meant for the game '%s' and will not work correctly with this game.", buffer);
                    quit (msg);
                }
                // skip the total time
                getw (in);
                // replay description, maybe we'll use this later
                fgetstring_limit (buffer, in, 99);
            }

            play.randseed = getw(in);
            int flen = filelength(fileno(in)) - ftell (in);
            if (replayver >= 3) {
                flen = getw(in) * sizeof(short);
            }
            recordbuffer = (short*)malloc (flen);
            fread (recordbuffer, flen, 1, in);
            srand (play.randseed);
            recbuffersize = flen / sizeof(short);
            recsize = 0;
            disable_mgetgraphpos = 1;
            replay_time = 0;
            replay_last_second = loopcounter;
            if (replayver >= 3) {
                int issave = getw(in);
                if (issave) {
                    if (restore_game_data (in, replayfile))
                        quit("!Error running replay... could be incorrect game version");
                    replay_last_second = loopcounter;
                }
            }
            fclose (in);
        }
    }
    else // file not found
        play.playback = 0;
}

int my_readkey() {
    int gott=readkey();
    int scancode = ((gott >> 8) & 0x00ff);

    if (gott == READKEY_CODE_ALT_TAB)
    {
        // Alt+Tab, it gets stuck down unless we do this
        return AGS_KEYCODE_ALT_TAB;
    }

    /*  char message[200];
    sprintf(message, "Scancode: %04X", gott);
    OutputDebugString(message);*/

    /*if ((scancode >= KEY_0_PAD) && (scancode <= KEY_9_PAD)) {
    // fix numeric pad keys if numlock is off (allegro 4.2 changed this behaviour)
    if ((key_shifts & KB_NUMLOCK_FLAG) == 0)
    gott = (gott & 0xff00) | EXTENDED_KEY_CODE;
    }*/

    if ((gott & 0x00ff) == EXTENDED_KEY_CODE) {
        gott = scancode + 300;

        // convert Allegro KEY_* numbers to scan codes
        // (for backwards compatibility we can't just use the
        // KEY_* constants now, it's too late)
        if ((gott>=347) & (gott<=356)) gott+=12;
        // F11-F12
        else if ((gott==357) || (gott==358)) gott+=76;
        // insert / numpad insert
        else if ((scancode == KEY_0_PAD) || (scancode == KEY_INSERT))
            gott = AGS_KEYCODE_INSERT;
        // delete / numpad delete
        else if ((scancode == KEY_DEL_PAD) || (scancode == KEY_DEL))
            gott = AGS_KEYCODE_DELETE;
        // Home
        else if (gott == 378) gott = 371;
        // End
        else if (gott == 379) gott = 379;
        // PgUp
        else if (gott == 380) gott = 373;
        // PgDn
        else if (gott == 381) gott = 381;
        // left arrow
        else if (gott==382) gott=375;
        // right arrow
        else if (gott==383) gott=377;
        // up arrow
        else if (gott==384) gott=372;
        // down arrow
        else if (gott==385) gott=380;
        // numeric keypad
        else if (gott==338) gott=379;
        else if (gott==339) gott=380;
        else if (gott==340) gott=381;
        else if (gott==341) gott=375;
        else if (gott==342) gott=376;
        else if (gott==343) gott=377;
        else if (gott==344) gott=371;
        else if (gott==345) gott=372;
        else if (gott==346) gott=373;
    }
    else
        gott = gott & 0x00ff;

    // Alt+X, abort (but only once game is loaded)
    if ((gott == play.abort_key) && (displayed_room >= 0)) {
        check_dynamic_sprites_at_exit = 0;
        quit("!|");
    }

    //sprintf(message, "Keypress: %d", gott);
    //OutputDebugString(message);

    return gott;
}
