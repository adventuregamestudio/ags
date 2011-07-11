#include <pspsdk.h>
#include <pspthreadman.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <pspkernel.h>
#include <psputility.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h> 
#include "../kernel/kernel.h"

PSP_MODULE_INFO("launcher", 0, 0, 71);


char psp_game_file_name[256];
char file_to_exec[256];


int exit_callback(int arg1, int arg2, void *common)
{
    sceKernelExitGame();
	return 0;
}



int CallbackThread(SceSize args, void *argp)
{
    int cbid;
    cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
	return 0;
}



int CompareFunction(const void* a, const void* b)
{
  return strcmp((const char*)a, (const char*)b);
}



void ShowMenu()
{
  int swap_buttons = 0;
  sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_UNKNOWN, &swap_buttons);
  unsigned int ok_button = swap_buttons ? PSP_CTRL_CROSS : PSP_CTRL_CIRCLE;

  pspDebugScreenInit();
  pspDebugScreenEnableBackColor(0);

  // Check for ac2game.dat in eboot folder and autostart if it exists
  FILE* test = fopen("ac2game.dat", "rb");
  if (test)
  {
    fclose(test);
    getcwd(psp_game_file_name, 256);
    strcat(psp_game_file_name, "/");
    strcat(psp_game_file_name, "ac2game.dat");
    return;
  }

  pspDebugScreenPrintf("Adventure Game Studio 3.2.1 by Chris Jones. PSP port by JJS.\n\n\n");
  pspDebugScreenPrintf("Please select a game:");

  int count = 0;
  int max_entries = 25;
  char entries[25][100];
  memset(entries, 0, max_entries * 100);
  
  DIR* fd = NULL;
  struct dirent* entry = NULL;
  struct stat info;
  if (fd = opendir("."))
  {
    while ((entry = readdir(fd)) && (count < max_entries))
    {
      if ((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0))
        continue;
    
      stat(entry->d_name, &info);
      if (!S_ISDIR(info.st_mode))
        continue;

      strcpy(entries[count], entry->d_name);
      count++;
    }  
    closedir(fd);
  }
  
  if (count == 0)
  {
    pspDebugScreenPrintf("\n\nError: No games found. Quitting in 10 seconds.\n");
    sceKernelDelayThread(10 * 1000 * 1000);
    sceKernelExitGame();
  }

  pspDebugScreenSetXY(0, 33);
  pspDebugScreenPrintf("Press %s to start", swap_buttons ? "CROSS" : "CIRCLE");


  qsort(entries, count, 100, CompareFunction);
  
  SceCtrlData pad;
  unsigned int old_buttons = 0;
  int index = 0;
  int repeatCount = 0;
  
  while (1)
  {
    pspDebugScreenSetXY(0, 6);
  
    int i;
    for (i = 0; i < count; i++)
    {
      if (i == index)
        pspDebugScreenSetTextColor(0xFFFFFFFF);
      else
        pspDebugScreenSetTextColor(0xFF999999);

      pspDebugScreenPrintf("   %s\n", entries[i]);
    }

    sceCtrlReadBufferPositive(&pad, 1);

    if ((pad.Buttons & PSP_CTRL_DOWN) && !(old_buttons & PSP_CTRL_DOWN))
    {
      index++;
      if (index >= count)
        index = 0;
    }

    if ((pad.Buttons & PSP_CTRL_UP) && !(old_buttons & PSP_CTRL_UP))
    {
      index--;
      if (index < 0)
        index = count - 1;
    }

    if ((pad.Buttons & ok_button) && !(old_buttons & ok_button))
    {
      getcwd(psp_game_file_name, 256);
      strcat(psp_game_file_name, "/");
      strcat(psp_game_file_name, entries[index]);
      strcat(psp_game_file_name, "/");
      strcat(psp_game_file_name, "ac2game.dat");
      pspDebugScreenClear();
      return;
    }
  
    // Handling of button press repeating
    if (old_buttons == pad.Buttons)
      repeatCount++;
    else
      repeatCount = 0;

    if (repeatCount > 10)
    {
      pad.Buttons = 0;
      repeatCount = 0;
    }

    old_buttons = pad.Buttons;
  }  
}



int main(int argc, char *argv[])
{
  // Setup callbacks, remember thread id
  int callback_thid = sceKernelCreateThread("launcher_update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
  if (callback_thid > -1)
	sceKernelStartThread(callback_thid, 0, 0);

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

  // Execute the AGS 3.21 engine
  strcpy(file_to_exec, argv[0]);
  strcpy(&file_to_exec[strlen(file_to_exec) - strlen("EBOOT.PBP")], "ags321.prx");

  char* buffer_argv[2];
  buffer_argv[0] = file_to_exec;
  buffer_argv[1] = psp_game_file_name;

  // Load the prx with the kernel mode function  
  SceUID result = kernel_loadExec(file_to_exec, 2, buffer_argv);
  
  return 0;
}
