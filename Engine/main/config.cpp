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
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "debug/debug_log.h"
#include "main/mainheader.h"
#include "main/config.h"
#include "ac/spritecache.h"
#include "platform/base/agsplatformdriver.h"
#include "platform/base/override_defines.h" //_getcwd()
#include "util/directory.h"
#include "util/filestream.h"
#include "util/ini_util.h"
#include "util/textstreamreader.h"
#include "util/path.h"

using namespace AGS::Common;

extern GameSetup usetup;
extern GameSetupStruct game;
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
const char *ac_conf_file_defname = "acsetup.cfg";
String ac_config_file;

// Replace the filename part of complete path WASGV with INIFIL
void INIgetdirec(char *wasgv, const char *inifil) {
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

bool INIreaditem(const ConfigTree &cfg, const String &sectn, const String &item, String &value)
{
    ConfigNode sec_it = cfg.find(sectn);
    if (sec_it != cfg.end())
    {
        StrStrIter item_it = sec_it->second.find(item);
        if (item_it != sec_it->second.end())
        {
            value = item_it->second;
            return true;
        }
    }
    return false;
}

int INIreadint(const ConfigTree &cfg, const String &sectn, const String &item)
{
    String str;
    if (!INIreaditem(cfg, sectn, item, str))
        return -1;

    return atoi(str);
}

float INIreadfloat(const ConfigTree &cfg, const String &sectn, const String &item, float def_value)
{
    String str;
    if (!INIreaditem(cfg, sectn, item, str))
        return def_value;

    return atof(str);
}

String INIreadstring(const ConfigTree &cfg, const String &sectn, const String &item, const String &def_value)
{
    String str;
    if (!INIreaditem(cfg, sectn, item, str))
        return def_value;
    return str;
}

void INIwritestring(ConfigTree &cfg, const String &sectn, const String &item, const String &value)
{
    cfg[sectn][item] = value;
}

void find_default_cfg_file(const char *alt_cfg_file)
{
    // Try current directory for config first; else try exe dir
    ac_config_file = ac_conf_file_defname;
    if (!Common::File::TestReadFile(ac_config_file))
    {
        char conffilebuf[512];
        strcpy(conffilebuf, alt_cfg_file);

        /*    for (int ee=0;ee<(int)strlen(conffilebuf);ee++) {
        if (conffilebuf[ee]=='/') conffilebuf[ee]='\\';
        }*/
        fix_filename_case(conffilebuf);
        fix_filename_slashes(conffilebuf);

        INIgetdirec(conffilebuf, ac_config_file);
        //    printf("Using config: '%s'\n",conffilebuf);
        ac_config_file = conffilebuf;
    }
    else {
        // put the full path, or it gets written back to the Windows folder
        ac_config_file = Directory::GetCurrentDirectory();
        ac_config_file.Append("/acsetup.cfg");
        Path::FixupPath(ac_config_file);
    }
}

void find_user_cfg_file()
{
    String parent_dir = MakeSpecialSubDir(PathOrCurDir(platform->GetUserConfigDirectory()));
    ac_config_file = String::FromFormat("%s/%s", parent_dir.GetCStr(), ac_conf_file_defname);
}

void config_defaults()
{
    // set default dir if no config file
    usetup.data_files_dir = ".";
#ifdef WINDOWS_VERSION
    usetup.digicard = DIGI_DIRECTAMX(0);
#endif
}

void read_config(const ConfigTree &cfg)
{
    {
#ifndef WINDOWS_VERSION
        usetup.digicard=INIreadint(cfg, "sound","digiid");
        usetup.midicard=INIreadint(cfg, "sound","midiid");
#else
        int idx = INIreadint(cfg, "sound","digiwinindx");
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

        idx = INIreadint(cfg, "sound","midiwinindx");
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

#if !defined (LINUX_VERSION)
        int threaded_audio = INIreadint(cfg, "sound", "threaded");
        if (threaded_audio >= 0)
            psp_audio_multithreaded = threaded_audio;
#endif

        usetup.windowed = INIreadint(cfg, "misc", "windowed") > 0;

        usetup.refresh = INIreadint (cfg, "misc", "refresh");
        usetup.enable_antialiasing = INIreadint (cfg, "misc", "antialias") > 0;
        usetup.force_hicolor_mode = INIreadint(cfg, "misc", "notruecolor") > 0;
        usetup.prefer_sideborders = INIreadint(cfg, "misc", "prefer_sideborders") != 0;

#if defined(IOS_VERSION) || defined(PSP_VERSION) || defined(ANDROID_VERSION)
        // PSP: Letterboxing is not useful on the PSP.
        usetup.prefer_letterbox = false;
#else
        usetup.prefer_letterbox = INIreadint (cfg, "misc", "prefer_letterbox") != 0;
#endif

        // This option is backwards (usevox is 0 if no_speech_pack)
        usetup.no_speech_pack = INIreadint(cfg, "sound", "usespeech") == 0;

        usetup.data_files_dir = INIreadstring(cfg, "misc","datadir");
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
        usetup.main_data_filename = INIreadstring(cfg, "misc", "datafile");
        usetup.user_data_dir = INIreadstring(cfg, "misc", "user_data_dir");

#if defined(IOS_VERSION) || defined(PSP_VERSION) || defined(ANDROID_VERSION)
        // PSP: No graphic filters are available.
        usetup.gfxFilterID = "";
#else
        usetup.gfxFilterID = INIreadstring(cfg, "misc", "gfxfilter");
#endif

#if defined (WINDOWS_VERSION)
        usetup.gfxDriverID = INIreadstring(cfg, "misc", "gfxdriver");
#else
        usetup.gfxDriverID = "DX5";
#endif

        usetup.translation = INIreadstring(cfg, "language", "translation");

#if !defined(IOS_VERSION) && !defined(PSP_VERSION) && !defined(ANDROID_VERSION)
        // PSP: Don't let the setup determine the cache size as it is always too big.
        int tempint = INIreadint(cfg, "misc", "cachemax");
        if (tempint > 0)
            spriteset.maxCacheSize = tempint * 1024;
#endif

        String repfile = INIreadstring(cfg, "misc", "replay");
        if (repfile != NULL) {
            strcpy (replayfile, repfile);
            play.playback = 1;
        }
        else
            play.playback = 0;

        usetup.mouse_auto_lock = INIreadint(cfg, "mouse", "auto_lock") > 0;

        usetup.mouse_speed = INIreadfloat(cfg, "mouse", "speed", 1.f);
        if (usetup.mouse_speed <= 0.f)
            usetup.mouse_speed = 1.f;
        const char *mouse_speed_options[kNumMouseSpeedDefs] = { "absolute", "current_display" };
        String mouse_str = INIreadstring(cfg, "mouse", "speed_def", "current_display");
        for (int i = 0; i < kNumMouseSpeedDefs; ++i)
        {
            if (mouse_str.CompareNoCase(mouse_speed_options[i]) == 0)
                usetup.mouse_speed_def = (MouseSpeedDef)i;
        }

        usetup.override_multitasking = INIreadint(cfg, "override", "multitasking");
        String override_os = INIreadstring(cfg, "override", "os");
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
        usetup.override_upscale = INIreadint(cfg, "override", "upscale") > 0;

        // NOTE: at the moment AGS provide little means to determine whether an
        // option was overriden by command line, and since command line args
        // are applied first, we need to check if the option differs from
        // default before applying value from config file.
        if (!enable_log_file && !disable_log_file)
        {
            int log_value = INIreadint (cfg, "misc", "log");
            if (log_value >= 0)
                enable_log_file = log_value > 0;
        }
    }
}

void post_config()
{
    if (usetup.gfxDriverID.IsEmpty())
        usetup.gfxDriverID = "DX5";
}

void load_default_config_file(ConfigTree &cfg, const char *alt_cfg_file)
{
    config_defaults();

    // Don't read in the standard config file if disabled.
    if (psp_ignore_acsetup_cfg_file)
    {
        usetup.gfxDriverID = "DX5";
        usetup.enable_antialiasing = psp_gfx_smooth_sprites != 0;
        usetup.translation = psp_translation;
        return;
    }

    find_default_cfg_file(alt_cfg_file);
    IniUtil::Read(ac_config_file, cfg);
}

void load_user_config_file(AGS::Common::ConfigTree &cfg)
{
    String def_cfg_file = ac_config_file;
    find_user_cfg_file();
    if (def_cfg_file.Compare(ac_config_file) != 0)
        IniUtil::Read(ac_config_file, cfg);
}

void save_config_file()
{
    ConfigTree cfg;

    if (Mouse::IsControlEnabled())
        cfg["mouse"]["speed"] = String::FromFormat("%f", Mouse::GetSpeed());

    IniUtil::Merge(ac_config_file, cfg);
}
