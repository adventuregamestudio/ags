#if !defined(BSD_VERSION) && !defined(LINUX_VERSION)
#error This file should only be included on the Linux or BSD build
#endif


#include "acplatfm.h"
#include <pspsdk.h>
#include <pspthreadman.h>
#include <pspdebug.h>
#include <psppower.h>
#include <pspctrl.h>
#include <pspkernel.h>
#include <psputility.h>
#include <pspfpu.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h> 

#include <allegro.h>


extern "C" {
#include <systemctrl.h>
#include <psploadexec_kernel.h>

#include "../PSP/exception/utility/exception.h"
#include "../PSP/kernel/kernel.h"
}
#include "../PSP/malloc/malloc_p5.h"


#ifdef PSP_ENABLE_PROFILING
extern "C" void gprof_cleanup();
#endif


#define PSP_CONFIG_FILENAME "psp.cfg"


struct AGSPSP : AGS32BitOSDriver {

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


int psp_standalone;
char psp_game_file_name[256];
extern char filetouse[];

char *INIreaditem(const char *sectn, const char *entry);
int INIreadint (const char *sectn, const char *item, int errornosect = 1);

extern void clear_sound_cache();

extern int display_fps;

int psp_return_to_menu = 1;
int psp_ignore_acsetup_cfg_file = 0;
int psp_enable_extra_memory = 0;
int psp_clear_cache_on_room_change = 0;



// Executable parameters from Allegro.
extern int psp_argc;
extern char **psp_argv;



// Graphic options from the Allegro library.
extern int psp_gfx_scaling;
extern int psp_gfx_smoothing;
extern int psp_disable_powersaving;



// Audio options from the Allegro library.
extern unsigned int psp_audio_samplerate;
int psp_audio_enabled = 1;
volatile int psp_audio_multithreaded = 1;
int psp_audio_cachesize = 10;
int psp_midi_enabled = 1;
int psp_midi_preload_patches = 0;

int psp_video_framedrop = 0;
char psp_translation[100];
int psp_gfx_smooth_sprites = 0;


// Mouse options from the Allegro library.
typedef struct
{
  int move_left;
  int move_right;
  int move_up;
  int move_down;
  int click_left;
  int click_right;
  int click_middle;
  int slow_down;
} psp_mouse_config_t;

extern psp_mouse_config_t psp_mouse_mapping;
extern psp_mouse_config_t psp_mouse_mapping_osk;

extern int psp_mouse_analog_deadzone;
extern float psp_mouse_analog_sensitivity;



// Keyboard options from the Allegro library.
typedef struct 
{
  int toggle;
  int enter_char;
  int next_char;
  int previous_char;
  int next_keyset;
  int previous_keyset;
} psp_keyboard_config_t;

extern psp_keyboard_config_t psp_keyboard_mapping;

typedef struct
{
  int button;
  int scancode;
} psp_button_mapping_t;

extern psp_button_mapping_t psp_to_scancode[11];
extern unsigned int psp_to_scancode_count;

extern psp_button_mapping_t psp_to_scancode_osk[11];
extern unsigned int psp_to_scancode_osk_count;

extern AL_CONST char *_keyboard_common_names[KEY_MAX];



// Values and human-readable names for the PSP buttons.
int psp_buttons_value[] =
{
  PSP_CTRL_TRIANGLE,
  PSP_CTRL_CIRCLE,
  PSP_CTRL_CROSS,
  PSP_CTRL_SQUARE,
  PSP_CTRL_UP,
  PSP_CTRL_DOWN,
  PSP_CTRL_LEFT,
  PSP_CTRL_RIGHT,
  PSP_CTRL_LTRIGGER,
  PSP_CTRL_RTRIGGER,
  PSP_CTRL_START,
  PSP_CTRL_SELECT
};

char* psp_buttons_name[] =
{
  "triangle",
  "circle",
  "cross",
  "square",
  "up",
  "down",
  "left",
  "right",
  "left_trigger",
  "right_trigger",
  "start",
  "select"
};


int GetScancodeFromKeyname(char* keyname)
{
  if (!keyname)
    return -1;

  int i;
  for (i = 0; i < KEY_MAX; i++)
  {
    if (stricmp(keyname, _keyboard_common_names[i]) == 0)
      return i;
  }

  return -1;
}


void ResetButtonConfiguration()
{
  psp_to_scancode_count = 0;
  psp_to_scancode_osk_count = 0;
  memset(&psp_to_scancode, 0, sizeof(psp_button_mapping_t) * 11);
  memset(&psp_mouse_mapping, 0, sizeof(psp_mouse_config_t));
  memset(&psp_to_scancode_osk, 0, sizeof(psp_button_mapping_t) * 11);
  memset(&psp_mouse_mapping_osk, 0, sizeof(psp_mouse_config_t));
  memset(&psp_keyboard_mapping, 0, sizeof(psp_keyboard_config_t));
}


void ReadButtonMapping(char* section, psp_button_mapping_t* button_mapping, unsigned int &button_mapping_count, psp_mouse_config_t* mouse_mapping, psp_keyboard_config_t* keyboard_mapping)
{
  char* value;
  int scancode;

  for (int i = 0; i < sizeof(psp_buttons_value) / 4; i++)
  {
    value = INIreaditem(section, psp_buttons_name[i]);

    if (!value)
      continue;

    // Check for key command
    scancode = GetScancodeFromKeyname(value);
    if (scancode > -1)
    {
      button_mapping[button_mapping_count].button = psp_buttons_value[i];
      button_mapping[button_mapping_count].scancode = scancode;
      button_mapping_count++;
      printf("%s = %s (%d)\n", psp_buttons_name[i], value, scancode);
      continue;
    }    

    // Check for mouse command.
    if (stricmp(value, "mouse_up") == 0)
      mouse_mapping->move_up = psp_buttons_value[i];
    else if (stricmp(value, "mouse_down") == 0)
      mouse_mapping->move_down = psp_buttons_value[i];
    else if (stricmp(value, "mouse_left") == 0)
      mouse_mapping->move_left = psp_buttons_value[i];
    else if (stricmp(value, "mouse_right") == 0)
      mouse_mapping->move_right = psp_buttons_value[i];
    else if (stricmp(value, "mouse_click_left") == 0)
      mouse_mapping->click_left = psp_buttons_value[i];
    else if (stricmp(value, "mouse_click_right") == 0)
      mouse_mapping->click_right = psp_buttons_value[i];
    else if (stricmp(value, "mouse_click_middle") == 0)
      mouse_mapping->click_middle = psp_buttons_value[i];
    else if (stricmp(value, "mouse_slow_down") == 0)
      mouse_mapping->slow_down = psp_buttons_value[i];

    // Check for onscreen keybord command.
    if (keyboard_mapping)
    {
      if (stricmp(value, "keyboard_toggle") == 0)
        keyboard_mapping->toggle = psp_buttons_value[i];
      else if (stricmp(value, "keyboard_enter_char") == 0)
        keyboard_mapping->enter_char = psp_buttons_value[i];
      else if (stricmp(value, "keyboard_next_char") == 0)
        keyboard_mapping->next_char = psp_buttons_value[i];
      else if (stricmp(value, "keyboard_previous_char") == 0)
        keyboard_mapping->previous_char = psp_buttons_value[i];
      else if (stricmp(value, "keyboard_next_keyset") == 0)
        keyboard_mapping->next_keyset = psp_buttons_value[i];
      else if (stricmp(value, "keyboard_previous_keyset") == 0)
        keyboard_mapping->previous_keyset = psp_buttons_value[i];
    }

    free(value);
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



int ReadString(char* variable, char* section, char* name, char* default_value)
{
  char* temp = INIreaditem(section, name);

  if (temp == NULL)
    temp = default_value;

  strcpy(variable, temp);

  return 1;
}




void ReadConfiguration(char* filename)
{
  FILE* test = fopen(filename, "rb");
  if (test)
  {
    fclose(test);
    strcpy(filetouse, filename);

    ResetButtonConfiguration();

    ReadButtonMapping("button_mapping", psp_to_scancode, psp_to_scancode_count, &psp_mouse_mapping, NULL);
    ReadButtonMapping("onscreen_keyboard", psp_to_scancode_osk, psp_to_scancode_osk_count, &psp_mouse_mapping_osk, &psp_keyboard_mapping);

    ReadString(&psp_translation[0], "misc", "translation", "default");

    ReadInteger((int*)&psp_disable_powersaving, "misc", "disable_power_saving", 0, 1, 1);

    ReadInteger((int*)&psp_return_to_menu, "misc", "return_to_menu", 0, 1, 1);

    ReadInteger(&display_fps, "misc", "show_fps", 0, 1, 0);
    if (display_fps == 1)
      display_fps = 2;

    ReadInteger((int*)&psp_ignore_acsetup_cfg_file, "compatibility", "ignore_acsetup_cfg_file", 0, 1, 0);
    ReadInteger((int*)&psp_enable_extra_memory, "compatibility", "enable_extra_memory", 0, 1, 0);
    ReadInteger((int*)&psp_clear_cache_on_room_change, "compatibility", "clear_cache_on_room_change", 0, 1, 0);

    ReadInteger((int*)&psp_audio_samplerate, "sound", "samplerate", 0, 44100, 44100);
    ReadInteger((int*)&psp_audio_enabled, "sound", "enabled", 0, 1, 1);
    ReadInteger((int*)&psp_audio_multithreaded, "sound", "threaded", 0, 1, 1);

    ReadInteger((int*)&psp_midi_enabled, "midi", "enabled", 0, 1, 1);
    ReadInteger((int*)&psp_midi_preload_patches, "midi", "preload_patches", 0, 1, 0);

    int audio_cachesize;
    if (ReadInteger((int*)&audio_cachesize, "sound", "cache_size", 1, 50, 10));
      psp_audio_cachesize = audio_cachesize;

    int mouse_sensitivity;
    if (ReadInteger((int*)&mouse_sensitivity, "analog_stick", "sensitivity", 0, 500, 50))
      psp_mouse_analog_sensitivity = (float)mouse_sensitivity / 25.0f;

    ReadInteger((int*)&psp_mouse_analog_deadzone, "analog_stick", "deadzone", 0, 128, 20);

    ReadInteger((int*)&psp_video_framedrop, "video", "framedrop", 0, 1, 0);

    ReadInteger((int*)&psp_gfx_smoothing, "graphics", "smoothing", 0, 1, 1);
    ReadInteger((int*)&psp_gfx_scaling, "graphics", "scaling", 0, 1, 1);
    ReadInteger((int*)&psp_gfx_smooth_sprites, "graphics", "smooth_sprites", 0, 1, 0);

    strcpy(filetouse, "nofile");
  }
}



// We need this as C code so that it can be called from the exception handler.
extern "C" void psp_quit()
{
  if (psp_enable_extra_memory)
    malloc_p5_shutdown();

#ifdef PSP_ENABLE_PROFILING
  gprof_cleanup();
  sceKernelExitGame();
#endif
  
  if (psp_return_to_menu)
  {
    char buffer[200];
    strcpy(buffer, psp_argv[0]);
    strcpy(&buffer[strlen(psp_argv[0]) - strlen("ags321.prx")], "EBOOT.PBP");

    struct SceKernelLoadExecVSHParam param;
    memset(&param, 0, sizeof(param));
    param.size = sizeof(SceKernelLoadExecVSHParam);
    param.args = strlen(buffer) + 1;
    param.argp = (void*)buffer;
    param.key = "game";
    if (strncmp(buffer, "ef0", 3) == 0)
      sctrlKernelLoadExecVSHWithApitype(0x152, buffer, &param);
    else
      sctrlKernelLoadExecVSHWithApitype(0x141, buffer, &param);
  }
  else
    sceKernelExitGame();
}



void psp_initialize()
{
  // Read in the default configuration.
  ReadConfiguration(PSP_CONFIG_FILENAME);
  
  // Check the arguments, if it is only 1 the engine is running without the menu.
  if (psp_argc == 1)
  {
    getcwd(psp_game_file_name, 256);
    strcat(psp_game_file_name, "/");
    strcat(psp_game_file_name, "ac2game.dat");

    // Exit to the XMB.
    psp_return_to_menu = 0;
  }
  else
  {
    // Get the file to run from the parameter.
	strcpy(psp_game_file_name, psp_argv[1]);
		
	// Get the games path.
	char path[256];
	strcpy(path, psp_game_file_name);
	int lastindex = strlen(path) - 1;
	while (path[lastindex] != '/')
	{
	  path[lastindex] = 0;
	  lastindex--;
	}
	
	// Make the game path current.
	chdir(path);
	
	// Read game configuration.
	ReadConfiguration(PSP_CONFIG_FILENAME);

    // Set the ULTRADIR environment variable so that the midi patches can be found.
    setenv("ULTRADIR", "..", 1);

    // If the game was started directly, don't return to the menu.
    if (strcmp(psp_argv[2], "quit") == 0)
      psp_return_to_menu = 0;
  }

  if (psp_enable_extra_memory)
    malloc_p5_init();
}




void AGSPSP::WriteDebugString(const char* texx, ...) {
  char displbuf[STD_BUFFER_SIZE] = "AGS: ";
  va_list ap;
  va_start(ap,texx);
  vsprintf(&displbuf[5],texx,ap);
  va_end(ap);
  strcat(displbuf, "\n");

  printf(displbuf);
}

void AGSPSP::ReplaceSpecialPaths(const char *sourcePath, char *destPath)
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

int AGSPSP::CDPlayerCommand(int cmdd, int datt) {
  return 1;//cd_player_control(cmdd, datt);
}

void AGSPSP::DisplayAlert(const char *text, ...) {
  // Print to debug screen, then wait for key press.
  char displbuf[2000];
  va_list ap;
  va_start(ap, text);
  vsprintf(displbuf, text, ap);
  va_end(ap);

  pspDebugScreenInit();
  pspDebugScreenPrintf(displbuf);
  
  printf("%s\n", displbuf);

  SceCtrlData ctrlData;

  do
  {
    sceCtrlPeekBufferPositive(&ctrlData, 1);
    sceKernelDelayThread(1000 * 100);
  } while (ctrlData.Buttons == 0);
}

void AGSPSP::Delay(int millis) {
  sceKernelDelayThread(millis * 1000);
}

unsigned long AGSPSP::GetDiskFreeSpaceMB() {
  // placeholder
  return 100;
}

const char* AGSPSP::GetNoMouseErrorString() {
  return "This game requires a mouse. You need to configure and setup your mouse to play this game.\n";
}

eScriptSystemOSID AGSPSP::GetSystemOSID() {
  return eOS_Win;
}

int AGSPSP::InitializeCDPlayer() {
  return 1;//cd_player_init();
}

void AGSPSP::PlayVideo(const char *name, int skip, int flags) {
  // do nothing
}

void AGSPSP::PostAllegroExit() {
  psp_quit();
}

int AGSPSP::RunSetup() {
  return 0;
}

void AGSPSP::SetGameWindowIcon() {
  // do nothing
}

void AGSPSP::WriteConsole(const char *text, ...) {
  char displbuf[2000];
  va_list ap;
  va_start(ap, text);
  vsprintf(displbuf, text, ap);
  va_end(ap);
  printf("%s", displbuf);
}

void AGSPSP::ShutdownCDPlayer() {
  //cd_exit();
}


void AGSPSP::ReadPluginsFromDisk(FILE *iii) {
  pl_read_plugins_from_disk(iii);
}

void AGSPSP::StartPlugins() {
  pl_startup_plugins();
}

void AGSPSP::ShutdownPlugins() {
  pl_stop_plugins();
}

int AGSPSP::RunPluginHooks(int event, int data) {
  return pl_run_plugin_hooks(event, data);
}

void AGSPSP::RunPluginInitGfxHooks(const char *driverName, void *data) {
  pl_run_plugin_init_gfx_hooks(driverName, data);
}

int AGSPSP::RunPluginDebugHooks(const char *scriptfile, int linenum) {
  return pl_run_plugin_debug_hooks(scriptfile, linenum);
}


AGSPlatformDriver* AGSPlatformDriver::GetDriver() {
  if (instance == NULL)
    instance = new AGSPSP();

  // Disable FPU exception.
  // JJS: I only know of one case where this is relevant. In Ben Jordan 8 when
  // going from the map to the library SCMD_FMULREG will be called with the
  // floating point register containing NAN.
  pspFpuSetEnable(0);

  // Setup the exception handler prx.
  initExceptionHandler();

  // Load the kernel module
  pspSdkLoadStartModule("kernel.prx", PSP_MEMORY_PARTITION_KERNEL);

  // Set CPU speed to maximum here.
  scePowerSetClockFrequency(333, 333, 166);
  
  // Initialize the game filename.
  psp_initialize();
  
  return instance;
}
