#if !defined(ANDROID_VERSION)
#error This file should only be included on the Android build
#endif

#include "acplatfm.h"
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h> 
#include <ctype.h>

#include <allegro.h>
#include "bigend.h"

#include <jni.h>
#include <android/log.h>


#define ANDROID_CONFIG_FILENAME "android.cfg"

extern char filetouse[];
char *INIreaditem(const char *sectn, const char *entry);
int INIreadint (const char *sectn, const char *item, int errornosect = 1);
void ReadConfiguration(char* filename);

struct AGSAndroid : AGS32BitOSDriver {

  virtual int  CDPlayerCommand(int cmdd, int datt);
  virtual void Delay(int millis);
  virtual void DisplayAlert(const char*, ...);
  virtual unsigned long GetDiskFreeSpaceMB();
  virtual const char* GetNoMouseErrorString();
  virtual eScriptSystemOSID GetSystemOSID();
  virtual int  InitializeCDPlayer();
  virtual void PlayVideo(const char* name, int skip, int flags);
  virtual void PostAllegroExit();
  virtual int  RunSetup();
  virtual void SetGameWindowIcon();
  virtual void ShutdownCDPlayer();
  virtual void WriteConsole(const char*, ...);
  virtual void ReplaceSpecialPaths(const char *sourcePath, char *destPath);
  virtual void WriteDebugString(const char* texx, ...);
  virtual void ReadPluginsFromDisk(FILE *iii);
  virtual void StartPlugins();
  virtual void ShutdownPlugins();
  virtual int RunPluginHooks(int event, int data);
  virtual void RunPluginInitGfxHooks(const char *driverName, void *data);
  virtual int RunPluginDebugHooks(const char *scriptfile, int linenum);
};


//int psp_return_to_menu = 1;
int psp_ignore_acsetup_cfg_file = 0;
int psp_clear_cache_on_room_change = 0;


// Graphic options from the Allegro library.
extern int psp_gfx_scaling;
extern int psp_gfx_smoothing;


// Audio options from the Allegro library.
unsigned int psp_audio_samplerate;
int psp_audio_enabled = 1;
volatile int psp_audio_multithreaded = 1;
int psp_audio_cachesize = 10;
int psp_midi_enabled = 1;
int psp_midi_preload_patches = 0;

int psp_video_framedrop = 0;

int psp_gfx_hardware_acceleration = 1;
int psp_gfx_render_to_texture = 1;

extern int display_fps;
extern int want_exit;
extern void PauseGame();
extern void UnPauseGame();
extern int main(int argc,char*argv[]);

char psp_game_file_name[256];
char* psp_game_file_name_pointer = psp_game_file_name;

JNIEnv *java_environment;
jobject java_object;
jclass java_class;
jmethodID java_messageCallback;
jmethodID java_blockExecution;
jmethodID java_swapBuffers;

extern "C" 
{

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


JNIEXPORT jboolean JNICALL 
  Java_com_bigbluecup_android_EngineGlue_startEngine(JNIEnv* env, jobject object, jclass stringclass, jstring filename, jstring directory)
{
  // Get JNI interfaces.
  java_object = env->NewGlobalRef(object);
  java_environment = env;
  java_class = (jclass)java_environment->NewGlobalRef(java_environment->GetObjectClass(object));
  java_messageCallback = java_environment->GetMethodID(java_class, "showMessage", "(Ljava/lang/String;)V");
  java_blockExecution = java_environment->GetMethodID(java_class, "blockExecution", "()V");

  // Initialize JNI for Allegro.
  android_allegro_initialize_jni(java_environment, java_class, java_object);

  // Get the file to run from Java.
  const char* cpath = java_environment->GetStringUTFChars(filename, NULL);
  strcpy(psp_game_file_name, cpath);
  java_environment->ReleaseStringUTFChars(filename, cpath);

  // Get the base directory (usually "/sdcard/ags").
  const char* cdirectory = java_environment->GetStringUTFChars(directory, NULL);
  chdir(cdirectory);
  java_environment->ReleaseStringUTFChars(directory, cdirectory);

  // Read general configuration.
  ReadConfiguration(ANDROID_CONFIG_FILENAME);

	// Get the games path.
	char path[256];
	strcpy(path, psp_game_file_name);
	int lastindex = strlen(path) - 1;
	while (path[lastindex] != '/')
	{
	  path[lastindex] = 0;
	  lastindex--;
	}
  chdir(path);
  
  setenv("ULTRADIR", "..", 1);

  // Read game specific configuration.
  ReadConfiguration(ANDROID_CONFIG_FILENAME);

  // Start the engine main function.
	main(1, &psp_game_file_name_pointer);
  
  return true;
}

}



int ReadInteger(int* variable, char* section, char* name, int minimum, int maximum, int default_value)
{
  int temp = INIreadint(section, name);

  if (temp == -1)
    return 0;

  if ((temp < minimum) || (temp > maximum))
    temp = default_value;

  *variable = temp;

  return 1;
}



void ReadConfiguration(char* filename)
{
  FILE* test = fopen(filename, "rb");
  if (test)
  {
    fclose(test);
    strcpy(filetouse, filename);

//    ReadInteger((int*)&psp_disable_powersaving, "misc", "disable_power_saving", 0, 1, 1);

//    ReadInteger((int*)&psp_return_to_menu, "misc", "return_to_menu", 0, 1, 1);

    ReadInteger(&display_fps, "misc", "show_fps", 0, 1, 0);
    if (display_fps == 1)
      display_fps = 2;

    ReadInteger((int*)&psp_ignore_acsetup_cfg_file, "compatibility", "ignore_acsetup_cfg_file", 0, 1, 0);
    ReadInteger((int*)&psp_clear_cache_on_room_change, "compatibility", "clear_cache_on_room_change", 0, 1, 0);

    ReadInteger((int*)&psp_audio_samplerate, "sound", "samplerate", 0, 44100, 44100);
    ReadInteger((int*)&psp_audio_enabled, "sound", "enabled", 0, 1, 1);
    ReadInteger((int*)&psp_audio_multithreaded, "sound", "threaded", 0, 1, 1);

    ReadInteger((int*)&psp_midi_enabled, "midi", "enabled", 0, 1, 1);
    ReadInteger((int*)&psp_midi_preload_patches, "midi", "preload_patches", 0, 1, 0);

    int audio_cachesize;
    if (ReadInteger((int*)&audio_cachesize, "sound", "cache_size", 1, 50, 10));
      psp_audio_cachesize = audio_cachesize;

    ReadInteger((int*)&psp_video_framedrop, "video", "framedrop", 0, 1, 0);

    ReadInteger((int*)&psp_gfx_hardware_acceleration, "graphics", "hardware_rendering", 0, 1, 1);
    ReadInteger((int*)&psp_gfx_smoothing, "graphics", "smoothing", 0, 1, 1);
    ReadInteger((int*)&psp_gfx_scaling, "graphics", "scaling", 0, 1, 1);
    ReadInteger((int*)&psp_gfx_render_to_texture, "graphics", "render_to_texture", 0, 1, 1);

    strcpy(filetouse, "nofile");
  }
}



void AGSAndroid::WriteDebugString(const char* texx, ...) {
  char displbuf[STD_BUFFER_SIZE] = "AGS: ";
  va_list ap;
  va_start(ap,texx);
  vsprintf(&displbuf[5],texx,ap);
  va_end(ap);
  __android_log_print(ANDROID_LOG_DEBUG, "AGSNative", displbuf);
}

void AGSAndroid::ReplaceSpecialPaths(const char *sourcePath, char *destPath)
{
  if (strnicmp(sourcePath, "$MYDOCS$", 8) == 0) 
  {
    strcpy(destPath, ".");
    strcat(destPath, &sourcePath[8]);
  }
  else if (strnicmp(sourcePath, "$APPDATADIR$", 12) == 0) 
  {
    strcpy(destPath, ".");
    strcat(destPath, &sourcePath[12]);
  }
  else {
    strcpy(destPath, sourcePath);
  }
}

int AGSAndroid::CDPlayerCommand(int cmdd, int datt) {
  return 1;//cd_player_control(cmdd, datt);
}

void AGSAndroid::DisplayAlert(const char *text, ...) {
  char displbuf[2000];
  va_list ap;
  va_start(ap, text);
  vsprintf(displbuf, text, ap);
  va_end(ap);
  __android_log_print(ANDROID_LOG_DEBUG, "AGSNative", displbuf);

  jstring java_string = java_environment->NewStringUTF(displbuf);
  java_environment->CallVoidMethod(java_object, java_messageCallback, java_string);
  java_environment->CallVoidMethod(java_object, java_blockExecution);
}

void AGSAndroid::Delay(int millis) {
  usleep(millis * 1000);
}

unsigned long AGSAndroid::GetDiskFreeSpaceMB() {
  // placeholder
  return 100;
}

const char* AGSAndroid::GetNoMouseErrorString() {
  return "This game requires a mouse. You need to configure and setup your mouse to play this game.\n";
}

eScriptSystemOSID AGSAndroid::GetSystemOSID() {
  return eOS_Win;
}

int AGSAndroid::InitializeCDPlayer() {
  return 1;//cd_player_init();
}

void AGSAndroid::PlayVideo(const char *name, int skip, int flags) {
  // do nothing
}

void AGSAndroid::PostAllegroExit() {
  java_environment->DeleteGlobalRef(java_class);
}

int AGSAndroid::RunSetup() {
  return 0;
}

void AGSAndroid::SetGameWindowIcon() {
  // do nothing
}

void AGSAndroid::WriteConsole(const char *text, ...) {
  char displbuf[2000];
  va_list ap;
  va_start(ap, text);
  vsprintf(displbuf, text, ap);
  va_end(ap);
  __android_log_print(ANDROID_LOG_DEBUG, "AGSNative", displbuf);  
}

void AGSAndroid::ShutdownCDPlayer() {
  //cd_exit();
}

void AGSAndroid::ReadPluginsFromDisk(FILE *iii) {
  pl_read_plugins_from_disk(iii);
}

void AGSAndroid::StartPlugins() {
  pl_startup_plugins();
}

void AGSAndroid::ShutdownPlugins() {
  pl_stop_plugins();
}

int AGSAndroid::RunPluginHooks(int event, int data) {
  return pl_run_plugin_hooks(event, data);
}

void AGSAndroid::RunPluginInitGfxHooks(const char *driverName, void *data) {
  pl_run_plugin_init_gfx_hooks(driverName, data);
}

int AGSAndroid::RunPluginDebugHooks(const char *scriptfile, int linenum) {
  return pl_run_plugin_debug_hooks(scriptfile, linenum);
}

AGSPlatformDriver* AGSPlatformDriver::GetDriver() {
  if (instance == NULL)
    instance = new AGSAndroid();

  return instance;
}
