#include <pspkernel.h>
#include <pspsdk.h>
#include <pspctrl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

PspDebugRegBlock exception_regs;

extern SceModule module_info;
extern int _ftext;

static const char *codeTxt[32] =
{
    "Interrupt", "TLB modification", "TLB load/inst fetch", "TLB store",
    "Address load/inst fetch", "Address store", "Bus error (instr)",
    "Bus error (data)", "Syscall", "Breakpoint", "Reserved instruction",
    "Coprocessor unusable", "Arithmetic overflow", "Unknown 14",
    "Unknown 15", "Unknown 16", "Unknown 17", "Unknown 18", "Unknown 19",
    "Unknown 20", "Unknown 21", "Unknown 22", "Unknown 23", "Unknown 24",
    "Unknown 25", "Unknown 26", "Unknown 27", "Unknown 28", "Unknown 29",
    "Unknown 31"
};

static const unsigned char regName[32][5] =
{
    "zr", "at", "v0", "v1", "a0", "a1", "a2", "a3",
    "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
    "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"
};

void ExceptionHandler(PspDebugRegBlock * regs)
{
    int i;
    SceCtrlData pad;

    pspDebugScreenInit();
    pspDebugScreenSetBackColor(0x00FF0000);
    pspDebugScreenSetTextColor(0xFFFFFFFF);
    pspDebugScreenClear();
    pspDebugScreenPrintf("Your PSP has just crashed!\n");
    pspDebugScreenPrintf("Exception details:\n\n");

    pspDebugScreenPrintf("Exception - %s\n", codeTxt[(regs->cause >> 2) & 31]);
    pspDebugScreenPrintf("EPC       - %08X / %s.text + %08X\n", (int)regs->epc, module_info.modname, (unsigned int)(regs->epc-(int)&_ftext));
    pspDebugScreenPrintf("Cause     - %08X\n", (int)regs->cause);
    pspDebugScreenPrintf("Status    - %08X\n", (int)regs->status);
    pspDebugScreenPrintf("BadVAddr  - %08X\n", (int)regs->badvaddr);
    for(i=0; i<32; i+=4) pspDebugScreenPrintf("%s:%08X %s:%08X %s:%08X %s:%08X\n", regName[i], (int)regs->r[i], regName[i+1], (int)regs->r[i+1], regName[i+2], (int)regs->r[i+2], regName[i+3], (int)regs->r[i+3]);

    sceKernelDelayThread(1000000);
    pspDebugScreenPrintf("\n\nPress X to dump information on file exception.log and quit");
    pspDebugScreenPrintf("\nPress O to quit");

    for (;;){
        sceCtrlReadBufferPositive(&pad, 1);
        if (pad.Buttons & PSP_CTRL_CROSS){
            FILE *log = fopen("exception.log", "w");
            if (log != NULL){
                char testo[512];
                sprintf(testo, "Exception details:\n\n");
                fwrite(testo, 1, strlen(testo), log);
                sprintf(testo, "Exception - %s\n", codeTxt[(regs->cause >> 2) & 31]);
                fwrite(testo, 1, strlen(testo), log);
                sprintf(testo, "EPC       - %08X / %s.text + %08X\n", (int)regs->epc, module_info.modname, (unsigned int)(regs->epc-(int)&_ftext));
                fwrite(testo, 1, strlen(testo), log);
                sprintf(testo, "Cause     - %08X\n", (int)regs->cause);
                fwrite(testo, 1, strlen(testo), log);
                sprintf(testo, "Status    - %08X\n", (int)regs->status);
                fwrite(testo, 1, strlen(testo), log);
                sprintf(testo, "BadVAddr  - %08X\n", (int)regs->badvaddr);
                fwrite(testo, 1, strlen(testo), log);
                for(i=0; i<32; i+=4){
                    sprintf(testo, "%s:%08X %s:%08X %s:%08X %s:%08X\n", regName[i], (int)regs->r[i], regName[i+1], (int)regs->r[i+1], regName[i+2], (int)regs->r[i+2], regName[i+3], (int)regs->r[i+3]);
                    fwrite(testo, 1, strlen(testo), log);
                }
                fclose(log);
            }
            break;
        }else if (pad.Buttons & PSP_CTRL_CIRCLE){
            break;
        }
		sceKernelDelayThread(100000);
    }    
    sceKernelExitGame();
}

void initExceptionHandler()
{
   SceKernelLMOption option;
   int args[2], fd, modid;

   memset(&option, 0, sizeof(option));
   option.size = sizeof(option);
   option.mpidtext = PSP_MEMORY_PARTITION_KERNEL;
   option.mpiddata = PSP_MEMORY_PARTITION_KERNEL;
   option.position = 0;
   option.access = 1;

   if((modid = sceKernelLoadModule("exception.prx", 0, &option)) >= 0)
   {
      args[0] = (int)ExceptionHandler;
      args[1] = (int)&exception_regs;
      sceKernelStartModule(modid, 8, args, &fd, NULL);
   }
}
