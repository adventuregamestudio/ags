#include <pspsdk.h>
#include <psppower.h>
#include <pspkernel.h>
#include <pspsuspend.h>
#include <pspsysevent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "../kernel/kernel.h"

//#define DEBUG_MALLOC_P5

#ifdef DEBUG_MALLOC_P5
unsigned int malloc_p5_memory_used = 0;
#endif

struct PspSysEventHandler malloc_p5_sysevent_handler_struct;

// From: http://forums.ps2dev.org/viewtopic.php?p=70854#70854
int malloc_p5_sysevent_handler(int ev_id, char* ev_name, void* param, int* result)
{
  if (ev_id == 0x100) // PSP_SYSEVENT_SUSPEND_QUERY
    return -1;
  
  return 0;
} 



int malloc_p5_initialized = 0;

int malloc_p5_init()
{
  // Unlock memory partition 5
  void* pointer = NULL;
  int returnsize = 0;
  int result = sceKernelVolatileMemLock(0, &pointer, &returnsize);
  if (result == 0)
  {
    printf("sceKernelVolatileMemLock(%d, %d) =  %d\n", (unsigned int)pointer, returnsize, result);

    // Register sysevent handler to prevent suspend mode because p5 memory cannot be resumed
    memset(&malloc_p5_sysevent_handler_struct, 0, sizeof(struct PspSysEventHandler));
    malloc_p5_sysevent_handler_struct.size = sizeof(struct PspSysEventHandler);
    malloc_p5_sysevent_handler_struct.name = "p5_suspend_handler";
    malloc_p5_sysevent_handler_struct.handler = &malloc_p5_sysevent_handler;
    malloc_p5_sysevent_handler_struct.type_mask = 0x0000FF00;
    kernel_sceKernelRegisterSysEventHandler(&malloc_p5_sysevent_handler_struct);

    malloc_p5_initialized = 1;
  }
  else
    malloc_p5_initialized = 0;
    
  return malloc_p5_initialized;
}

int malloc_p5_shutdown()
{
  if (malloc_p5_initialized)
  {
    if (sceKernelVolatileMemUnlock(0) == 0)
    {
      kernel_sceKernelUnregisterSysEventHandler(&malloc_p5_sysevent_handler_struct);

      malloc_p5_initialized = 0;
    }
  }

  return !malloc_p5_initialized;
}

void* malloc(size_t size)
{
  if (!malloc_p5_initialized)
    return (void*)_malloc_r(NULL, size);
  
  void* result = (void*)_malloc_r(NULL, size);

#ifdef DEBUG_MALLOC_P5
  struct mallinfo info = _mallinfo_r(NULL);
  printf("used memory %d of %d - %d\n", info.usmblks + info.uordblks, info.arena, malloc_p5_memory_used);
#endif

  if (result)
    return result;

  SceUID uid = sceKernelAllocPartitionMemory(5, "", PSP_SMEM_Low, size + 8, NULL);
  if (uid >= 0)
  {
#ifdef DEBUG_MALLOC_P5
    printf("getting memory from p5 %d %d\n", size, uid);  
    malloc_p5_memory_used += size;
#endif
    unsigned int* pointer = (unsigned int*)sceKernelGetBlockHeadAddr(uid);
    *pointer = uid;
    *(pointer + 4) = size;
    return (void*)(pointer + 8);
  }
  else
  {
#ifdef DEBUG_MALLOC_P5
    printf("*****failed to allocate %d byte from p5\n", size);
#endif

    return NULL;
  }
}

void* calloc(size_t num, size_t size)
{
  if (!malloc_p5_initialized)
    return (void*)_calloc_r(NULL, num, size);
  
  void* result = malloc(num * size);
  if (result)
    memset(result, 0, num * size);

  return result;
}

void* realloc(void* ptr, size_t size)
{
  if (!malloc_p5_initialized)
    return (void*)_realloc_r(NULL, ptr, size);
  
  if (!ptr)
    return malloc(size);

  if (ptr >= (void*)0x08800000)
  {
    void* result = (void*)_realloc_r(NULL, ptr, size);
    return result;
  }
  else
  {
    unsigned int oldsize = (unsigned int)*((SceUID*)ptr - 4);

#ifdef DEBUG_MALLOC_P5
    printf("realloc p5 memory %d %d\n", oldsize, size);
    malloc_p5_memory_used += (size - oldsize);
#endif

    void* target = malloc(size);
    if (target)
    {
      memcpy(target, ptr, oldsize);
      free(ptr);
      return target;
    }
  }

  return NULL;
}

void free(void* ptr)
{
  if (!malloc_p5_initialized)
  {
    _free_r(NULL, ptr);
  return;
  }

  if (!ptr)
    return;  

  if (ptr >= (void*)0x08800000)
    _free_r(NULL, ptr);
  else
  {
#ifdef DEBUG_MALLOC_P5
    printf("freeing p5 memory %d\n", (unsigned int)*((SceUID*)ptr - 8));
    malloc_p5_memory_used -= *((SceUID*)ptr - 4);
#endif

    sceKernelFreePartitionMemory(*((SceUID*)ptr - 8));
  }
}
