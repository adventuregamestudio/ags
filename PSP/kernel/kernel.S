	.set noreorder

#include "pspstub.s"

	STUB_START "kernel",0x40090000,0x00030005
	STUB_FUNC  0x33CD1661,kernel_loadExec
	STUB_FUNC  0xA1110D68,kernel_sceKernelRegisterSysEventHandler
	STUB_FUNC  0xABE629DE,kernel_sceKernelUnregisterSysEventHandler
	STUB_END
