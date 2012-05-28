/* libcda; djgpp component.
 *
 * Warped from bcd.c -- Brennan's CD-ROM Audio Playing Library, by
 * Brennan Underwood (v1.3).  Has been melded to.
 *
 * Peter Wang <tjaden@users.sf.net> 
 */

#include <dos.h>
#include <dpmi.h>
#include <go32.h>
#include <fcntl.h>
#include <malloc.h>
#include <unistd.h>
#include <strings.h>
#include "libcda.h"


#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif


#define MIN(x,y)     (((x) < (y)) ? (x) : (y))
#define MAX(x,y)     (((x) > (y)) ? (x) : (y))
#define MID(x,y,z)   MAX((x), MIN((y), (z)))



typedef struct {
    int is_audio;
    int start, end, len;
} Track;


static char _cd_error[256] = "(error strings not available yet)";
const char *cd_error = _cd_error;


static int first_drive;
static int lowest_track, highest_track;
static Track *tracks;

static int dos_mem_segment, dos_mem_selector = -1;
static int status, error, error_code;

#define RESETerror (error = error_code = 0)
#define ERROR_BIT (1 << 15)
#define BUSY_BIT (1 << 9)


#pragma pack(1)

/* I know 'typedef struct {} bleh' is a bad habit, but... */
typedef struct {
    unsigned char len __attribute__((packed));
    unsigned char unit __attribute__((packed));
    unsigned char command __attribute__((packed));
    unsigned short status __attribute__((packed));
    unsigned char reserved[8] __attribute__((packed));
} RequestHeader;

typedef struct {
    RequestHeader request_header __attribute__((packed));
    unsigned char descriptor __attribute__((packed));
    unsigned long address __attribute__((packed));
    unsigned short len __attribute__((packed));
    unsigned short secnum __attribute__((packed));
    unsigned long ptr __attribute__((packed));
} IOCTLI;

typedef struct {
    unsigned char control __attribute__((packed));
    unsigned char lowest __attribute__((packed));
    unsigned char highest __attribute__((packed));
    unsigned char total[4] __attribute__((packed));
} DiskInfo;

typedef struct {
    unsigned char control __attribute__((packed));
    unsigned char track_number __attribute__((packed));
    unsigned char start[4] __attribute__((packed));
    unsigned char info __attribute__((packed));
} TrackInfo;

typedef struct {
    RequestHeader request __attribute__((packed));
    unsigned char mode __attribute__((packed));
    unsigned long start __attribute__((packed));
    unsigned long len __attribute__((packed));
} PlayRequest;

typedef struct {
    RequestHeader request __attribute__((packed));
} StopRequest;

typedef struct {
    RequestHeader request __attribute__((packed));
} ResumeRequest;

typedef struct {
    unsigned char control __attribute__((packed));
    unsigned char input0 __attribute__((packed));
    unsigned char volume0 __attribute__((packed));
    unsigned char input1 __attribute__((packed));
    unsigned char volume1 __attribute__((packed));
    unsigned char input2 __attribute__((packed));
    unsigned char volume2 __attribute__((packed));
    unsigned char input3 __attribute__((packed));
    unsigned char volume3 __attribute__((packed));
} VolumeRequest;

typedef struct {
    unsigned char control __attribute__((packed));
    unsigned char fn __attribute__((packed));
} LockRequest;

typedef struct {
    unsigned char control __attribute__((packed));
    unsigned char mbyte __attribute__((packed));
} MediaChangedRequest;

typedef struct {
    unsigned char control __attribute__((packed));
    unsigned long status __attribute__((packed));
} StatusRequest;

typedef struct {
    unsigned char control __attribute__((packed));
    unsigned char mode __attribute__((packed));
    unsigned long loc __attribute__((packed));
} PositionRequest;

#pragma pack()


/* DOS IOCTL w/ command block */
static void _ioctl(IOCTLI * ioctli, void *command, int len)
{
    int ioctli_len = sizeof(IOCTLI);
    unsigned long command_address = dos_mem_segment << 4;
    __dpmi_regs regs;

    memset(&regs, 0, sizeof regs);
    regs.x.es = (__tb >> 4) & 0xffff;
    regs.x.ax = 0x1510;
    regs.x.bx = __tb & 0xf;
    regs.x.cx = first_drive;
    ioctli->address = dos_mem_segment << 16;
    ioctli->len = len;
    dosmemput(ioctli, ioctli_len, __tb);	/* put ioctl into dos area */
    dosmemput(command, len, command_address);	/* and command too */
    if (__dpmi_int(0x2f, &regs) == -1)
	return;

    dosmemget(__tb, ioctli_len, ioctli);	/* retrieve results */
    dosmemget(command_address, len, command);
    status = ioctli->request_header.status;
    if (status & ERROR_BIT) {
	error = TRUE;
	error_code = status & 0xff;
    }
    else {
	error = FALSE;
	error_code = 0;
    }
}


/* no command block */
static void _ioctl2(void *cmd, int len)
{
    __dpmi_regs regs;
    memset(&regs, 0, sizeof regs);
    regs.x.es = (__tb >> 4) & 0xffff;
    regs.x.ax = 0x1510;
    regs.x.bx = __tb & 0xf;
    regs.x.cx = first_drive;
    dosmemput(cmd, len, __tb);	/* put ioctl block in dos arena */
    if (__dpmi_int(0x2f, &regs) == -1) 
	return;

    /* I hate to have no error capability for ioctl2 but the command block
       doesn't necessarily have a status field */
    RESETerror;
}


static int red2hsg(char *r)
{
    return r[0] + r[1] * 75 + r[2] * 4500 - 150;
}


#define BCD_DOOR_OPEN		1
#define BCD_DOOR_UNLOCKED	2
#define BCD_SUPPORT_COOKED	4
#define BCD_READ_ONLY		8
#define BCD_DATA_READ_ONLY	16
#define BCD_SUPPORT_INTERLEAVE	32

static int device_status()
{
    IOCTLI ioctli;
    StatusRequest req;

    memset(&ioctli, 0, sizeof ioctli);
    memset(&req, 0, sizeof req);
    ioctli.request_header.len = sizeof ioctli;

    ioctli.request_header.command = 3;
    ioctli.len = sizeof req;
    req.control = 6;
    _ioctl(&ioctli, &req, sizeof req);
    return req.status;
}


static int get_status_word()
{
    IOCTLI ioctli;
    DiskInfo disk_info;

    /* get cd info as an excuse to get a look at the status word */
    memset(&disk_info, 0, sizeof disk_info);
    memset(&ioctli, 0, sizeof ioctli);

    ioctli.request_header.len = 26;
    ioctli.request_header.command = 3;
    ioctli.len = 7;
    disk_info.control = 10;
    _ioctl(&ioctli, &disk_info, sizeof disk_info);
    return status;
}


/* Internal: If the door is open, then the head is busy, and so the
 * busy bit is on. It is not, however, playing audio. */
static int audio_busy()
{
    if (device_status() & BCD_DOOR_OPEN)
	return 0;

    get_status_word();
    if (error)
	return -1;
    return (status & BUSY_BIT) ? 1 : 0;
}


/* Internal function to get current audio position */
static int audio_position()
{
    IOCTLI ioctli;
    PositionRequest req;

    memset(&ioctli, 0, sizeof ioctli);
    memset(&req, 0, sizeof req);
    ioctli.request_header.len = sizeof ioctli;
    ioctli.request_header.command = 3;
    ioctli.len = sizeof req;
    req.control = 1;
    _ioctl(&ioctli, &req, sizeof req);
    return req.loc;
}


/* Internal function to get track info */
static void get_track_info(int n, Track *t)
{
    IOCTLI ioctli;
    TrackInfo info;

    memset(&ioctli, 0, sizeof ioctli);
    memset(&info, 0, sizeof info);
    ioctli.request_header.len = sizeof ioctli;
    ioctli.request_header.command = 3;
    info.control = 11;
    info.track_number = n;
    _ioctl(&ioctli, &info, sizeof info);

    t->start = red2hsg(info.start);
    t->is_audio = (info.info & 64) ? 0 : 1;
}


/* Internal function to get disc info */
static int get_audio_info()
{
    IOCTLI ioctli;
    DiskInfo disk_info;
    int i;

    if (tracks) {
	free(tracks);
	tracks = NULL;
    }

    memset(&disk_info, 0, sizeof disk_info);
    memset(&ioctli, 0, sizeof ioctli);

    ioctli.request_header.len = 26;
    ioctli.request_header.command = 3;
    ioctli.len = 7;
    disk_info.control = 10;
    _ioctl(&ioctli, &disk_info, sizeof disk_info);
    if (error)
	return 0;

    lowest_track = disk_info.lowest;
    highest_track = disk_info.highest;

    /* alloc max space in order to attempt to avoid possible overrun bug */
    tracks = calloc(highest_track + 1, sizeof(Track));
    if (!tracks) return 0;

    /* get track starts */
    for (i = lowest_track; i <= highest_track; i++)
	get_track_info(i, tracks + i);

    /* figure out track ends */
    for (i = lowest_track; i < highest_track; i++)
	tracks[i].end = tracks[i + 1].start - 1;
    tracks[i].end = red2hsg(disk_info.total);

    for (i = lowest_track; i <= highest_track; i++)
	tracks[i].len = tracks[i].end - tracks[i].start;

    return 1;
}


/* Handles the setup for CD-ROM audio interface. */
int cd_init()
{
    __dpmi_regs regs;

    /* disk I/O wouldn't work anyway if you set sizeof tb this low, but... */
    if (_go32_info_block.size_of_transfer_buffer < 4096)
	return -1;

    /* check for mscdex */
    memset(&regs, 0, sizeof regs);
    regs.x.ax = 0x1500;
    regs.x.bx = 0x0;
    __dpmi_int(0x2f, &regs);
    if (regs.x.bx == 0) /* abba no longer lives */
	return -1;

    first_drive = regs.x.cx;	/* use the first drive */

    /* check for mscdex at least 2.0 */
    memset(&regs, 0, sizeof regs);
    regs.x.ax = 0x150C;
    __dpmi_int(0x2f, &regs);
    if (regs.x.bx == 0)
	return -1;

    /* allocate 256 bytes of dos memory for the command blocks */
    if ((dos_mem_segment = __dpmi_allocate_dos_memory(16, &dos_mem_selector)) < 0)
	return -1;

    return 0;
}


/* Shuts down CD-ROM audio interface */
void cd_exit()
{
    if (dos_mem_selector != -1) {
	__dpmi_free_dos_memory(dos_mem_selector);
	dos_mem_selector = -1;
    }
    
    if (tracks) {
	free(tracks);
	tracks = NULL;
    }
}


#if 0 /* UNUSED */
int cd_reset()
{
    IOCTLI ioctli;
    char reset = 2;

    memset(&ioctli, 0, sizeof ioctli);
    ioctli.request_header.len = sizeof ioctli;
    ioctli.request_header.command = 12;
    ioctli.len = 1;
    _ioctl(&ioctli, &reset, sizeof reset);
    if (error)
	return 0;
    return 1;
}
#endif


/* Get first and last track numbers. */
int cd_get_tracks(int *first, int *last)
{
    if (!get_audio_info())
	return -1;

    if (first) *first = lowest_track;
    if (last)  *last  = highest_track;
    return 0;
}


/* Return non-zero is track contains audio. */
int cd_is_audio(int trackno)
{
    if (!get_audio_info() ||
	(trackno < lowest_track || trackno > highest_track))
	return -1;

    return tracks[trackno].is_audio ? 1 : 0;
}


/* Get CD playback volume (left & right channels). */
void cd_get_volume(int *c0, int *c1)
{
    IOCTLI ioctli;
    VolumeRequest v;

    memset(&ioctli, 0, sizeof ioctli);
    ioctli.request_header.len = sizeof ioctli;
    ioctli.request_header.command = 3;
    ioctli.len = sizeof v;
    v.control = 4;
    _ioctl(&ioctli, &v, sizeof v);

    if (c0) *c0 = v.volume0;
    if (c1) *c1 = v.volume1;
}


/* Set CD playback volume (0-255) */
void cd_set_volume(int c0, int c1)
{
    IOCTLI ioctli;
    VolumeRequest v;

    memset(&ioctli, 0, sizeof ioctli);
    ioctli.request_header.len = sizeof ioctli;
    ioctli.request_header.command = 12;
    ioctli.len = sizeof v;
    v.control = 3;
    v.volume0 = MID(0, c0, 255);  
    v.volume1 = MID(0, c1, 255);  
    v.volume2 = 0;  
    v.volume3 = 0; 
    v.input0 = 0;
    v.input1 = 1;
    v.input2 = 2;
    v.input3 = 3;

    _ioctl(&ioctli, &v, sizeof v);
}


/* Internal: low-level play function */
static int play(int location, int frames)
{
    PlayRequest cmd;

    if (audio_busy())
	cd_stop();

    memset(&cmd, 0, sizeof cmd);
    cmd.request.len = sizeof cmd;
    cmd.request.command = 132;
    cmd.start = location;
    cmd.len = frames;
    _ioctl2(&cmd, sizeof cmd);

    return error ? -1 : 0;
}


/* Play a single track. */
int cd_play(int trackno)
{
    if ((!get_audio_info()) ||
	(trackno < lowest_track || trackno > highest_track) ||
	(!tracks[trackno].is_audio))
	return -1;
    
    return play(tracks[trackno].start, tracks[trackno].len);
}


/* Play range of tracks. */
int cd_play_range(int start, int end)
{
    int i, len = 0;

    if ((!get_audio_info()) ||
	(MIN(start, end) < lowest_track || 
	 MAX(start, end) > highest_track)) 
	return -1;

    for (i = start; i <= end; i++) {
	if (!tracks[i].is_audio)
	    return -1;
	len += tracks[i].len;
    }

    return play(tracks[start].start, len);
}


/* Play from track to end of disc. */
int cd_play_from(int track)
{
    if (!get_audio_info())
	return -1;

    return cd_play_range(track, highest_track);
}


/* Return currently-playing track no. */
int cd_current_track()
{
    int i, loc;
    
    if ((!audio_busy()) ||
	(tracks == NULL && !get_audio_info()))
	return 0;

    loc = audio_position();
    for (i = lowest_track; i <= highest_track; i++) 
	if (loc >= tracks[i].start && loc <= tracks[i].end)
	    return i;
    
    /* some bizarre location? */
    return 0;
}


/* Internal: audio stop request.
 * Call once for Pause, twice for Stop. */
static void stop()
{
    StopRequest cmd;

    memset(&cmd, 0, sizeof cmd);
    cmd.request.len = sizeof cmd;
    cmd.request.command = 133;
    _ioctl2(&cmd, sizeof cmd);
}


/* Pause playback. */
void cd_pause()
{
    if (!cd_is_paused()) 
	stop();
}


/* Resume playback. */
void cd_resume()
{
    ResumeRequest cmd;

    memset(&cmd, 0, sizeof cmd);
    cmd.request.len = sizeof cmd;
    cmd.request.command = 136;
    _ioctl2(&cmd, sizeof cmd);
}


/* Return non-zero is playback is paused. */
int cd_is_paused()
{
    IOCTLI ioctli;
    StatusRequest req;

    memset(&ioctli, 0, sizeof ioctli);
    memset(&req, 0, sizeof req);
    ioctli.request_header.len = sizeof ioctli;

    ioctli.request_header.command = 3;
    ioctli.len = sizeof req;
    req.control = 15;
    _ioctl(&ioctli, &req, sizeof req);
    return req.status & 1;
}


/* Stop playback. */
void cd_stop()
{
    stop();	/* This is not a hack. */
    stop();
}


/* Eject CD tray. */
void cd_eject()
{
    IOCTLI ioctli;
    char eject = 0;

    memset(&ioctli, 0, sizeof ioctli);
    ioctli.request_header.len = sizeof ioctli;
    ioctli.request_header.command = 12;
    ioctli.len = 1;
    _ioctl(&ioctli, &eject, sizeof eject);
}


/* Close CD tray. */
void cd_close()
{
    IOCTLI ioctli;
    char closeit = 5;

    memset(&ioctli, 0, sizeof ioctli);
    ioctli.request_header.len = sizeof ioctli;
    ioctli.request_header.command = 12;
    ioctli.len = 1;
    _ioctl(&ioctli, &closeit, sizeof closeit);
}
