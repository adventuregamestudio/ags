	.set noreorder

#include "pspstub.s"

	STUB_START "kernel",0x40090000,0x00010005
	STUB_FUNC  0x33CD1661,kernel_loadExec
	STUB_END
