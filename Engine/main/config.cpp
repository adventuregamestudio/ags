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

#include "ac/gamesetup.h"
#include "ac/gamestate.h"
#include "debug/debug_log.h"
#include "main/mainheader.h"
#include "main/config.h"
#include "ac/spritecache.h"
#include "platform/base/agsplatformdriver.h"
#include "platform/base/override_defines.h" //_getcwd()
#include "util/filestream.h"
#include "util/textstreamreader.h"
#include "util/path.h"
#include "util/string_utils.h"

using namespace AGS::Common;
using namespace AGS::Engine;

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

int INIreadint (const char *sectn, const char *item, int def_value = 0) {
    char *tempstr = INIreaditem (sectn, item);
    if (tempstr == NULL)
        return def_value;

    int toret = atoi(tempstr);
    free (tempstr);
    return toret;
}

String INIreadstring(const char *sectn, const char *entry, const char *def_value = "")
{
    char *tempstr = INIreaditem(sectn, entry);
    if (tempstr == NULL)
        return def_value;

    String str = tempstr;
    free(tempstr);
    return str;
}

uint32_t parse_scaling_factor(const String &scaling_option)
{
    if (scaling_option.CompareNoCase("max") == 0)
        return 0;
    int gfx_scaling = StrUtil::StringToInt(scaling_option);
    if (gfx_scaling >= 0)
        return gfx_scaling <<= kShift;
    else
        return kUnit / abs(gfx_scaling);
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
    usetup.data_files_dir = ".";
    usetup.translation = NULL;
#ifdef WINDOWS_VERSION
    usetup.digicard = DIGI_DIRECTAMX(0);
#endif

    // Don't read in the standard config file if disabled.
    if (psp_ignore_acsetup_cfg_file)
    {
        usetup.Screen.DriverID = "DX5";
        usetup.enable_antialiasing = psp_gfx_smooth_sprites != 0;
        usetup.translation = psp_translation;
        return;
    }

    if (Common::File::TestReadFile(ac_config_file)) {
        strcpy(filetouse,ac_config_file);
#ifndef WINDOWS_VERSION
        usetup.digicard=INIreadint("sound","digiid", DIGI_AUTODETECT);
        usetup.midicard=INIreadint("sound","midiid", MIDI_AUTODETECT);
#else
        int idx = INIreadint("sound","digiwinindx", -1);
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

        idx = INIreadint("sound","midiwinindx", -1);
        if (idx == 1)
            idx = MIDI_NONE;
        else if (idx == 2)
            idx = MIDI_WIN32MAPPER;
        else
            idx = MIDI_AUTODETECT;
        usetup.midicard = idx;
#endif
#if !defined (LINUX_VERSION)
        psp_audio_multithreaded = INIreadint("sound", "threaded", psp_audio_multithreaded);
#endif

        // Graphics mode
#if defined (WINDOWS_VERSION)
        usetup.Screen.DriverID = INIreadstring("graphics", "driver");
#else
        usetup.Screen.DriverID = "DX5";
#endif
        usetup.Screen.Windowed = INIreadint("graphics", "windowed") > 0;
        const char *screen_sz_def_options[kNumScreenDef] = { "explicit", "scaling", "max" };
        usetup.Screen.SizeDef = kScreenDef_MaxDisplay;
        String screen_sz_def_str = INIreadstring("graphics", "screen_def");
        for (int i = 0; i < kNumScreenDef; ++i)
        {
            if (screen_sz_def_str.CompareNoCase(screen_sz_def_options[i]) == 0)
            {
                usetup.Screen.SizeDef = (ScreenSizeDefinition)i;
                break;
            }
        }
            
        usetup.Screen.Size.Width = INIreadint("graphics", "screen_width");
        usetup.Screen.Size.Height = INIreadint("graphics", "screen_height");
        usetup.Screen.MatchDeviceRatio = INIreadint("graphics", "match_device_ratio", 1) != 0;
#if defined(IOS_VERSION) || defined(PSP_VERSION) || defined(ANDROID_VERSION)
        // PSP: No graphic filters are available.
        usetup.Screen.Filter.ID = "";
#else
        if (usetup.Screen.Filter.ID.IsEmpty())
        {
            usetup.Screen.Filter.ID = INIreadstring("graphics", "filter", "StdScale");
            String gfx_scaling_both, gfx_scaling_x, gfx_scaling_y;
            gfx_scaling_both = INIreadstring("graphics", "filter_scaling", "max");
            if (gfx_scaling_both.CompareNoCase("max") == 0)
            {
                usetup.Screen.Filter.MaxUniform = true;
                usetup.Screen.Filter.ScaleX = 0;
                usetup.Screen.Filter.ScaleY = 0;
            }
            else
            {
                gfx_scaling_x = INIreadstring("graphics", "filter_scaling_x", gfx_scaling_both);
                gfx_scaling_y = INIreadstring("graphics", "filter_scaling_y", gfx_scaling_both);
                usetup.Screen.Filter.MaxUniform = false;
                usetup.Screen.Filter.ScaleX = parse_scaling_factor(gfx_scaling_x);
                usetup.Screen.Filter.ScaleY = parse_scaling_factor(gfx_scaling_y);
            }
        }
#endif

        const char *game_frame_options[kNumRectPlacement] = { "offset", "center", "stretch", "proportional" };
        usetup.Screen.FramePlacement = kPlaceCenter;
        String game_frame_str = INIreadstring("graphics", "game_frame", "center");
        for (int i = 0; i < kNumRectPlacement; ++i)
        {
            if (game_frame_str.CompareNoCase(game_frame_options[i]) == 0)
            {
                usetup.Screen.FramePlacement = (RectPlacement)i;
                break;
            }
        }
        usetup.Screen.RefreshRate = INIreadint ("graphics", "refresh");
        usetup.Screen.VSync = INIreadint("graphics", "vsync") > 0;

        usetup.enable_antialiasing = INIreadint ("misc", "antialias") > 0;
        usetup.force_hicolor_mode = INIreadint("misc", "notruecolor") > 0;

        // This option is backwards (usevox is 0 if no_speech_pack)
        usetup.no_speech_pack = INIreadint ("sound", "usespeech", 1) == 0;

        usetup.data_files_dir = INIreadstring("misc","datadir");
        if (usetup.data_files_dir.IsEmpty())
            usetup.data_files_dir = ".";
        // strip any trailing slash
        // TODO: move this to Path namespace later
        AGS::Common::Path::FixupPath(usetup.data_files_dir);
#if defined (WINDOWS_VERSION)
        // if the path is just x:\ don't strip the slash
        if (!(usetup.data_files_dir.GetLength() < 4 && usetup.data_files_dir[1] == ':'))
        {
            usetup.data_files_dir.TrimRight('/');
        }
#else
        usetup.data_files_dir.TrimRight('/');
#endif
        usetup.main_data_filename = INIreadstring ("misc", "datafile");

        usetup.translation = INIreaditem ("language", "translation");

        // PSP: Don't let the setup determine the cache size as it is always too big.
#if !defined(IOS_VERSION) && !defined(PSP_VERSION) && !defined(ANDROID_VERSION)
        // the config file specifies cache size in KB, here we convert it to bytes
        spriteset.maxCacheSize = INIreadint ("misc", "cachemax", DEFAULTCACHESIZE / 1024) * 1024;
#endif

        char *repfile = INIreaditem ("misc", "replay");
        if (repfile != NULL) {
            strcpy (replayfile, repfile);
            free (repfile);
            play.playback = 1;
        }
        else
            play.playback = 0;

        usetup.override_multitasking = INIreadint("override", "multitasking", -1);
        String override_os = INIreadstring("override", "os");
        usetup.override_script_os = -1;
        if (override_os.CompareNoCase("dos") == 0)
        {
            usetup.override_script_os = eOS_DOS;
        }
        else if (override_os.CompareNoCase("win") == 0)
        {
            usetup.override_script_os = eOS_Win;
        }
        else if (override_os.CompareNoCase("linux") == 0)
        {
            usetup.override_script_os = eOS_Linux;
        }
        else if (override_os.CompareNoCase("mac") == 0)
        {
            usetup.override_script_os = eOS_Mac;
        }
        usetup.override_upscale = INIreadint("override", "upscale") > 0;

        // NOTE: at the moment AGS provide little means to determine whether an
        // option was overriden by command line, and since command line args
        // are applied first, we need to check if the option differs from
        // default before applying value from config file.
        if (!enable_log_file && !disable_log_file)
        {
            enable_log_file = INIreadint ("misc", "log") != 0;
        }
    }
    else if (usetup.Screen.Filter.ID.IsEmpty())
    {
        // No config file found
        usetup.Screen.Filter.ID = "StdScale";
    }

    if (usetup.Screen.DriverID.IsEmpty())
        usetup.Screen.DriverID = "DX5";

    // FIXME: this correction is needed at the moment because graphics driver
    // implementation requires some filter to be created anyway
    usetup.Screen.Filter.UserRequest = usetup.Screen.Filter.ID;
    if (usetup.Screen.Filter.ID.IsEmpty() || usetup.Screen.Filter.ID.CompareNoCase("none") == 0)
    {
        usetup.Screen.Filter.ID = "StdScale";
        usetup.Screen.Filter.MaxUniform = false;
        usetup.Screen.Filter.ScaleX = kUnit;
        usetup.Screen.Filter.ScaleY = kUnit;
    }
}
