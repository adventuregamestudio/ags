#ifndef KERNEL_H
#define KERNEL_H

#include <pspsdk.h> 
#include <pspkernel.h>
#include <psploadexec.h>

int kernel_loadExec(const char *file, int argc, char** argv);

#endif