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
#include "main/config.h"
#include "plugin/agsplugin.h"
#include "util/android_file.h"
#include "util/path.h"
#include "util/string_compat.h"
#include "util/file.h"

using namespace AGS::Common;

#define ANDROID_CONFIG_FILENAME "android.cfg"

bool ReadConfiguration(const char* filename, bool read_everything);
void ResetConfiguration();

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


//int psp_return_to_menu = 1;
int psp_ignore_acsetup_cfg_file = 1;
int psp_clear_cache_on_room_change = 0;
int psp_rotation = 0;
int psp_config_enabled = 0;
char psp_translation[100];
char* psp_translations[100];

// Mouse option
int config_mouse_control_mode;


// Graphic options
int psp_gfx_scaling;
int psp_gfx_smoothing;


// Audio options from the Allegro library.
unsigned int psp_audio_samplerate = 44100;
int psp_audio_enabled = 1;
volatile int psp_audio_multithreaded = 1;
int psp_audio_cachesize = 10;
int psp_midi_enabled = 1;
int psp_midi_preload_patches = 0;

int psp_video_framedrop = 0;

int psp_gfx_renderer = 0;
int psp_gfx_super_sampling = 0;
int psp_gfx_smooth_sprites = 0;

int psp_debug_write_to_logcat = 1;

int config_mouse_longclick = 0;

extern int display_fps;
extern int want_exit;
extern void PauseGame();
extern void UnPauseGame();
//extern int main(int argc,char*argv[]);

String android_base_directory = ".";;
String android_app_directory = ".";;
String android_save_directory = ".";;
char psp_game_file_name[256];
char* psp_game_file_name_pointer = psp_game_file_name;

bool psp_load_latest_savegame = false;
extern char saveGameDirectory[260];
extern const char *loadSaveGameOnStartup;
char lastSaveGameName[200];


JavaVM* android_jni_vm;
JNIEnv *java_environment;
jobject java_object;
jclass java_class;
jmethodID java_messageCallback;
jmethodID java_blockExecution;
jmethodID java_swapBuffers;
jmethodID java_setRotation;
jmethodID java_enableLongclick;

bool reset_configuration = false;

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


JNIEXPORT jstring JNICALL
  Java_com_bigbluecup_android_EngineGlue_findGameDataInDirectory(JNIEnv* env, jclass klass, jstring path)
{
  int tmp_errno = 0;
  int *orig_allegro_errno;
  auto path_c = env->GetStringUTFChars(path, NULL);
  auto path_str = String(path_c);

  // We have to configure our own allegro_errno because
  // - we call al_find* during the search
  // - allegro might not have been initialised yet

  orig_allegro_errno = allegro_errno;
  allegro_errno = &tmp_errno;

  auto result_str = FindGameData(path_str);
  
  allegro_errno = orig_allegro_errno;

  if (result_str.GetLength() == 0) { return NULL; }

  auto result_jni = env->NewStringUTF(result_str.GetCStr());
  return result_jni;
}


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
  FILE* config = fopen(ANDROID_CONFIG_FILENAME, "wb");
  if (config)
  {
    fprintf(config, "[misc]\n");
    fprintf(config, "config_enabled = %d\n", psp_config_enabled);
    fprintf(config, "rotation = %d\n", psp_rotation);
    fprintf(config, "translation = %s\n", psp_translation);

    fprintf(config, "[controls]\n");
    fprintf(config, "mouse_method = %d\n", config_mouse_control_mode);
    fprintf(config, "mouse_longclick = %d\n", config_mouse_longclick);
	
    fprintf(config, "[compatibility]\n");
//    fprintf(config, "ignore_acsetup_cfg_file = %d\n", psp_ignore_acsetup_cfg_file);
    fprintf(config, "clear_cache_on_room_change = %d\n", psp_clear_cache_on_room_change);

    fprintf(config, "[sound]\n");
    fprintf(config, "samplerate = %d\n", psp_audio_samplerate );
    fprintf(config, "enabled = %d\n", psp_audio_enabled);
    fprintf(config, "threaded = %d\n", psp_audio_multithreaded);
    fprintf(config, "cache_size = %d\n", psp_audio_cachesize);
    
    fprintf(config, "[midi]\n");
    fprintf(config, "enabled = %d\n", psp_midi_enabled);
    fprintf(config, "preload_patches = %d\n", psp_midi_preload_patches);

    fprintf(config, "[video]\n");
    fprintf(config, "framedrop = %d\n", psp_video_framedrop);

    fprintf(config, "[graphics]\n");
    fprintf(config, "renderer = %d\n", psp_gfx_renderer);
    fprintf(config, "smoothing = %d\n", psp_gfx_smoothing);
    fprintf(config, "scaling = %d\n", psp_gfx_scaling);
    fprintf(config, "super_sampling = %d\n", psp_gfx_super_sampling);
    fprintf(config, "smooth_sprites = %d\n", psp_gfx_smooth_sprites);

    fprintf(config, "[debug]\n");
    fprintf(config, "show_fps = %d\n", (display_fps == 2) ? 1 : 0);
    fprintf(config, "logging = %d\n", psp_debug_write_to_logcat);

    fclose(config);

    return true;
  }

  return false;
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
  Java_com_bigbluecup_android_EngineGlue_pauseEngine(JNIEnv* env, jobject object)
{
  PauseGame();
}

JNIEXPORT void JNICALL
  Java_com_bigbluecup_android_EngineGlue_resumeEngine(JNIEnv* env, jobject object)
{
  UnPauseGame();
}


JNIEXPORT void JNICALL
  Java_com_bigbluecup_android_EngineGlue_shutdownEngine(JNIEnv* env, jobject object)
{
  want_exit = 1;
}


void selectLatestSavegame()
{
  DIR* dir;
  struct dirent* entry;
  struct stat statBuffer;
  char buffer[200];
  time_t lastTime = 0;

  dir = opendir(saveGameDirectory);

  if (dir)
  {
    while ((entry = readdir(dir)) != 0)
    {
      if (ags_strnicmp(entry->d_name, "agssave", 7) == 0)
      {
        if (ags_stricmp(entry->d_name, "agssave.999") != 0)
        {
          strcpy(buffer, saveGameDirectory);
          strcat(buffer, entry->d_name);
          stat(buffer, &statBuffer);
          if (statBuffer.st_mtime > lastTime)
          {
            strcpy(lastSaveGameName, buffer);
            loadSaveGameOnStartup = lastSaveGameName;
            lastTime = statBuffer.st_mtime;
          }
        }
      }
    }
    closedir(dir);
  }
}

}


int ReadInteger(int* variable, const ConfigTree &cfg, const char* section, const char* name, int minimum, int maximum, int default_value)
{
  if (reset_configuration)
  {
    *variable = default_value;
    return 0;
  }

  int temp = INIreadint(cfg, section, name);

  if (temp == -1)
    return 0;

  if ((temp < minimum) || (temp > maximum))
    temp = default_value;

  *variable = temp;

  return 1;
}



int ReadString(char* variable, const ConfigTree &cfg, const char* section, const char* name, const char* default_value)
{
  if (reset_configuration)
  {
    strcpy(variable, default_value);
    return 0;
  }

  String temp;
  if (!INIreaditem(cfg, section, name, temp))
    temp = default_value;

  strcpy(variable, temp.GetCStr());

  return 1;
}



void ResetConfiguration()
{
  reset_configuration = true;

  ReadConfiguration(ANDROID_CONFIG_FILENAME, true);

  reset_configuration = false;
}



bool ReadConfiguration(const char* filename, bool read_everything)
{
  ConfigTree cfg;
  if (IniUtil::Read(filename, cfg) || reset_configuration)
  {
//    ReadInteger((int*)&psp_disable_powersaving, "misc", "disable_power_saving", 0, 1, 1);

//    ReadInteger((int*)&psp_return_to_menu, "misc", "return_to_menu", 0, 1, 1);

    ReadString(&psp_translation[0], cfg, "misc", "translation", "default");

    ReadInteger((int*)&psp_config_enabled, cfg, "misc", "config_enabled", 0, 1, 0);
    if (!psp_config_enabled && !read_everything)
      return true;

    ReadInteger(&psp_debug_write_to_logcat, cfg, "debug", "logging", 0, 1, 0);
    ReadInteger(&display_fps, cfg, "debug", "show_fps", 0, 1, 0);
    if (display_fps == 1)
      display_fps = 2;

    ReadInteger((int*)&psp_rotation, cfg, "misc", "rotation", 0, 2, 0);

//    ReadInteger((int*)&psp_ignore_acsetup_cfg_file, "compatibility", "ignore_acsetup_cfg_file", 0, 1, 0);
    ReadInteger((int*)&psp_clear_cache_on_room_change, cfg, "compatibility", "clear_cache_on_room_change", 0, 1, 0);

    ReadInteger((int*)&psp_audio_samplerate, cfg, "sound", "samplerate", 0, 44100, 44100);
    ReadInteger((int*)&psp_audio_enabled, cfg, "sound", "enabled", 0, 1, 1);
    ReadInteger((int*)&psp_audio_multithreaded, cfg, "sound", "threaded", 0, 1, 1);
    ReadInteger((int*)&psp_audio_cachesize, cfg, "sound", "cache_size", 1, 50, 10);

    ReadInteger((int*)&psp_midi_enabled, cfg, "midi", "enabled", 0, 1, 1);
    ReadInteger((int*)&psp_midi_preload_patches, cfg, "midi", "preload_patches", 0, 1, 0);

    ReadInteger((int*)&psp_video_framedrop, cfg, "video", "framedrop", 0, 1, 0);

    ReadInteger((int*)&psp_gfx_renderer, cfg, "graphics", "renderer", 0, 2, 0);
    ReadInteger((int*)&psp_gfx_smoothing, cfg, "graphics", "smoothing", 0, 1, 1);
    ReadInteger((int*)&psp_gfx_scaling, cfg, "graphics", "scaling", 0, 2, 1);
    ReadInteger((int*)&psp_gfx_super_sampling, cfg, "graphics", "super_sampling", 0, 1, 0);
    ReadInteger((int*)&psp_gfx_smooth_sprites, cfg, "graphics", "smooth_sprites", 0, 1, 0);

    ReadInteger((int*)&config_mouse_control_mode, cfg, "controls", "mouse_method", 0, 1, 0);
    ReadInteger((int*)&config_mouse_longclick, cfg, "controls", "mouse_longclick", 0, 1, 1);

    return true;
  }

  return false;
}

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
    const char * _game_file_name;
    _game_file_name = env->GetStringUTFChars(static_cast<jstring>(jobj_game_file_name), nullptr);
    if(_game_file_name != nullptr) {
        strcpy(psp_game_file_name, _game_file_name);
    }
    env->ReleaseStringUTFChars(static_cast<jstring>(jobj_game_file_name), _game_file_name);

    jfieldID fid_android_base_directory = env->GetFieldID(clazz, "_android_base_directory", "Ljava/lang/String;");
    jobject jobj_android_base_directory = env->GetObjectField(activity, fid_android_base_directory);
    const char * _android_base_directory;
    _android_base_directory = env->GetStringUTFChars(static_cast<jstring>(jobj_android_base_directory), nullptr);
    if(_android_base_directory != nullptr) {
        strcpy(android_base_directory, _android_base_directory);
    }
    env->ReleaseStringUTFChars(static_cast<jstring>(jobj_android_base_directory), _android_base_directory);

    jfieldID fid_android_app_directory = env->GetFieldID(clazz, "_android_app_directory", "Ljava/lang/String;");
    jobject jobj_android_app_directory = env->GetObjectField(activity, fid_android_app_directory);
    const char * _android_app_directory;
    _android_app_directory = env->GetStringUTFChars(static_cast<jstring>(jobj_android_app_directory), nullptr);
    if(_android_app_directory != nullptr) {
        strcpy(android_app_directory, _android_app_directory);
    }
    env->ReleaseStringUTFChars(static_cast<jstring>(jobj_android_app_directory), _android_app_directory);

    jfieldID fid_loadLastSave = env->GetFieldID(clazz, "_loadLastSave", "Z");
    jboolean jbool_loadLastSave = env->GetBooleanField(activity, fid_loadLastSave);
    bool loadLastSave = jbool_loadLastSave;

    // Reset configuration.
    ResetConfiguration();

    // Read general configuration.
    ReadConfiguration((char*) ANDROID_CONFIG_FILENAME, true);

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
    ReadConfiguration((char*) ANDROID_CONFIG_FILENAME, false);

    // Set the screen rotation.
    if (psp_rotation == 0) {
      // Auto, let the user rotate as wished.
    } else if(psp_rotation == 1) {
      // Portrait
      SDL_SetHint(SDL_HINT_ORIENTATIONS, "Portrait PortraitUpsideDown");
    }  else if(psp_rotation == 2) {
      // Landscape
        SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
    }

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
  return 1;//cd_player_control(cmdd, datt);
}

void AGSAndroid::DisplayAlert(const char *text, ...) {
  char displbuf[2000];
  va_list args;
  va_start(args, text);
  vsprintf(displbuf, text, args);
  SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "AGSNative",displbuf, nullptr);
  va_end(args);


  /*
  // It is possible that this is called from a thread that is not yet known
  // to the Java VM. So attach it first before displaying the message.
  JNIEnv* thread_env;
 // android_jni_vm->AttachCurrentThread(&thread_env, NULL);

  __android_log_print(ANDROID_LOG_DEBUG, "AGSNative", "%s", displbuf);

  jstring java_string = thread_env->NewStringUTF(displbuf);
  thread_env->CallVoidMethod(java_object, java_messageCallback, java_string);
  usleep(1000 * 1000);
  thread_env->CallVoidMethod(java_object, java_blockExecution);
*/
//  android_jni_vm->DetachCurrentThread();
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
  return 1;//cd_player_init();
}

void AGSAndroid::PostBackendExit() {
  ShutdownAndroidFile();
  //java_environment->DeleteGlobalRef(java_class);
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
  //cd_exit();
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
