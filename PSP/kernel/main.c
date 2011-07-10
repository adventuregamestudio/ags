#include <stdio.h>
#include <string.h>
#include "kernel.h"


PSP_MODULE_INFO("kernel", PSP_MODULE_KERNEL, 1, 1);

char exefile[256];
char parameters[1024];
int paramlength;


int launcher_thread(SceSize args, void *argp)
{
	int status = 0;

	// Unload the launcher
	SceModule2* mod = (SceModule2*)sceKernelFindModuleByName("launcher");
    int result = sceKernelStopModule(mod->modid, 0, NULL, &status, NULL);
	result = sceKernelUnloadModule(mod->modid);
	
	// Load the game engine
	SceUID modid = sceKernelLoadModule(exefile, 0, NULL);
	sceKernelStartModule(modid, paramlength, parameters, &status, NULL);
	
	// Unload this module
	sceKernelSelfStopUnloadModule(1, 0, NULL);
	
	return 0;
}

int kernel_loadExec(const char *file, int argc, char** argv)
{
	u32 k1;
	k1 = pspSdkSetK1(0);
	
	// Store file name
	strcpy(exefile, file);
	
	// Concat argument strings
	paramlength = 0;
	
	// argv[0]
	strcpy(parameters, file);
	paramlength = strlen(file) + 1;
	
	// Rest of the arguments
	int i;
	for (i = 0; i < argc; i++)
	{
	  strcpy(&parameters[paramlength], argv[i]);
	  paramlength += (strlen(argv[i]) + 1);
	}
	
    SceUID thid = sceKernelCreateThread("launcher_thread", launcher_thread, 0x20, 0xFA0, 0, 0);
    if (thid > -1)
      thid = sceKernelStartThread(thid, 0, 0);
	
	pspSdkSetK1(k1);
	return 0;
}


int module_start(SceSize args, void *argp)
{
  return 0;
}

int module_stop(SceSize args, void *argp)
{
  return 0;
}