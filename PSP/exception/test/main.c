#include <pspkernel.h>
#include <pspctrl.h>
#include <pspdebug.h>
#include <pspsdk.h>

#include "../utility/exception.h"
PSP_MODULE_INFO("Exception Handler Test", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);
//PSP_HEAP_SIZE_KB(22000);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Globals:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int runningFlag = 1;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Callbacks:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Exit callback */
int exit_callback(int arg1, int arg2, void *common) {
    runningFlag = 0;
    return 0;
}

/* Callback thread */
int CallbackThread(SceSize args, void *argp) {
    int cbid;
    cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}

/* Sets up the callback thread and returns its thread id */
int SetupCallbacks(void) {
    int thid = 0;
    thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, PSP_THREAD_ATTR_USER, 0);
    if(thid >= 0)
        sceKernelStartThread(thid, 0, 0);
    return thid;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Main:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int main(){
	pspDebugScreenInit();
	SetupCallbacks();

    pspDebugScreenPrintf("Expcetion Handler Test\n");

    initExceptionHandler();
    
    pspDebugScreenPrintf("Press X for a breakpoint\n");
    pspDebugScreenPrintf("Press O for a bus error\n");

    SceCtrlData pad;
    while(runningFlag){
        sceCtrlReadBufferPositive(&pad, 1);
        if (pad.Buttons & PSP_CTRL_CROSS){
			/* Cause a break exception */
			asm(
				"break\r\n"
			  );
        }else if (pad.Buttons & PSP_CTRL_CIRCLE){
			/* Cause a bus error */
			_sw(0, 0);
        }
    }
	sceKernelExitGame();
    return 0;

}
