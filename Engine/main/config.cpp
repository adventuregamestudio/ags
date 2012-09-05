/* Adventure Creator v2 Run-time engine
   Started 27-May-99 (c) 1999-2011 Chris Jones

  Adventure Game Studio source code Copyright 1999-2011 Chris Jones.
  All rights reserved.

  The AGS Editor Source Code is provided under the Artistic License 2.0
  http://www.opensource.org/licenses/artistic-license-2.0.php

  You MAY NOT compile your own builds of the engine without making it EXPLICITLY
  CLEAR that the code has been altered from the Standard Version.

*/

//
// Game configuration
//

#include "util/wgt2allg.h"
#include "ac/gamesetup.h"
#include "ac/gamestate.h"
#include "main/mainheader.h"
#include "main/config.h"
#include "ac/spritecache.h"
#include "platform/base/override_defines.h" //_getcwd()

extern GameSetup usetup;
extern int spritewidth[MAX_SPRITES],spriteheight[MAX_SPRITES];
extern SpriteCache spriteset;
extern int psp_video_framedrop;
extern int psp_audio_enabled;
extern int psp_midi_enabled;
extern int psp_ignore_acsetup_cfg_file;
extern int psp_clear_cache_on_room_change;
extern int psp_midi_preload_patches;
extern int psp_audio_cachesize;
extern char psp_game_file_name[];
extern int psp_gfx_smooth_sprites;
extern char psp_translation[];
extern int force_letterbox;
extern char replayfile[MAX_PATH];
extern GameState play;

//char datname[80]="ac.clb";
char ac_conf_file_defname[MAX_PATH] = "acsetup.cfg";
char *ac_config_file = &ac_conf_file_defname[0];
char conffilebuf[512];

char filetouse[MAX_PATH] = "nofile";

// Replace the filename part of complete path WASGV with INIFIL
void INIgetdirec(char *wasgv, char *inifil) {
    int u = strlen(wasgv) - 1;

    for (u = strlen(wasgv) - 1; u >= 0; u--) {
        if ((wasgv[u] == '\\') || (wasgv[u] == '/')) {
            memcpy(&wasgv[u + 1], inifil, strlen(inifil) + 1);
            break;
        }
    }

    if (u <= 0) {
        // no slashes - either the path is just "f:acwin.exe"
        if (strchr(wasgv, ':') != NULL)
            memcpy(strchr(wasgv, ':') + 1, inifil, strlen(inifil) + 1);
        // or it's just "acwin.exe" (unlikely)
        else
            strcpy(wasgv, inifil);
    }

}

char *INIreaditem(const char *sectn, const char *entry) {
    FILE *fin = fopen(filetouse, "rt");
    if (fin == NULL)
        return NULL;

    char templine[200];
    char wantsect[100];
    sprintf (wantsect, "[%s]", sectn);

    while (!feof(fin)) {
        fgets (templine, 199, fin);
        // find the section
        if (strnicmp (wantsect, templine, strlen(wantsect)) == 0) {
            while (!feof(fin)) {
                // we're in the right section, find the entry
                fgets (templine, 199, fin);
                if (templine[0] == '[')
                    break;
                if (feof(fin))
                    break;
                // Strip CRLF
                char *lastchar = &templine[strlen(templine) -1];
                while(*lastchar == '\r' || *lastchar == '\n') {
                    *lastchar = 0;
                    lastchar--;
                }
                // Have we found the entry?
                if (strnicmp (templine, entry, strlen(entry)) == 0) {
                    char *pptr = &templine[strlen(entry)];
                    while ((pptr[0] == ' ') || (pptr[0] == '\t'))
                        pptr++;
                    if (pptr[0] == '=') {
                        pptr++;
                        while ((pptr[0] == ' ') || (pptr[0] == '\t'))
                            pptr++;
                        char *toret = (char*)malloc (strlen(pptr) + 5);
                        strcpy (toret, pptr);
                        fclose (fin);
                        return toret;
                    }
                }
            }
        }
    }
    fclose (fin);
    return NULL;
}

int INIreadint (const char *sectn, const char *item, int errornosect = 1) {
    char *tempstr = INIreaditem (sectn, item);
    if (tempstr == NULL)
        return -1;

    int toret = atoi(tempstr);
    free (tempstr);
    return toret;
}

void read_config_file(char *argv0) {

    // Try current directory for config first; else try exe dir
    strcpy (ac_conf_file_defname, "acsetup.cfg");
    ac_config_file = &ac_conf_file_defname[0];
    FILE *ppp = fopen(ac_config_file, "rb");
    if (ppp == NULL) {

        strcpy(conffilebuf,argv0);

        /*    for (int ee=0;ee<(int)strlen(conffilebuf);ee++) {
        if (conffilebuf[ee]=='/') conffilebuf[ee]='\\';
        }*/
        fix_filename_case(conffilebuf);
        fix_filename_slashes(conffilebuf);

        INIgetdirec(conffilebuf,ac_config_file);
        //    printf("Using config: '%s'\n",conffilebuf);
        ac_config_file=&conffilebuf[0];
    }
    else {
        fclose(ppp);
        // put the full path, or it gets written back to the Windows folder
        _getcwd (ac_config_file, 255);
        strcat (ac_config_file, "\\acsetup.cfg");
        fix_filename_case(ac_config_file);
        fix_filename_slashes(ac_config_file);
    }

    // set default dir if no config file
    usetup.data_files_dir = ".";
    usetup.translation = NULL;
    usetup.main_data_filename = "ac2game.dat";
#ifdef WINDOWS_VERSION
    usetup.digicard = DIGI_DIRECTAMX(0);
#endif

    // Don't read in the standard config file if disabled.
    if (psp_ignore_acsetup_cfg_file)
    {
        usetup.gfxDriverID = "DX5";
        usetup.enable_antialiasing = psp_gfx_smooth_sprites;
        usetup.translation = psp_translation;
        return;
    }

    ppp=fopen(ac_config_file,"rt");
    if (ppp!=NULL) {
        strcpy(filetouse,ac_config_file);
        fclose(ppp);
#ifndef WINDOWS_VERSION
        usetup.digicard=INIreadint("sound","digiid");
        usetup.midicard=INIreadint("sound","midiid");
#else
        int idx = INIreadint("sound","digiwinindx", 0);
        if (idx == 0)
            idx = DIGI_DIRECTAMX(0);
        else if (idx == 1)
            idx = DIGI_WAVOUTID(0);
        else if (idx == 2)
            idx = DIGI_NONE;
        else if (idx == 3) 
            idx = DIGI_DIRECTX(0);
        else 
            idx = DIGI_AUTODETECT;
        usetup.digicard = idx;

        idx = INIreadint("sound","midiwinindx", 0);
        if (idx == 1)
            idx = MIDI_NONE;
        else if (idx == 2)
            idx = MIDI_WIN32MAPPER;
        else
            idx = MIDI_AUTODETECT;
        usetup.midicard = idx;

        if (usetup.digicard < 0)
            usetup.digicard = DIGI_AUTODETECT;
        if (usetup.midicard < 0)
            usetup.midicard = MIDI_AUTODETECT;
#endif

#if defined(WINDOWS_VERSION) || defined(LINUX_VERSION) || defined(MAC_VERSION)
        usetup.windowed = INIreadint("misc","windowed");
        if (usetup.windowed < 0)
            usetup.windowed = 0;
#endif

        usetup.refresh = INIreadint ("misc", "refresh", 0);
        usetup.enable_antialiasing = INIreadint ("misc", "antialias", 0);
        usetup.force_hicolor_mode = INIreadint("misc", "notruecolor", 0);
        usetup.enable_side_borders = INIreadint("misc", "sideborders", 0);

#if defined(IOS_VERSION) || defined(PSP_VERSION) || defined(ANDROID_VERSION)
        // PSP: Letterboxing is not useful on the PSP.
        force_letterbox = 0;
#else
        force_letterbox = INIreadint ("misc", "forceletterbox", 0);
#endif

        if (usetup.enable_antialiasing < 0)
            usetup.enable_antialiasing = 0;
        if (usetup.force_hicolor_mode < 0)
            usetup.force_hicolor_mode = 0;
        if (usetup.enable_side_borders < 0)
            usetup.enable_side_borders = 1;

        // This option is backwards (usevox is 0 if no_speech_pack)
        usetup.no_speech_pack = INIreadint ("sound", "usespeech", 0);
        if (usetup.no_speech_pack == 0)
            usetup.no_speech_pack = 1;
        else
            usetup.no_speech_pack = 0;

        usetup.data_files_dir = INIreaditem("misc","datadir");
        if (usetup.data_files_dir == NULL)
            usetup.data_files_dir = ".";
        // strip any trailing slash
#if defined(LINUX_VERSION) || defined(MAC_VERSION)
        if (usetup.data_files_dir[strlen(usetup.data_files_dir)-1] == '/')
            usetup.data_files_dir[strlen(usetup.data_files_dir)-1] = 0;
#else
        if ((strlen(usetup.data_files_dir) < 4) && (usetup.data_files_dir[1] == ':'))
        { }  // if the path is just  d:\  don't strip the slash
        else if (usetup.data_files_dir[strlen(usetup.data_files_dir)-1] == '\\')
            usetup.data_files_dir[strlen(usetup.data_files_dir)-1] = 0;
#endif

        usetup.main_data_filename = INIreaditem ("misc", "datafile");
        if (usetup.main_data_filename == NULL)
            usetup.main_data_filename = "ac2game.dat";

#if defined(IOS_VERSION) || defined(PSP_VERSION) || defined(ANDROID_VERSION)
        // PSP: No graphic filters are available.
        usetup.gfxFilterID = NULL;
#else
        usetup.gfxFilterID = INIreaditem("misc", "gfxfilter");
#endif

#if defined(LINUX_VERSION) || defined(MAC_VERSION)
        usetup.gfxDriverID = "DX5";
#else
        usetup.gfxDriverID = INIreaditem("misc", "gfxdriver");
#endif

        usetup.translation = INIreaditem ("language", "translation");

#if !defined(IOS_VERSION) && !defined(PSP_VERSION) && !defined(ANDROID_VERSION)
        // PSP: Don't let the setup determine the cache size as it is always too big.
        int tempint = INIreadint ("misc", "cachemax");
        if (tempint > 0)
            spriteset.maxCacheSize = tempint * 1024;
#endif

        char *repfile = INIreaditem ("misc", "replay");
        if (repfile != NULL) {
            strcpy (replayfile, repfile);
            free (repfile);
            play.playback = 1;
        }
        else
            play.playback = 0;

    }

    if (usetup.gfxDriverID == NULL)
        usetup.gfxDriverID = "DX5";

}
