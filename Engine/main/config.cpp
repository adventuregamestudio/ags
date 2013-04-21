//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================

//
// Game configuration
//

#include "ac/spritecache.h"
#include "game/game_objects.h"
#include "main/mainheader.h"
#include "main/config.h"
#include "platform/base/override_defines.h" //_getcwd()
#include "util/filestream.h"
#include "util/textstreamreader.h"
#include "util/path.h"

using AGS::Common::Stream;
using AGS::Common::TextStreamReader;
using AGS::Common::String;

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
    Stream *fin = Common::File::OpenFileRead(filetouse);
    if (fin == NULL)
        return NULL;
    TextStreamReader reader(fin);

    //char templine[200];
    char wantsect[100];
    sprintf (wantsect, "[%s]", sectn);

    // NOTE: the string is used as a raw buffer down there;
    // FIXME that as soon as string class is optimized for common use
    String line;

    while (!reader.EOS()) {
        //fgets (templine, 199, fin);
        line = reader.ReadLine();

        // find the section
        if (strnicmp (wantsect, line.GetCStr(), strlen(wantsect)) == 0) {
            while (!reader.EOS()) {
                // we're in the right section, find the entry
                //fgets (templine, 199, fin);
                line = reader.ReadLine();
                if (line.IsEmpty())
                    continue;
                if (line[0] == '[')
                    break;
                // Have we found the entry?
                if (strnicmp (line.GetCStr(), entry, strlen(entry)) == 0) {
                    const char *pptr = &line.GetCStr()[strlen(entry)];
                    while ((pptr[0] == ' ') || (pptr[0] == '\t'))
                        pptr++;
                    if (pptr[0] == '=') {
                        pptr++;
                        while ((pptr[0] == ' ') || (pptr[0] == '\t'))
                            pptr++;
                        char *toret = (char*)malloc (strlen(pptr) + 5);
                        strcpy (toret, pptr);
                        return toret;
                    }
                }
            }
        }
    }
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
    if (!Common::File::TestReadFile(ac_config_file)) {

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
        // put the full path, or it gets written back to the Windows folder
        _getcwd (ac_config_file, 255);
        strcat (ac_config_file, "\\acsetup.cfg");
        fix_filename_case(ac_config_file);
        fix_filename_slashes(ac_config_file);
    }

    // set default dir if no config file
    usetup.DataFilesDir = ".";
    usetup.Translation.Empty();
#ifdef WINDOWS_VERSION
    usetup.DigitalSoundCard = DIGI_DIRECTAMX(0);
#endif

    // Don't read in the standard config file if disabled.
    if (psp_ignore_acsetup_cfg_file)
    {
        usetup.GfxDriverID = "DX5";
        usetup.EnableAntiAliasing = psp_gfx_smooth_sprites != 0;
        usetup.Translation = psp_translation;
        return;
    }

    if (Common::File::TestReadFile(ac_config_file)) {
        strcpy(filetouse,ac_config_file);
#ifndef WINDOWS_VERSION
        usetup.DigitalSoundCard=INIreadint("sound","digiid");
        usetup.MidiSoundCard=INIreadint("sound","midiid");
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
        usetup.DigitalSoundCard = idx;

        idx = INIreadint("sound","midiwinindx", 0);
        if (idx == 1)
            idx = MIDI_NONE;
        else if (idx == 2)
            idx = MIDI_WIN32MAPPER;
        else
            idx = MIDI_AUTODETECT;
        usetup.MidiSoundCard = idx;

        if (usetup.DigitalSoundCard < 0)
            usetup.DigitalSoundCard = DIGI_AUTODETECT;
        if (usetup.MidiSoundCard < 0)
            usetup.MidiSoundCard = MIDI_AUTODETECT;
#endif

        usetup.Windowed = INIreadint("misc","windowed");
        if (usetup.Windowed < 0)
            usetup.Windowed = 0;

        usetup.RefreshRate = INIreadint ("misc", "refresh", 0);
        int enable_anti_alias = INIreadint ("misc", "antialias", 0) != 0;
        usetup.EnableAntiAliasing = enable_anti_alias > 0;
        int no_true_color = INIreadint("misc", "notruecolor", 0);
        usetup.ForceHicolorMode = no_true_color > 0;
        int side_borders = INIreadint("misc", "sideborders", 0);
        usetup.EnableSideBorders = side_borders != 0;

#if defined(IOS_VERSION) || defined(PSP_VERSION) || defined(ANDROID_VERSION)
        // PSP: Letterboxing is not useful on the PSP.
        force_letterbox = 0;
#else
        force_letterbox = INIreadint ("misc", "forceletterbox", 0);
#endif

        // This option is backwards (usevox is 0 if no_speech_pack)
        int no_speech = INIreadint ("sound", "usespeech", 0) != 0;
        usetup.NoSpeechPack = no_speech == 0;

        usetup.DataFilesDir = INIreaditem("misc","datadir");
        if (usetup.DataFilesDir.IsEmpty())
            usetup.DataFilesDir = ".";
        // strip any trailing slash
        // TODO: move this to Path namespace later
        AGS::Common::Path::FixupPath(usetup.DataFilesDir);
#if defined (WINDOWS_VERSION)
        // if the path is just x:\ don't strip the slash
        if (!(usetup.DataFilesDir.GetLength() < 4 && usetup.DataFilesDir[1] == ':'))
        {
            usetup.DataFilesDir.TrimRight('/');
        }
#else
        usetup.DataFilesDir.TrimRight('/');
#endif
        usetup.MainDataFilename = INIreaditem ("misc", "datafile");

#if defined(IOS_VERSION) || defined(PSP_VERSION) || defined(ANDROID_VERSION)
        // PSP: No graphic filters are available.
        usetup.GfxFilterID.Empty();
#else
        char *gfx_filter = INIreaditem("misc", "gfxfilter");
        usetup.GfxFilterID = gfx_filter;
        free(gfx_filter);
#endif

#if defined (WINDOWS_VERSION)
        char *gfx_driver = INIreaditem("misc", "gfxdriver");
        usetup.GfxDriverID = gfx_driver;
        free(gfx_driver);
#else
        usetup.GfxDriverID = "DX5";
#endif

        char *translation = INIreaditem ("language", "translation");
        usetup.Translation = translation;
        free(translation);

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
            play.IsPlayback = 1;
        }
        else
            play.IsPlayback = 0;

    }

    if (usetup.GfxDriverID.IsEmpty())
        usetup.GfxDriverID = "DX5";

}
