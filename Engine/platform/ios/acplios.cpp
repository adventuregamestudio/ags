//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "core/platform.h"

#if AGS_PLATFORM_OS_IOS
#include <stdio.h>
#include <ctype.h>
#include <SDL.h>
#include <allegro.h>
#include "platform/base/agsplatformdriver.h"
#include "platform/base/mobile_base.h"
#include "ac/gamesetup.h"
#include "ac/runtime_defines.h"
#include "main/config.h"
#include "util/directory.h"
#include "util/file.h"
#include "util/path.h"
#include "util/string_compat.h"
#include "util/string_utils.h"


using namespace AGS::Common;

#define IOS_CONFIG_FILENAME "ios.cfg"

String ios_log_directory = "";
String ios_save_directory = "";

extern int main(int argc,char*argv[]);

const int CONFIG_IGNORE_ACSETUP = 0;
const int CONFIG_CLEAR_CACHE = 1;
const int CONFIG_AUDIO_ENABLED = 3;
const int CONFIG_AUDIO_CACHESIZE = 5;
const int CONFIG_GFX_RENDERER = 9;
const int CONFIG_GFX_SMOOTHING = 10;
const int CONFIG_GFX_SCALING = 11;
const int CONFIG_ROTATION = 13;
const int CONFIG_ENABLED = 14;
const int CONFIG_DEBUG_FPS = 15;
const int CONFIG_GFX_SMOOTH_SPRITES = 16;
const int CONFIG_TRANSLATION = 17;
const int CONFIG_DEBUG_LOGCAT = 18;
const int CONFIG_MOUSE_EMULATION = 19;
const int CONFIG_MOUSE_METHOD = 20;
const int CONFIG_MOUSE_SPEED = 21;


struct AGSIOS : AGSPlatformDriver {
  void MainInit() override;
  const char *GetGameDataFile() override;
  void ReadConfiguration(ConfigTree &cfg) override;
  int  CDPlayerCommand(int cmdd, int datt) override;
  void Delay(int millis) override;
  FSLocation GetAllUsersDataDirectory() override;
  FSLocation GetUserSavedgamesDirectory() override;
  FSLocation GetUserGlobalConfigDirectory() override;
  FSLocation GetAppOutputDirectory() override;
  uint64_t GetDiskFreeSpaceMB(const String &path) override;
  eScriptSystemOSID GetSystemOSID() override;
  int  InitializeCDPlayer() override;
  void ShutdownCDPlayer() override;

  static MobileSetup &GetMobileSetup() { return _msetup; }

private:
  static MobileSetup _msetup; // static for the use from global callbacks
};

MobileSetup AGSIOS::_msetup;


bool readConfigFile(const char* directory)
{
  chdir(directory);

  ResetConfiguration(AGSIOS::GetMobileSetup());

  return ReadConfiguration(AGSIOS::GetMobileSetup(), IOS_CONFIG_FILENAME, true);
}


bool writeConfigFile()
{
  return WriteConfiguration(AGSIOS::GetMobileSetup(), IOS_CONFIG_FILENAME);
}


int readIntConfigValue(int id)
{
  const auto &setup = AGSIOS::GetMobileSetup();
  switch (id)
  {
      case CONFIG_IGNORE_ACSETUP:
        return setup.ignore_acsetup_cfg_file;
      case CONFIG_CLEAR_CACHE:
        return setup.clear_cache_on_room_change;
      case CONFIG_AUDIO_ENABLED:
        return setup.audio_enabled;
      case CONFIG_AUDIO_CACHESIZE:
        return setup.audio_cachesize;
      case CONFIG_GFX_RENDERER:
        return setup.gfx_renderer;
      case CONFIG_GFX_SMOOTHING:
        return setup.gfx_smoothing;
      case CONFIG_GFX_SCALING:
        return setup.gfx_scaling;
      case CONFIG_GFX_SMOOTH_SPRITES:
        return setup.gfx_smooth_sprites;
      case CONFIG_ROTATION:
        return setup.rotation;
      case CONFIG_ENABLED:
        return setup.config_enabled;
      case CONFIG_DEBUG_FPS:
        return setup.show_fps;
      case CONFIG_DEBUG_LOGCAT:
        return setup.debug_write_to_logcat;
      case CONFIG_MOUSE_EMULATION:
        return setup.mouse_emulation;
      case CONFIG_MOUSE_METHOD:
        return setup.mouse_control_mode;
      case CONFIG_MOUSE_SPEED:
        return setup.mouse_speed;
      default:
        return 0;
  }
}


const char* readStringConfigValue(int id)
{
  const auto &setup = AGSIOS::GetMobileSetup();
  switch (id)
  {
    case CONFIG_TRANSLATION:
      return setup.translation.GetCStr();
    default:
      return nullptr;
  }
}


void setIntConfigValue(int id, int value)
{
  auto &setup = AGSIOS::GetMobileSetup();
  switch (id)
  {

      case CONFIG_IGNORE_ACSETUP:
        setup.ignore_acsetup_cfg_file = value;
        break;
      case CONFIG_CLEAR_CACHE:
        setup.clear_cache_on_room_change = value;
        break;
      case CONFIG_AUDIO_ENABLED:
        setup.audio_enabled = value;
        break;
      case CONFIG_AUDIO_CACHESIZE:
        setup.audio_cachesize = value;
        break;
      case CONFIG_GFX_RENDERER:
        setup.gfx_renderer = value;
        break;
      case CONFIG_GFX_SMOOTHING:
        setup.gfx_smoothing = value;
        break;
      case CONFIG_GFX_SCALING:
        setup.gfx_scaling = value;
        break;
      case CONFIG_GFX_SMOOTH_SPRITES:
        setup.gfx_smooth_sprites = value;
        break;
      case CONFIG_ROTATION:
        setup.rotation = value;
        break;
      case CONFIG_ENABLED:
        setup.config_enabled = value;
        break;
      case CONFIG_DEBUG_FPS:
        setup.show_fps = value;
        break;
      case CONFIG_DEBUG_LOGCAT:
        setup.debug_write_to_logcat = value;
        break;
      case CONFIG_MOUSE_EMULATION:
        setup.mouse_emulation = value;
        break;
      case CONFIG_MOUSE_METHOD:
        setup.mouse_control_mode = value;
      case CONFIG_MOUSE_SPEED:
        setup.mouse_speed = value;
        break;
      default:
        break;
  }
}


void setStringConfigValue(int id, const char* value)
{
  auto &setup = AGSIOS::GetMobileSetup();
  switch (id)
  {
    case CONFIG_TRANSLATION:
      setup.translation = value;
      break;
    default:
      break;
  }
}

int getAvailableTranslations(char* translations)
{
    int count = 0;
    for (FindFile ff = FindFile::OpenFiles(".", "*.tra"); !ff.AtEnd(); ff.Next())
    {
        String filename = Path::RemoveExtension(Path::GetFilename(ff.Current()));
        // FIXME: figure out how to pass the string to iOS back!
        //env->SetObjectArrayElement(translations, count++, env->NewStringUTF(filename.GetCStr()));
    }
    return count;
}


void startEngine(const char* filename, const char* directory, int loadLastSave)
{
    auto &setup = AGSIOS::GetMobileSetup();
    setup.game_file_name = filename;

    // Get the base directory (usually "/sdcard/ags").
    chdir(directory);

    // Reset configuration.
    ResetConfiguration(setup);

    // Read general configuration.
    ReadConfiguration(setup, IOS_CONFIG_FILENAME, true);

    // Get the games path.
    String gamepath = Path::GetParent(setup.game_file_name);
    chdir(gamepath.GetCStr());

    setenv("ULTRADIR", "..", 1);

    // Read game specific configuration.
    // readConfigFile(".");

    setup.load_latest_savegame = loadLastSave;

    // Start the engine main function.
    main(0, nullptr);

    // Explicitly quit here, otherwise the app will hang forever.
    exit(0);
}

static void MakeGameSaveDirectory()
{
    String gamename = Path::RemoveExtension(Path::GetFilename(usetup.MainDataFile));
    if(gamename.IsNullOrSpace() || gamename.IsEmpty()) {
        gamename = "save";
    }
    String prefpath = SDL_GetPrefPath("AdventureGameStudio","AGS");
    ios_save_directory = Path::ConcatPaths(prefpath, gamename);
    ios_log_directory = Path::ConcatPaths(prefpath, "log");
    Directory::CreateAllDirectories(prefpath, gamename);
    Directory::CreateAllDirectories(prefpath, "log");
}


void AGSIOS::MainInit()
{
    auto &setup = AGSIOS::GetMobileSetup();
    
    MakeGameSaveDirectory();
    
    // Reset configuration.
    ResetConfiguration(setup);

    // Read general configuration.
    readConfigFile(SDL_GetBasePath());
    
}


const char *AGSIOS::GetGameDataFile()
{
  return _msetup.game_file_name.GetCStr();
}

void AGSIOS::ReadConfiguration(Common::ConfigTree &cfg)
{
  ApplyEngineConfiguration(_msetup, cfg);
}

int AGSIOS::CDPlayerCommand(int cmdd, int datt) {
  return 0;//cd_player_control(cmdd, datt);
}

void AGSIOS::Delay(int millis) {
  SDL_Delay(millis);
}

uint64_t AGSIOS::GetDiskFreeSpaceMB(const String &path) {
  // placeholder
  return 100;
}

eScriptSystemOSID AGSIOS::GetSystemOSID() {
  return eOS_iOS;
}

int AGSIOS::InitializeCDPlayer() {
  return 0;//cd_player_init();
}

void AGSIOS::ShutdownCDPlayer() {
  //cd_exit();
}

FSLocation AGSIOS::GetAllUsersDataDirectory()
{
  return FSLocation(ios_save_directory);
}

FSLocation AGSIOS::GetUserSavedgamesDirectory()
{
  return FSLocation(ios_save_directory);
}

FSLocation AGSIOS::GetUserGlobalConfigDirectory()
{
  return FSLocation(SDL_GetBasePath());
}


FSLocation AGSIOS::GetAppOutputDirectory()
{
  return FSLocation(ios_log_directory);
}

AGSPlatformDriver* AGSPlatformDriver::CreateDriver()
{
    return new AGSIOS();
}

#endif
