#include <pspsdk.h>
#include <pspthreadman.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <pspkernel.h>
#include <psputility.h>
#include <pspdisplay.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../kernel/kernel.h"
#include "pe.h"


PSP_MODULE_INFO("launcher", 0, 1, 1);


typedef struct
{
  char display_name[100];
  char path[200];
  version_info_t version_info;
  int iscompatible;
  int isdata;
  int is31xdata;
} games_t;


char psp_game_file_name[256];
char file_to_exec[256];
int quit_to_menu = 1;

int exit_game = 0;

int count = 0;
int max_entries = 200;
games_t entries[200];




int exit_callback(int arg1, int arg2, void *common)
{
  sceKernelExitGame();
  return 0;
}



int CallbackThread(SceSize args, void *argp)
{
  int cbid;
  cbid = sceKernelCreateCallback("Launcher Exit Callback", exit_callback, NULL);
  sceKernelRegisterExitCallback(cbid);
  sceKernelSleepThreadCB();
  return 0;
}



int CompareFunction(const void* a, const void* b)
{
  return strcmp(((games_t*)a)->display_name, ((games_t*)b)->display_name);
}



int IsDataFile(version_info_t* version_info)
{
  return (strcmp(version_info->internal_name, "acwin") == 0);
}



int IsCompatibleDatafile(version_info_t* version_info)
{
  int major = 0;
  int minor = 0;
  int rev = 0;
  int build = 0;
  
  sscanf(version_info->version, "%d.%d.%d.%d", &major, &minor, &rev, &build);
  
  return ((major == 3) && (minor == 2));
}



int IsCompatible31xDatafile(version_info_t* version_info)
{
  int major = 0;
  int minor = 0;
  int rev = 0;
  int build = 0;
  
  sscanf(version_info->version, "%d.%d.%d.%d", &major, &minor, &rev, &build);
  
  return ((major == 3) && (minor == 1) && (rev > 0));
}



void AddGameEntry(char* path, struct dirent* entry, version_info_t* version_info)
{
  strcpy(entries[count].path, path);
  strcpy(entries[count].display_name, entry->d_name);
  memcpy(&entries[count].version_info, version_info, sizeof(version_info_t));
  entries[count].isdata = 1;

  if (IsCompatibleDatafile(version_info))
    entries[count].iscompatible = 1;

  if (IsCompatible31xDatafile(version_info))
    entries[count].is31xdata = entries[count].iscompatible = 1;

  count++;
}



int CreateGameList()
{
  memset(entries, 0, max_entries * sizeof(games_t));
  
  version_info_t version_info;
  char buffer[200];
  DIR* fd = NULL;
  struct dirent* entry = NULL;
  struct stat info;
  if ((fd = opendir(".")))
  {
    while ((entry = readdir(fd)) && (count < max_entries))
    {
      // Exclude pseudo directories
      if ((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0))
        continue;
    
      // Exclude files
      stat(entry->d_name, &info);
      if (!S_ISDIR(info.st_mode))
        continue;

      // Check for ac2game.dat in the folder
      strcpy(buffer, entry->d_name);
      strcat(buffer, "/ac2game.dat");
      if (stat(buffer, &info) == 0)
      {
        if (!getVersionInformation(buffer, &version_info))
          break;
  
        if (IsDataFile(&version_info))
        {
          AddGameEntry(buffer, entry, &version_info);
          continue;
        }
      }

      // Check all ".exe" files in the folder
      DIR* fd1 = NULL;
      struct dirent* entry1 = NULL;

      if ((fd1 = opendir(entry->d_name)))
      {
        while ((entry1 = readdir(fd1)))
        {
          // Exclude the setup program
          if (stricmp(entry1->d_name, "winsetup.exe") == 0)
            continue;

          // Filename must be >= 4 chars long
          int length = strlen(entry1->d_name);
          if (length < 4)
            continue;
  
          if (stricmp(&(entry1->d_name[length - 4]), ".exe") == 0)
          {
            strcpy(buffer, entry->d_name);
            strcat(buffer, "/");
            strcat(buffer, entry1->d_name);

            if (!getVersionInformation(buffer, &version_info))
              continue;
  
            if (IsDataFile(&version_info))
            {
              AddGameEntry(buffer, entry, &version_info);
              continue;
            }
          }
        }
        closedir(fd1);
      }
    }  
    closedir(fd);
  }
  
  return 1;
}



int ShowMenu()
{
  int swap_buttons = 0;
  sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_UNKNOWN, &swap_buttons);
  unsigned int ok_button = swap_buttons ? PSP_CTRL_CROSS : PSP_CTRL_CIRCLE;

  pspDebugScreenInit();
  pspDebugScreenEnableBackColor(1);
  pspDebugScreenClearLineEnable();
  
  // Check for ac2game.dat in eboot folder and autostart if it exists
  FILE* test = fopen("ac2game.dat", "rb");
  if (test)
  {
    fclose(test);
    getcwd(psp_game_file_name, 256);
    strcat(psp_game_file_name, "/ac2game.dat");
    quit_to_menu = 0;
    return 1;
  }

  pspDebugScreenPrintf("Adventure Game Studio 3.21 by Chris Jones. PSP port by JJS.\n\n\n");
  pspDebugScreenPrintf("Please select a game:");
  
  if (count == 0)
  {
    pspDebugScreenPrintf("\n\nError: No games found. Quitting in 10 seconds.\n");
    sceKernelDelayThread(10 * 1000 * 1000);
    sceKernelExitGame();
  }

  pspDebugScreenSetXY(0, 33);
  pspDebugScreenPrintf("Press %s to start", swap_buttons ? "CROSS" : "CIRCLE");


  qsort(entries, count, sizeof(games_t), CompareFunction);
  
  SceCtrlData pad;
  unsigned int old_buttons = 0;
  int max_entries_to_show = 25;
  int index = 0;
  int start_index = 0;
  int repeatCount = 0;
  int repeatFast = 0;
  char tempbuffer[100];  
  
  while (!exit_game)
  {
    pspDebugScreenSetXY(0, 6);
  
    int i;
    int end_index;
  
    end_index = start_index + max_entries_to_show;
    if (end_index > count)
      end_index = count;
  
    for (i = start_index; i < end_index; i++)
    {
      if (i == index)
        pspDebugScreenSetTextColor(0xFFFFFFFF);
      else
      {
        if (entries[i].iscompatible)
          pspDebugScreenSetTextColor(0xFF999999);
        else
          pspDebugScreenSetTextColor(0xFF555555);
      }

      if (entries[i].iscompatible)
        sprintf(tempbuffer, "%s", entries[i].display_name);
      else if (entries[i].isdata)
        sprintf(tempbuffer, "%s (incompatible version %s)", entries[i].display_name, entries[i].version_info.version);

      strcpy(&tempbuffer[64], "\0");
      pspDebugScreenPrintf("   %-64s\n", tempbuffer);
    }

    sceDisplayWaitVblankStart();
    sceCtrlReadBufferPositive(&pad, 1);

    if ((pad.Buttons & PSP_CTRL_DOWN) && !(old_buttons & PSP_CTRL_DOWN))
    {
      index++;
      if (index >= count)
      {
        index = 0;
        start_index = 0;
      }
      
      if (index >= end_index)
      {
        start_index++;
        end_index++;
      }
    }

    if ((pad.Buttons & PSP_CTRL_UP) && !(old_buttons & PSP_CTRL_UP))
    {
      index--;
      if (index < 0)
      {
        index = count - 1;
        start_index = count - max_entries_to_show;
        if (start_index < 0)
          start_index = 0;
      }
    
      if (index < start_index)
      {
        start_index--;
        end_index--;
      }  
    }

    if ((pad.Buttons & ok_button) && !(old_buttons & ok_button))
    {
      getcwd(psp_game_file_name, 256);
      strcat(psp_game_file_name, "/");
      strcat(psp_game_file_name, entries[index].path);
      pspDebugScreenClear();
      return 1;
    }
  
    // Handling of button press repeating
    if (old_buttons == pad.Buttons)
    {
      if (pad.Buttons == 0)
      {
        repeatFast = 0;
        repeatCount = 0;
      }
      else
        repeatCount++;
    }
    else
    {
      repeatCount = repeatFast ? 24 : 0;
      repeatFast = 0;
    }

    if (repeatCount > 25)
    {
      pad.Buttons = 0;
      repeatCount = 0;
      repeatFast = 1;
    }

    old_buttons = pad.Buttons;
  }

  return 0;
}



int main(int argc, char *argv[])
{
  // Setup callbacks, remember thread id
  int callback_thid = sceKernelCreateThread("launcher_update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
  if (callback_thid > -1)
    sceKernelStartThread(callback_thid, 0, 0);

  // Search game files
  pspDebugScreenInit();
  pspDebugScreenPrintf("Searching for compatible games...");
  CreateGameList();

  // Display game selection screen  
  ShowMenu();
  
  // Load the kernel mode prx  
  SceUID moduleId = pspSdkLoadStartModule("kernel.prx", PSP_MEMORY_PARTITION_KERNEL);
  if (moduleId < 0)
  {
    pspDebugScreenInit();
    pspDebugScreenPrintf("Error: Cannot load \"kernel.prx\". Quitting in 10 seconds.\n");
    sceKernelDelayThread(10 * 1000 * 1000);
    sceKernelExitGame();
  }

  // Terminate the callback thread
  sceKernelWakeupThread(callback_thid);
  sceKernelWaitThreadEnd(callback_thid, NULL);
  sceKernelTerminateDeleteThread(callback_thid);

  // Execute the AGS 3.21 engine
  strcpy(file_to_exec, argv[0]);
  strcpy(&file_to_exec[strlen(file_to_exec) - strlen("EBOOT.PBP")], "ags321.prx");

  char* buffer_argv[2];
  buffer_argv[0] = psp_game_file_name;
  buffer_argv[1] = (quit_to_menu ? "menu" : "quit");
 
  // Load the prx with the kernel mode function  
  kernel_loadExec(file_to_exec, 2, buffer_argv);

  return 0;
}
