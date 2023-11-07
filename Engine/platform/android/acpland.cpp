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

#if AGS_PLATFORM_OS_ANDROID
#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h> 
#include <unistd.h>
#include <SDL.h>
#include <allegro.h>
#include <jni.h>
#include <android/log.h>
#include <android/asset_manager_jni.h>
#include "platform/base/agsplatformdriver.h"
#include "ac/runtime_defines.h"
#include "game/main_game_file.h"
#include "platform/base/mobile_base.h"
#include "plugin/agsplugin.h"
#include "util/android_file.h"
#include "util/path.h"
#include "util/string_compat.h"
#include "util/file.h"

using namespace AGS::Common;

#define ANDROID_CONFIG_FILENAME "android.cfg"

struct AGSAndroid : AGSPlatformDriver
{
  void MainInit() override;
  void PostBackendExit() override;
  const char *GetGameDataFile() override;
  void ReadConfiguration(ConfigTree &cfg) override;
  void Delay(int millis) override;
  void DisplayAlert(const char*, ...) override;
  FSLocation GetAllUsersDataDirectory() override;
  FSLocation GetUserSavedgamesDirectory() override;
  FSLocation GetUserGlobalConfigDirectory() override;
  FSLocation GetAppOutputDirectory() override;
  unsigned long GetDiskFreeSpaceMB() override;
  eScriptSystemOSID GetSystemOSID() override;
  void WriteStdOut(const char *fmt, ...) override;
  void WriteStdErr(const char *fmt, ...) override;

  static MobileSetup &GetMobileSetup() { return _msetup; }

private:
  static MobileSetup _msetup; // static for the use from JNI callbacks
};

MobileSetup AGSAndroid::_msetup;


String android_base_directory = ".";
String android_app_directory = ".";
String android_save_directory = "";

// NOTE: the JVM can't use JNI outside here due to C++ name mangling
extern "C" 
{

const int CONFIG_IGNORE_ACSETUP = 0;
const int CONFIG_CLEAR_CACHE = 1;
const int CONFIG_AUDIO_ENABLED = 3;
const int CONFIG_AUDIO_CACHESIZE = 5;
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
const int CONFIG_MOUSE_EMULATION = 19;
const int CONFIG_MOUSE_METHOD = 20;
const int CONFIG_MOUSE_SPEED = 21;

JNIEXPORT jboolean JNICALL
  Java_uk_co_adventuregamestudio_runtime_PreferencesActivity_readConfigFile(JNIEnv* env, jobject object, jstring directory)
{
  const char* cdirectory = env->GetStringUTFChars(directory, NULL);
  String dir = cdirectory;
  env->ReleaseStringUTFChars(directory, cdirectory);

  String config_file_path = ANDROID_CONFIG_FILENAME;
  if(!dir.IsEmpty())
    config_file_path = Path::ConcatPaths(dir, config_file_path);

  ResetConfiguration(AGSAndroid::GetMobileSetup());

  return ReadConfiguration(AGSAndroid::GetMobileSetup(), config_file_path.GetCStr(), true);
}


JNIEXPORT jboolean JNICALL
  Java_uk_co_adventuregamestudio_runtime_PreferencesActivity_writeConfigFile(JNIEnv* env, jobject object, jstring directory)
{
  const char* cdirectory = env->GetStringUTFChars(directory, NULL);
  String dir = cdirectory;
  env->ReleaseStringUTFChars(directory, cdirectory);

  String config_file_path = ANDROID_CONFIG_FILENAME;
  if(!dir.IsEmpty())
    config_file_path = Path::ConcatPaths(dir, config_file_path);

  return WriteConfiguration(AGSAndroid::GetMobileSetup(), config_file_path.GetCStr());
}


JNIEXPORT jint JNICALL
  Java_uk_co_adventuregamestudio_runtime_PreferencesActivity_readIntConfigValue(JNIEnv* env,
                                                                                 jclass object, jint id)
{
  const auto &setup = AGSAndroid::GetMobileSetup();
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


JNIEXPORT jstring JNICALL
  Java_uk_co_adventuregamestudio_runtime_PreferencesActivity_readStringConfigValue(JNIEnv* env,
                                                                                    jclass object, jint id)
{
  const auto &setup = AGSAndroid::GetMobileSetup();
  switch (id)
  {
    case CONFIG_TRANSLATION:
      return env->NewStringUTF(setup.translation.GetCStr());
    default:
      return nullptr;
  }
}


JNIEXPORT void JNICALL
  Java_uk_co_adventuregamestudio_runtime_PreferencesActivity_setIntConfigValue(JNIEnv* env, jobject object, jint id, jint value)
{
  auto &setup = AGSAndroid::GetMobileSetup();
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


JNIEXPORT void JNICALL
  Java_uk_co_adventuregamestudio_runtime_PreferencesActivity_setStringConfigValue(JNIEnv* env, jobject object, jint id, jstring value)
{
  const char* cstring = env->GetStringUTFChars(value, NULL);

  auto &setup = AGSAndroid::GetMobileSetup();
  switch (id)
  {
    case CONFIG_TRANSLATION:
      setup.translation = cstring;
      break;
    default:
      break;
  }

  env->ReleaseStringUTFChars(value, cstring);
}


JNIEXPORT jint JNICALL
  Java_uk_co_adventuregamestudio_runtime_PreferencesActivity_getAvailableTranslations(JNIEnv* env, jobject object, jobjectArray translations)
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

JNIEXPORT void JNICALL
Java_uk_co_adventuregamestudio_runtime_AGSRuntimeActivity_nativeSdlShowKeyboard(JNIEnv* env, jclass clazz)
{
  SDL_StartTextInput();
}

} // END of Extern "C"


void AGSAndroid::MainInit()
{
    // retrieve the JNI environment.
    JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();

    // retrieve the Java instance of the SDLActivity
    jobject activity = (jobject)SDL_AndroidGetActivity();
    // find the Java class of the activity.
    jclass clazz(env->GetObjectClass(activity));

    auto &setup = AGSAndroid::GetMobileSetup();

    jfieldID fid_game_file_name = env->GetFieldID(clazz, "_game_file_name", "Ljava/lang/String;");
    jobject jobj_game_file_name = env->GetObjectField(activity, fid_game_file_name);
    const char * _game_file_name =
        env->GetStringUTFChars(static_cast<jstring>(jobj_game_file_name), nullptr);
    if(_game_file_name != nullptr) {
        setup.game_file_name = _game_file_name;
    }
    env->ReleaseStringUTFChars(static_cast<jstring>(jobj_game_file_name), _game_file_name);

    jfieldID fid_android_base_directory = env->GetFieldID(clazz, "_android_base_directory", "Ljava/lang/String;");
    jobject jobj_android_base_directory = env->GetObjectField(activity, fid_android_base_directory);
    const char * _android_base_directory =
        env->GetStringUTFChars(static_cast<jstring>(jobj_android_base_directory), nullptr);
    if(_android_base_directory != nullptr) {
        android_base_directory = _android_base_directory;
    }
    env->ReleaseStringUTFChars(static_cast<jstring>(jobj_android_base_directory), _android_base_directory);

    jfieldID fid_android_app_directory = env->GetFieldID(clazz, "_android_app_directory", "Ljava/lang/String;");
    jobject jobj_android_app_directory = env->GetObjectField(activity, fid_android_app_directory);
    const char * _android_app_directory =
        env->GetStringUTFChars(static_cast<jstring>(jobj_android_app_directory), nullptr);
    if(_android_app_directory != nullptr) {
        android_app_directory = _android_app_directory;
    }
    env->ReleaseStringUTFChars(static_cast<jstring>(jobj_android_app_directory), _android_app_directory);

    jfieldID fid_loadLastSave = env->GetFieldID(clazz, "_loadLastSave", "Z");
    jboolean jbool_loadLastSave = env->GetBooleanField(activity, fid_loadLastSave);
    bool loadLastSave = jbool_loadLastSave;

    // Reset configuration.
    ResetConfiguration(setup);

    // Read general configuration. (in single game package, this is the config read)
    String general_config_file = Path::ConcatPaths(android_base_directory, ANDROID_CONFIG_FILENAME);
    ::ReadConfiguration(setup, general_config_file.GetCStr(), true);

    // Get the games path.
    String path = setup.game_file_name;
    path = Path::GetDirectoryPath(path);
    if(path != "./" && path.GetLength() > 1) {
      // We are using AGS Player and passing a specific game

      chdir(path.GetCStr());
      // TODO: this is an ancient GUS patch fix for MIDI playback, is this still relevant?
      setenv("ULTRADIR", "..", 1);
    }

    // Read game specific configuration. (this does nothing in single game package)
    ::ReadConfiguration(setup, ANDROID_CONFIG_FILENAME, false);

    setup.load_latest_savegame = loadLastSave;

    // clean up the local references.
    env->DeleteLocalRef(activity);
    env->DeleteLocalRef(clazz);
}

bool IsValidAgsGame(const String& filename)
{
  return (((filename.CompareRightNoCase(".ags") == 0 ||
            filename.CompareNoCase("ac2game.dat") == 0 ||
            filename.CompareRightNoCase(".exe") == 0)) &&
          IsMainGameLibrary(filename));
}

const char *AGSAndroid::GetGameDataFile()
{
  AAssetManager* assetManager = GetAAssetManager();
  AAssetDir* aAssetDir = AAssetManager_openDir(assetManager,"");

  _msetup.game_file_name = "";

  for(String f = AAssetDir_getNextFileName(aAssetDir); !f.IsNullOrSpace(); f = AAssetDir_getNextFileName(aAssetDir))
  {
    if(IsValidAgsGame(f))
    {
      _msetup.game_file_name = f;
      break;
    }
  }
  AAssetDir_close(aAssetDir);
  return _msetup.game_file_name.GetCStr();
}

void AGSAndroid::ReadConfiguration(ConfigTree &cfg)
{
  ApplyEngineConfiguration(_msetup, cfg);
}
void AGSAndroid::DisplayAlert(const char *text, ...) {
  char displbuf[2000];
  va_list args;
  va_start(args, text);
  vsprintf(displbuf, text, args);
  SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "AGSNative",displbuf, nullptr);
  va_end(args);
}

void AGSAndroid::Delay(int millis) {
  usleep(millis * 1000);
}

unsigned long AGSAndroid::GetDiskFreeSpaceMB() {
  // placeholder
  return 100;
}

eScriptSystemOSID AGSAndroid::GetSystemOSID() {
  return eOS_Android;
}

void AGSAndroid::PostBackendExit() {
  ShutdownAndroidFile();
}

void AGSAndroid::WriteStdOut(const char *fmt, ...)
{
  // TODO: this check should probably be done once when setting up output targets for logging
  if (_msetup.debug_write_to_logcat)
  {
    va_list args;
    va_start(args, fmt);
    __android_log_vprint(ANDROID_LOG_DEBUG, "AGSNative", fmt, args);
    // NOTE: __android_log_* functions add trailing '\n'
    va_end(args);
  }
}

void AGSAndroid::WriteStdErr(const char *fmt, ...)
{
  // TODO: find out if Android needs separate implementation for stderr
  if (_msetup.debug_write_to_logcat)
  {
    va_list args;
    va_start(args, fmt);
    __android_log_vprint(ANDROID_LOG_DEBUG, "AGSNative", fmt, args);
    // NOTE: __android_log_* functions add trailing '\n'
    va_end(args);
  }
}

static void MakeGameSaveDirectory()
{
  // Test the app dir for write access, if failed then switch to "base dir"
  if (File::TestCreateFile("./tmptest.tmp"))
    android_save_directory = ".";
  else
    android_save_directory = android_base_directory;
}

FSLocation AGSAndroid::GetAllUsersDataDirectory()
{
  if (android_save_directory.IsEmpty())
    MakeGameSaveDirectory();
  return FSLocation(android_save_directory);
}

FSLocation AGSAndroid::GetUserSavedgamesDirectory()
{
  if (android_save_directory.IsEmpty())
    MakeGameSaveDirectory();
  return FSLocation(android_save_directory);
}

FSLocation AGSAndroid::GetUserGlobalConfigDirectory()
{
  return FSLocation(android_base_directory);
}

FSLocation AGSAndroid::GetAppOutputDirectory()
{
  return FSLocation(android_base_directory);
}

AGSPlatformDriver* AGSPlatformDriver::CreateDriver()
{
    return new AGSAndroid();
}

#endif
