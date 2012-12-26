/*
  Partial rewrite of newlib I/O functions to allow suspend by JJS.
  
  The idea is described in this post by Jim Paris on ps2dev:
  http://forums.ps2dev.org/viewtopic.php?p=87576#87576

  Implementation here only covers actual files.
  
  Usage is simple. Just link with this file and place a call to
  suspend_init() at the start of your application.
  
  This is the file header from PSP newlib (from MINPSPW):
*/
/*
 * PSP Software Development Kit - http://www.pspdev.org
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in PSPSDK root for details.
 *
 * libcglue.c - Newlib-compatible system calls.
 *
 * Copyright (c) 2005 Marcus R. Brown <mrbrown@ocgnet.org>
 * Copyright (c) 2005 James Forshaw <tyranid@gmail.com>
 * Copyright (c) 2005 John Kelley <ps2dev@kelley.ca>
 * Copyright (c) 2005 Jim Paris <jim@jtan.com>
 * 
 */

#include <pspsdk.h>
#include <psppower.h>
#include <pspkernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>

#define __PSP_FILENO_MAX 1024
#define MAXPATHLEN 1024

#define __PSP_IS_FD_VALID(FD) \
	( (FD >= 0) && (FD < __PSP_FILENO_MAX) && (__psp_descriptormap[FD] != NULL) )

typedef struct {
	char * filename;
	u8     type;
	u32    sce_descriptor;
	u32    flags;
	u32    ref_count;
} __psp_descriptormap_type;

typedef enum {
	__PSP_DESCRIPTOR_TYPE_FILE,
	__PSP_DESCRIPTOR_TYPE_PIPE,
	__PSP_DESCRIPTOR_TYPE_SOCKET,
	__PSP_DESCRIPTOR_TYPE_TTY
} __psp_fdman_fd_types;

extern __psp_descriptormap_type *__psp_descriptormap[__PSP_FILENO_MAX];
extern int __psp_path_absolute(const char *in, char *out, int len);
extern int __psp_fdman_get_new_descriptor();
extern int __psp_set_errno(int code);
extern int __psp_pipe_nonblocking_read(int fd, void *buf, size_t len);
extern int __psp_pipe_read(int fd, void *buf, size_t len);
extern int __psp_pipe_write(int fd, const void *buf, size_t size);
extern int __psp_pipe_nonblocking_write(int fd, const void *buf, size_t len);
extern ssize_t __psp_socket_send(int s, const void *buf, size_t len, int flags) __attribute__((weak));
extern ssize_t __psp_socket_recv(int s, void *buf, size_t len, int flags) __attribute__((weak));

volatile int offsets[__PSP_FILENO_MAX];
volatile int suspending = 0;
volatile int resumed = 0;
volatile int descriptors_are_valid = 1; /* Descriptors are not updated until the next file operation. */


/* Close, reopen and seek all files that were opened before going into suspend mode. */
void reopen_files()
{
	int i;
	for (i = 3; i < __PSP_FILENO_MAX; i++) { // 0,1,2 are owned by the kernel
		if ((__psp_descriptormap[i]) && (offsets[i] > -1)) {
			int result = sceIoClose(__psp_descriptormap[i]->sce_descriptor);
			printf("Closing file %p with result %p\n", i, result);
			if (result > -1) {
				__psp_descriptormap[i]->sce_descriptor = sceIoOpen(__psp_descriptormap[i]->filename, get_sce_flags(__psp_descriptormap[i]->flags, 1), 0777);
				result = sceIoLseek32(__psp_descriptormap[i]->sce_descriptor, offsets[i], PSP_SEEK_SET);
				printf("Reopened file %p with result %p\n", i, __psp_descriptormap[i]->sce_descriptor);
				printf("Seeked file %p to position %p with result %p\n", i, offsets[i], result);
			}
		}
	}
	descriptors_are_valid = 1;
	resumed = 0;
	suspending = 0;
}


int get_sce_flags(int flags, int modify_for_resuming)
{
    int sce_flags;

	/* O_RDONLY starts at 0, where PSP_O_RDONLY starts at 1, so remap the read/write
	   flags by adding 1. */
	sce_flags = (flags & O_ACCMODE) + 1;

	/* Translate standard open flags into the flags understood by the PSP kernel. */
	if (flags & O_APPEND) {
		sce_flags |= PSP_O_APPEND;
	}
	if (flags & O_CREAT) {
		sce_flags |= PSP_O_CREAT;
	}
	
	/* Don' truncate files on reopening. */
	if (!modify_for_resuming && (flags & O_TRUNC)) {
		sce_flags |= PSP_O_TRUNC;
	}
	
	if (flags & O_EXCL) {
		sce_flags |= PSP_O_EXCL;
	}
	if (flags & O_NONBLOCK) {
		sce_flags |= PSP_O_NBLOCK;
	}
	
	return sce_flags;
}


off_t _lseek(int fd, off_t offset, int whence)
{
	if (!__PSP_IS_FD_VALID(fd)) {
		errno = EBADF;
		return -1;
	}

	if (__psp_descriptormap[fd]->type != __PSP_DESCRIPTOR_TYPE_FILE) {
		errno = EBADF;
		return -1;
	}
	else {
		/* We don't have to do anything with the whence argument because SEEK_* == PSP_SEEK_*. */
		while (suspending) {
			sceKernelDelayThread(1000 * 1000);
			printf("Waiting in seek...\n");

			if (resumed) {
				resumed = 0;
				printf("Resuming from seek...\n");
				reopen_files();
			}			
		}	
		return (off_t) __psp_set_errno(sceIoLseek(__psp_descriptormap[fd]->sce_descriptor, offset, whence));
	}
}


int _open(const char *name, int flags, int mode)
{
	int scefd, fd;
	int sce_flags;
	char dest[MAXPATHLEN + 1];

	if(__psp_path_absolute(name, dest, MAXPATHLEN) < 0) {
		errno = ENAMETOOLONG;
		return -1;
	}

	sce_flags = get_sce_flags(flags, 0);
	
	while (suspending) {
		sceKernelDelayThread(1000 * 1000);
		printf("Waiting in open...\n");

		if (resumed) {
		    resumed = 0;
			printf("Resuming from open...\n");
			reopen_files();
		}
	}
		
	scefd = sceIoOpen(dest, sce_flags, mode);
	if (scefd >= 0) {
		fd = __psp_fdman_get_new_descriptor();
		if (fd != -1) {
			__psp_descriptormap[fd]->sce_descriptor = scefd;
			__psp_descriptormap[fd]->type     		= __PSP_DESCRIPTOR_TYPE_FILE;
			__psp_descriptormap[fd]->flags    		= flags;
			__psp_descriptormap[fd]->filename 		= strdup(dest);
			return fd;
		}
		else {
			sceIoClose(scefd);
			errno = ENOMEM;
			return -1;
		}
	} 
	else {
		return __psp_set_errno(scefd);
	}
	
}


int _read(int fd, void *buf, size_t size)
{
	if (!__PSP_IS_FD_VALID(fd)) {
		errno = EBADF;
		return -1;
	}

	switch(__psp_descriptormap[fd]->type)
	{
		case __PSP_DESCRIPTOR_TYPE_FILE:
		{
			while (suspending) {
				sceKernelDelayThread(1000 * 1000);
				printf("Waiting in read...\n");
				
				if (resumed) {
					resumed = 0;
					printf("Resuming from read...\n");
					reopen_files();
				}
			}
			return __psp_set_errno(sceIoRead(__psp_descriptormap[fd]->sce_descriptor, buf, size));
			break;
		}
		case __PSP_DESCRIPTOR_TYPE_TTY:
			return __psp_set_errno(sceIoRead(__psp_descriptormap[fd]->sce_descriptor, buf, size));
			break;
		case __PSP_DESCRIPTOR_TYPE_PIPE:
			if (__psp_descriptormap[fd]->flags & O_NONBLOCK) {
				return __psp_pipe_nonblocking_read(fd, buf, size);
			}
			else {
				return __psp_pipe_read(fd, buf, size);
			}
			break;
		case __PSP_DESCRIPTOR_TYPE_SOCKET:
			if (__psp_socket_recv != NULL) {
				return __psp_socket_recv(fd, buf, size, 0);
			}
			break;
		default:
			break;
	}

	errno = EBADF;
	return -1;
}


int _write(int fd, const void *buf, size_t size)
{
	if (!__PSP_IS_FD_VALID(fd)) {
		errno = EBADF;
		return -1;
	}

	switch(__psp_descriptormap[fd]->type)
	{
		case __PSP_DESCRIPTOR_TYPE_FILE:
		{
			while (suspending) {
				sceKernelDelayThread(1000 * 1000);
				printf("Waiting in write...\n");
				
				if (resumed) {
					resumed = 0;
					printf("Resuming from write...\n");
					reopen_files();
				}
			}
			return __psp_set_errno(sceIoWrite(__psp_descriptormap[fd]->sce_descriptor, buf, size));
			break;
		}
		case __PSP_DESCRIPTOR_TYPE_TTY:
			return __psp_set_errno(sceIoWrite(__psp_descriptormap[fd]->sce_descriptor, buf, size));
			break;
	
		case __PSP_DESCRIPTOR_TYPE_PIPE:
			if (__psp_descriptormap[fd]->flags & O_NONBLOCK) {
				return __psp_pipe_nonblocking_write(fd, buf, size);
			}
			else {
				return __psp_pipe_write(fd, buf, size);
			}
			break;
			break;
		case __PSP_DESCRIPTOR_TYPE_SOCKET:
			if (__psp_socket_send != NULL) {
				return __psp_socket_send(fd, buf, size, 0);
			}
			break;
		default:
			break;
	}

	errno = EBADF;
	return -1;
}



int suspend_power_callback(int unknown, int pwrflags, void *common)
{
	int i;
	if (pwrflags & PSP_POWER_CB_RESUME_COMPLETE) {
		// The delay might help with avoiding the 0x80020328 (driver deleted) error.
		// It occurs when calling sceIo* functions shortly after resuming.
		sceKernelDelayThread(1000 * 500);
		resumed = 1;
	}
	else if (pwrflags & PSP_POWER_CB_SUSPENDING) {
		suspending = 1;
		if (descriptors_are_valid)
		{
			for (i = 3; i < __PSP_FILENO_MAX; i++) {
				offsets[i] = -1;
				if (__psp_descriptormap[i]) {
					offsets[i] = sceIoLseek32(__psp_descriptormap[i]->sce_descriptor, 0, PSP_SEEK_CUR);
				}
			}
		}
		descriptors_are_valid = 0;
	}

	return 0;
}


int suspend_callback_thread(SceSize args, void *argp)
{
	int cbid;

	cbid = sceKernelCreateCallback("Power Callback", suspend_power_callback, NULL);
	scePowerRegisterCallback(-1, cbid);
	sceKernelSleepThreadCB();

	return 0;
}


/* Call this function from somewhere in your application to set up the callbacks. */
void suspend_init()
{
	memset((void*)offsets, 0, sizeof(int) * __PSP_FILENO_MAX);

	SceUID thid = sceKernelCreateThread("suspend_update_thread", suspend_callback_thread, 0x11, 0xFA0, THREAD_ATTR_USER, 0);
	if(thid > -1) {
		sceKernelStartThread(thid, 0, 0);
	}
}
