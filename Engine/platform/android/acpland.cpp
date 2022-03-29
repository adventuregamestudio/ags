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

struct AGSAndroid : AGSPlatformDriver {

  virtual void MainInit();
  virtual void PostBackendExit();

  virtual const char *GetGameDataFile();
  virtual int  CDPlayerCommand(int cmdd, int datt);
  virtual void Delay(int millis);
  virtual void DisplayAlert(const char*, ...);
  virtual FSLocation GetAllUsersDataDirectory();
  virtual FSLocation GetUserSavedgamesDirectory();
  virtual FSLocation GetUserGlobalConfigDirectory();
  virtual FSLocation GetAppOutputDirectory();
  virtual unsigned long GetDiskFreeSpaceMB();
  virtual eScriptSystemOSID GetSystemOSID();
  virtual int  InitializeCDPlayer();
  virtual void ShutdownCDPlayer();
  virtual void WriteStdOut(const char *fmt, ...);
  virtual void WriteStdErr(const char *fmt, ...);
};


String android_base_directory = ".";
String android_app_directory = ".";
String android_save_directory = "";
char psp_game_file_name[256];
char* psp_game_file_name_pointer = psp_game_file_name;

// NOTE: the JVM can't use JNI outside here due to C++ name mangling
extern "C" 
{

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

JNIEXPORT jboolean JNICALL
  Java_uk_co_adventuregamestudio_runtime_PreferencesActivity_readConfigFile(JNIEnv* env, jobject object, jstring directory)
{
  const char* cdirectory = env->GetStringUTFChars(directory, NULL);
  chdir(cdirectory);
  env->ReleaseStringUTFChars(directory, cdirectory);

  ResetConfiguration();

  return ReadConfiguration(ANDROID_CONFIG_FILENAME, true);
}


JNIEXPORT jboolean JNICALL
  Java_uk_co_adventuregamestudio_runtime_PreferencesActivity_writeConfigFile(JNIEnv* env, jobject object)
{
  return WriteConfiguration(ANDROID_CONFIG_FILENAME);
}


JNIEXPORT jint JNICALL
  Java_uk_co_adventuregamestudio_runtime_PreferencesActivity_readIntConfigValue(JNIEnv* env,
                                                                                 jclass object, jint id)
{
  switch (id)
  {
    case CONFIG_IGNORE_ACSETUP:
      return psp_ignore_acsetup_cfg_file;
      break;
    case CONFIG_CLEAR_CACHE:
      return psp_clear_cache_on_room_change;
      break;
    case CONFIG_AUDIO_RATE:
      return psp_audio_samplerate;
      break;
    case CONFIG_AUDIO_ENABLED:
      return psp_audio_enabled;
      break;
    case CONFIG_AUDIO_THREADED:
      return psp_audio_multithreaded;
      break;
    case CONFIG_AUDIO_CACHESIZE:
      return psp_audio_cachesize;
      break;
    case CONFIG_MIDI_ENABLED:
      return psp_midi_enabled;
      break;
    case CONFIG_MIDI_PRELOAD:
      return psp_midi_preload_patches;
      break;
    case CONFIG_VIDEO_FRAMEDROP:
      return psp_video_framedrop;
      break;
    case CONFIG_GFX_RENDERER:
      return psp_gfx_renderer;
      break;
    case CONFIG_GFX_SMOOTHING:
      return psp_gfx_smoothing;
      break;
    case CONFIG_GFX_SCALING:
      return psp_gfx_scaling;
      break;
    case CONFIG_GFX_SS:
      return psp_gfx_super_sampling;
      break;
    case CONFIG_GFX_SMOOTH_SPRITES:
      return psp_gfx_smooth_sprites;
      break;
    case CONFIG_ROTATION:
      return psp_rotation;
      break;
    case CONFIG_ENABLED:
      return psp_config_enabled;
      break;
    case CONFIG_DEBUG_FPS:
      return (display_fps == 2) ? 1 : 0;
      break;
    case CONFIG_DEBUG_LOGCAT:
      return psp_debug_write_to_logcat;
      break;
    case CONFIG_MOUSE_METHOD:
      return config_mouse_control_mode;
      break;
    case CONFIG_MOUSE_LONGCLICK:
      return config_mouse_longclick;
      break;
    default:
      return 0;
      break;
  }
}


JNIEXPORT jstring JNICALL
  Java_uk_co_adventuregamestudio_runtime_PreferencesActivity_readStringConfigValue(JNIEnv* env,
                                                                                    jclass object, jint id)
{
  switch (id)
  {
    case CONFIG_TRANSLATION:
      return env->NewStringUTF(&psp_translation[0]);
      break;
  }
}


JNIEXPORT void JNICALL
  Java_uk_co_adventuregamestudio_runtime_PreferencesActivity_setIntConfigValue(JNIEnv* env, jobject object, jint id, jint value)
{
  switch (id)
  {
    case CONFIG_IGNORE_ACSETUP:
      psp_ignore_acsetup_cfg_file = value;
      break;
    case CONFIG_CLEAR_CACHE:
      psp_clear_cache_on_room_change = value;
      break;
    case CONFIG_AUDIO_RATE:
      psp_audio_samplerate = value;
      break;
    case CONFIG_AUDIO_ENABLED:
      psp_audio_enabled = value;
      break;
    case CONFIG_AUDIO_THREADED:
      psp_audio_multithreaded = value;
      break;
    case CONFIG_AUDIO_CACHESIZE:
      psp_audio_cachesize = value;
      break;
    case CONFIG_MIDI_ENABLED:
      psp_midi_enabled = value;
      break;
    case CONFIG_MIDI_PRELOAD:
      psp_midi_preload_patches = value;
      break;
    case CONFIG_VIDEO_FRAMEDROP:
      psp_video_framedrop = value;
      break;
    case CONFIG_GFX_RENDERER:
      psp_gfx_renderer = value;
      break;
    case CONFIG_GFX_SMOOTHING:
      psp_gfx_smoothing = value;
      break;
    case CONFIG_GFX_SCALING:
      psp_gfx_scaling = value;
      break;
    case CONFIG_GFX_SS:
      psp_gfx_super_sampling = value;
      break;
    case CONFIG_GFX_SMOOTH_SPRITES:
      psp_gfx_smooth_sprites = value;
      break;
    case CONFIG_ROTATION:
      psp_rotation = value;
      break;
    case CONFIG_ENABLED:
      psp_config_enabled = value;
      break;
    case CONFIG_DEBUG_FPS:
      display_fps = (value == 1) ? 2 : 0;
      break;
    case CONFIG_DEBUG_LOGCAT:
      psp_debug_write_to_logcat = value;
      break;
    case CONFIG_MOUSE_METHOD:
      config_mouse_control_mode = value;
      break;
    case CONFIG_MOUSE_LONGCLICK:
      config_mouse_longclick = value;
      break;
    default:
      break;
  }
}


JNIEXPORT void JNICALL
  Java_uk_co_adventuregamestudio_runtime_PreferencesActivity_setStringConfigValue(JNIEnv* env, jobject object, jint id, jstring value)
{
  const char* cstring = env->GetStringUTFChars(value, NULL);

  switch (id)
  {
    case CONFIG_TRANSLATION:
      strcpy(psp_translation, cstring);
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
          psp_translations[i] = (char*)malloc(strlen(buffer) + 1);
          strcpy(psp_translations[i], buffer);
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
Java_uk_co_adventuregamestudio_runtime_AGSRuntimeActivity_nativeSdlShowKeyboard(JNIEnv* env, jobject object, jobjectArray translations)
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

    jfieldID fid_game_file_name = env->GetFieldID(clazz, "_game_file_name", "Ljava/lang/String;");
    jobject jobj_game_file_name = env->GetObjectField(activity, fid_game_file_name);
    const char * _game_file_name =
        env->GetStringUTFChars(static_cast<jstring>(jobj_game_file_name), nullptr);
    if(_game_file_name != nullptr) {
        strcpy(psp_game_file_name, _game_file_name);
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
    ResetConfiguration();

    // Read general configuration.
    ReadConfiguration(ANDROID_CONFIG_FILENAME, true);

    // Get the games path.
    String path = psp_game_file_name;
    path = Path::GetDirectoryPath(path);
    if(path != "./" && path.GetLength() > 1) {
      chdir(path.GetCStr());
      setenv("ULTRADIR", "..", 1);
    } else {
      chdir(_android_base_directory);
    }

    // Read game specific configuration.
    ReadConfiguration(ANDROID_CONFIG_FILENAME, false);

    if (config_mouse_longclick > 0) {
        jmethodID method_id = env->GetMethodID(clazz, "AgsEnableLongclick", "()V");
        env->CallVoidMethod(activity, method_id);
    }

    psp_load_latest_savegame = loadLastSave;

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

const char * AGSAndroid::GetGameDataFile()
{
  AAssetManager* assetManager = GetAAssetManager();
  AAssetDir* aAssetDir = AAssetManager_openDir(assetManager,"");

  psp_game_file_name[0] = '\0';

  for(String f = AAssetDir_getNextFileName(aAssetDir); !f.IsNullOrSpace(); f = AAssetDir_getNextFileName(aAssetDir))
  {
    if(IsValidAgsGame(f))
    {
      strcpy(psp_game_file_name, f.GetCStr());
      break;
    }
  }
  AAssetDir_close(aAssetDir);
  return psp_game_file_name;
}

int AGSAndroid::CDPlayerCommand(int cmdd, int datt) {
  return 1;
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

int AGSAndroid::InitializeCDPlayer() {
  return 1;
}

void AGSAndroid::PostBackendExit() {
  ShutdownAndroidFile();
}

void AGSAndroid::WriteStdOut(const char *fmt, ...)
{
  // TODO: this check should probably be done once when setting up output targets for logging
  if (psp_debug_write_to_logcat)
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
  if (psp_debug_write_to_logcat)
  {
    va_list args;
    va_start(args, fmt);
    __android_log_vprint(ANDROID_LOG_DEBUG, "AGSNative", fmt, args);
    // NOTE: __android_log_* functions add trailing '\n'
    va_end(args);
  }
}

void AGSAndroid::ShutdownCDPlayer() {

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
