//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
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
#include <dirent.h>
#include <sys/stat.h> 
#include <ctype.h>

#include <allegro.h>
#include "platform/base/agsplatformdriver.h"
#include "platform/base/mobile_base.h"
#include "ac/runtime_defines.h"
#include "main/config.h"
#include "plugin/agsplugin.h"
#include "util/string_utils.h"

using namespace AGS::Common;

#define IOS_CONFIG_FILENAME "ios.cfg"

extern char* ios_document_directory;

extern int main(int argc,char*argv[]);

const int CONFIG_IGNORE_ACSETUP = 0;
const int CONFIG_CLEAR_CACHE = 1;
const int CONFIG_AUDIO_RATE = 2;
const int CONFIG_AUDIO_ENABLED = 3;
const int CONFIG_AUDIO_THREADED = 4;
const int CONFIG_AUDIO_CACHESIZE = 5;
const int CONFIG_MIDI_ENABLED = 6;
const int CONFIG_MIDI_PRELOAD = 7;
const int CONFIG_VIDEO_FRAMEDROP = 8;
const int CONFIG_GFX_RENDERER = 9;
const int CONFIG_GFX_SMOOTHING = 10;
const int CONFIG_GFX_SCALING = 11;
const int CONFIG_GFX_SS = 12;
const int CONFIG_ROTATION = 13;
const int CONFIG_ENABLED = 14;
const int CONFIG_DEBUG_FPS = 15;
const int CONFIG_GFX_SMOOTH_SPRITES = 16;
const int CONFIG_TRANSLATION = 17;
const int CONFIG_DEBUG_LOGCAT = 18;
const int CONFIG_MOUSE_METHOD = 19;
const int CONFIG_MOUSE_LONGCLICK = 20;



struct AGSIOS : AGSPlatformDriver
{
  void ReadConfiguration(ConfigTree &cfg) override;
  int  CDPlayerCommand(int cmdd, int datt) override;
  void DisplayAlert(const char*, ...) override;
  FSLocation GetAppOutputDirectory() override;
  unsigned long GetDiskFreeSpaceMB() override;
  eScriptSystemOSID GetSystemOSID() override;

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
    case CONFIG_AUDIO_RATE:
      return setup.audio_samplerate;
    case CONFIG_AUDIO_ENABLED:
      return setup.audio_enabled;
    case CONFIG_AUDIO_THREADED:
      return setup.audio_multithreaded;
    case CONFIG_AUDIO_CACHESIZE:
      return setup.audio_cachesize;
    case CONFIG_MIDI_ENABLED:
      return setup.midi_enabled;
    case CONFIG_MIDI_PRELOAD:
      return setup.midi_preload_patches;
    case CONFIG_VIDEO_FRAMEDROP:
      return setup.video_framedrop;
    case CONFIG_GFX_RENDERER:
      return setup.gfx_renderer;
    case CONFIG_GFX_SMOOTHING:
      return setup.gfx_smoothing;
    case CONFIG_GFX_SCALING:
      return setup.gfx_scaling;
    case CONFIG_GFX_SS:
      return setup.gfx_super_sampling;
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
    case CONFIG_MOUSE_METHOD:
      return setup.mouse_control_mode;
    case CONFIG_MOUSE_LONGCLICK:
      return setup.mouse_longclick;
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
    case CONFIG_AUDIO_RATE:
      setup.audio_samplerate = value;
      break;
    case CONFIG_AUDIO_ENABLED:
      setup.audio_enabled = value;
      break;
    case CONFIG_AUDIO_THREADED:
      setup.audio_multithreaded = value;
      break;
    case CONFIG_AUDIO_CACHESIZE:
      setup.audio_cachesize = value;
      break;
    case CONFIG_MIDI_ENABLED:
      setup.midi_enabled = value;
      break;
    case CONFIG_MIDI_PRELOAD:
      setup.midi_preload_patches = value;
      break;
    case CONFIG_VIDEO_FRAMEDROP:
      setup.video_framedrop = value;
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
    case CONFIG_GFX_SS:
      setup.gfx_super_sampling = value;
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
    case CONFIG_MOUSE_METHOD:
      setup.mouse_control_mode = value;
      break;
    case CONFIG_MOUSE_LONGCLICK:
      setup.mouse_longclick = value;
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

/*
int getAvailableTranslations(char* translations)
{
  int i = 0;
  int length;
  DIR* dir;
  struct dirent* entry;
  char buffer[200];

  dir = opendir(".");
  if (dir)
  {
    while ((entry = readdir(dir)) != 0)
    {
      length = strlen(entry->d_name);
      if (length > 4)
      {
        if (ags_stricmp(&entry->d_name[length - 4], ".tra") == 0)
        {
          memset(buffer, 0, 200);
          strncpy(buffer, entry->d_name, length - 4);
          env->SetObjectArrayElement(translations, i, env->NewStringUTF(&buffer[0]));
          i++;
        }
      }
    }
    closedir(dir);
  }

  return i;
}
*/

extern void ios_show_message_box(char* buffer);
volatile int ios_wait_for_ui = 0;


void startEngine(char* filename, char* directory, int loadLastSave)
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
  char path[256];
  strcpy(path, setup.game_file_name.GetCStr());
  int lastindex = strlen(path) - 1;
  while (path[lastindex] != '/')
  {
    path[lastindex] = 0;
    lastindex--;
  }
  chdir(path);
  
  setenv("ULTRADIR", "..", 1);

  // Read game specific configuration.
  ReadConfiguration(IOS_CONFIG_FILENAME, false);

  setup.load_latest_savegame = loadLastSave;

  // Start the engine main function.
  main(0, nullptr);
  
  // Explicitly quit here, otherwise the app will hang forever.
  exit(0);
}

const char *AGSIOS::GetGameDataFile()
{
  return _msetup.game_file_name.GetCStr();
}

void AGSIOS::ReadConfiguration(Common::ConfigTree &cfg)
{
  ApplyEngineConfiguration(_msetup, cfg);
}
void AGSIOS::DisplayAlert(const char *text, ...) {
  char displbuf[2000];
  va_list ap;
  va_start(ap, text);
  vsprintf(displbuf, text, ap);
  va_end(ap);
  printf("%s", displbuf);
  ios_show_message_box(displbuf);
  
  while (ios_wait_for_ui)
    usleep(200);
}

void AGSIOS::Delay(int millis) {
  usleep(millis);
}

unsigned long AGSIOS::GetDiskFreeSpaceMB() {
  // placeholder
  return 100;
}

eScriptSystemOSID AGSIOS::GetSystemOSID() {
  return eOS_iOS;
}

FSLocation AGSIOS::GetAppOutputDirectory()
{
  return FSLocation(ios_document_directory);
}

AGSPlatformDriver* AGSPlatformDriver::CreateDriver()
{
    return new AGSIOS();
}

#endif
