/* libcda; BCC/DOS component.
 *
 * Quick port of some old freeware code I found by Barry Egerter
 * (barry.egerter@softnet.com), with our API hacked over the top.
 *
 * Peter Wang <tjaden@users.sf.net> 
 */

#include <dos.h>
#include <io.h>
#include <mem.h>
#include <fcntl.h>
#include "libcda.h"


#define MIN(x,y)     (((x) < (y)) ? (x) : (y))
#define MAX(x,y)     (((x) > (y)) ? (x) : (y))
#define MID(x,y,z)   MAX((x), MIN((y), (z)))


static char _cd_error[256] = "(error strings not available yet)";
const char *cd_error = _cd_error;


#define CDROM 		0x21
#define EJECT_TRAY 	0
#define RESET		2
#define CLOSE_TRAY 	5
#define MEDIA_CHANGE 	9
#define BUSY  		512
#define TRACK_MASK 	208


typedef struct playinfo {
    unsigned char control;
    unsigned char adr;
    unsigned char track;
    unsigned char index;
    unsigned char min;
    unsigned char sec;
    unsigned char frame;
    unsigned char zero;
    unsigned char amin;
    unsigned char asec;
    unsigned char aframe;
};


static struct {
    unsigned short drives;
    unsigned char  first_drive;
    unsigned short current_track;
    unsigned long  track_position;
    unsigned char  track_type;
    unsigned char  low_audio;
    unsigned char  high_audio;
    unsigned char  disk_length_min;
    unsigned char  disk_length_sec;
    unsigned char  disk_length_frames;
    unsigned long	 endofdisk;
    unsigned char  upc[7];
    unsigned char  diskid[6];
    unsigned long  status;
    unsigned short error;	/* bit 15 means error */
} cdrom_data;


static struct {
    unsigned char mode;
    unsigned char input0;
    unsigned char volume0;
    unsigned char input1;
    unsigned char volume1;
    unsigned char input2;
    unsigned char volume2;
    unsigned char input3;
    unsigned char volume3;
} vol;


static int paused;


static union REGS inregs, outregs;
static struct SREGS sregs;


static void device_request(void *block)
{
    inregs.x.ax = 0x1510;
    inregs.x.cx = cdrom_data.first_drive;
    inregs.x.bx = FP_OFF(block);
    sregs.es = FP_SEG(block);
    int86x(0x2f, &inregs, &outregs, &sregs);
}


static void red_book(unsigned long value, unsigned char *min, 
		     unsigned char *sec, unsigned char *frame)
{
    *frame = value & 0x000000ff;
    *sec = (value & 0x0000ff00) >> 8;
    *min = (value & 0x00ff0000) >> 16;
}


static unsigned long hsg(unsigned long value)
{
    unsigned char min, sec, frame;

    red_book (value, &min, &sec, &frame);
    value = (unsigned long)min * 4500;
    value += (short)sec * 75;
    value += frame - 150;
    return value;
}


static unsigned long head_position(void)
{
    struct {
	unsigned char length;
	unsigned char subunit;
	unsigned char comcode;
	unsigned short status;
	char ununsed[8];
	unsigned char media;
	unsigned long address;
	unsigned short bytes;
	unsigned short sector;
	unsigned long  volid;
	unsigned char unused[4];
    } tray_request;

    struct {
	unsigned char mode;
	unsigned char adr_mode;
	unsigned long address;
    } head_data;

    memset(&tray_request, 0, sizeof tray_request);
    tray_request.length = sizeof tray_request;
    tray_request.comcode = 3;
    tray_request.address = (unsigned long)&head_data;
    tray_request.bytes = 6;
    head_data.mode = 0x01;
    head_data.adr_mode = 0x00;
    device_request(&tray_request);
    cdrom_data.error = tray_request.status;

    return head_data.address;
}


static int get_audio_info(void)
{
    struct {
	unsigned char length;
	unsigned char subunit;
	unsigned char comcode;
	unsigned short status;
	char ununsed[8];
	unsigned char media;
	long address;
	short bytes;
	short sector;
	long volid;
    } ioctli;

    struct {
	unsigned char mode;
	unsigned char lowest;
	unsigned char highest;
	unsigned long address;
    } track_data;

    ioctli.length = sizeof ioctli;
    ioctli.subunit = 0;
    ioctli.comcode = 3;
    ioctli.media = 0;
    ioctli.sector = 0;
    ioctli.volid = 0;
    ioctli.address = (long)&track_data;
    ioctli.bytes = sizeof track_data;
    track_data.mode = 0x0a;
    device_request(&ioctli);

    memcpy(&cdrom_data.diskid, &track_data.lowest, 6);
    cdrom_data.low_audio = track_data.lowest;
    cdrom_data.high_audio = track_data.highest;
    red_book(track_data.address, &cdrom_data.disk_length_min, 
	     &cdrom_data.disk_length_sec, &cdrom_data.disk_length_frames);
    cdrom_data.endofdisk = hsg (track_data.address);
    cdrom_data.error = ioctli.status;

    return !(cdrom_data.error & 0x8000);
}


static void set_track(short tracknum)
{
    struct {
	unsigned char length;
	unsigned char subunit;
	unsigned char comcode;
	unsigned short status;
	char ununsed[8];
	unsigned char media;
	unsigned long address;
	unsigned short bytes;
	unsigned short sector;
	unsigned long  volid;
    } tray_request;

    struct {
	unsigned char mode;
	unsigned char track;
	unsigned long address;
	unsigned char control;
    } track_data;

    tray_request.length = sizeof tray_request;
    tray_request.subunit = 0;
    tray_request.comcode = 3;
    tray_request.media = 0;
    tray_request.media = tray_request.sector = tray_request.volid = 0;
    tray_request.address = (unsigned long)&track_data;
    tray_request.bytes = 7;
    track_data.mode = 0x0b;
    track_data.track = tracknum;
    device_request(&tray_request);
    cdrom_data.error = tray_request.status;
    cdrom_data.track_position = hsg(track_data.address);
    cdrom_data.current_track = tracknum;
    cdrom_data.track_type = (track_data.control & TRACK_MASK);
}


static int device_status(void)
{
    struct {
	unsigned char length;
	unsigned char subunit;
	unsigned char comcode;
	unsigned short status;
	char ununsed[8];
	unsigned char media;
	unsigned long address;
	unsigned short bytes;
	unsigned short sector;
	unsigned long  volid;
    } tray_request;

    struct {
	unsigned char mode;
	unsigned long status;
    } cd_data;

    tray_request.length = sizeof tray_request;
    tray_request.subunit = 0;
    tray_request.comcode = 3;
    tray_request.media = 0;
    tray_request.media = tray_request.sector = tray_request.volid = 0;
    tray_request.address = (unsigned long)&cd_data;
    tray_request.bytes = 5;

    cd_data.mode = 0x06;
    device_request (&tray_request);
    cdrom_data.status = cd_data.status;
    cdrom_data.error = tray_request.status;

    return cdrom_data.status;
}


static int audio_busy(void)
{
    if (device_status() & 1)	/* door open */
	return 0;
    return (cdrom_data.error & BUSY);
}


static void stop(void)
{
    struct {
	unsigned char length;
	unsigned char subunit;
	unsigned char comcode;
	unsigned short status;
	char ununsed[8];
    } stop_request;

    stop_request.length = sizeof stop_request;
    stop_request.subunit = 0;
    stop_request.comcode = 133;
    device_request(&stop_request);
    cdrom_data.error = stop_request.status;
}


static int play(unsigned long begin, unsigned long end)
{
    unsigned long leng;

    struct {
	unsigned char length;
	unsigned char subunit;
	unsigned char comcode;
	unsigned short status;
	char ununsed[8];
	unsigned char addressmode;
	unsigned long start;
	unsigned long playlength;
    } play_request;
    
    stop();

    play_request.length = sizeof play_request;
    play_request.subunit = 0;
    play_request.comcode = 132;
    play_request.addressmode = 0;
    play_request.start = begin;
    play_request.playlength = end - begin;
    device_request(&play_request);
    cdrom_data.error = play_request.status;

    paused = 0;
    return !(cdrom_data.error & 0x8000);
}


static void cd_cmd(unsigned char mode)
{
    struct {
	unsigned char length;
	unsigned char subunit;
	unsigned char comcode;
	unsigned short status;
	char ununsed[8];
	unsigned char media;
	unsigned long address;
	unsigned short bytes;
	unsigned char unused[4];
    } tray_request;

    unsigned char cd_mode;

    cd_mode = mode;
    tray_request.length = sizeof tray_request;
    tray_request.subunit = 0;
    tray_request.comcode = 12;
    tray_request.media = 0;
    tray_request.address = (unsigned long)&cd_mode;
    tray_request.bytes = 1;
    device_request(&tray_request);
    cdrom_data.error = tray_request.status;
}



/*
 * libcda interface
 */

int cd_init(void)
{
    inregs.h.ah = 0x15;
    inregs.h.al = 0x00;
    inregs.x.bx = 0;
    int86 (0x2f, &inregs, &outregs);
    if (outregs.x.bx == 0)
	return -1;

    cdrom_data.drives = outregs.x.bx;
    cdrom_data.first_drive = outregs.x.cx;
    get_audio_info();

    paused = 0;
    return 0;
}


void cd_exit(void)
{     
    /* do nothing */
}



static unsigned long start_position(int start)
{
    set_track(start);
    return cdrom_data.track_position;
}


static unsigned long end_position(int end)
{
    if (end == cdrom_data.high_audio)
	return cdrom_data.endofdisk;
    else {
	set_track(end + 1);
	return cdrom_data.track_position;
    }
}


int cd_play(int track)
{
    unsigned long startpos, endpos;

    if ((!get_audio_info()) ||
	(track < cdrom_data.low_audio) || 
	(track > cdrom_data.high_audio))
	return -1;

    startpos = start_position(track);
    endpos = end_position(track);
    return (!play(startpos, endpos)) ? -1 : 0;
}


int cd_play_range(int start, int end)
{
    unsigned long startpos, endpos;

    if ((!get_audio_info()) ||
	(MIN(start, end) < cdrom_data.low_audio) || 
	(MAX(start, end) > cdrom_data.high_audio))
	return -1;
     
    startpos = start_position(start);
    endpos = end_position(end);
    return (!play(startpos, endpos)) ? -1 : 0;
}


int cd_play_from(int track)
{
    unsigned long startpos, endpos;
  
    if (!get_audio_info() ||
	(track < cdrom_data.low_audio) || 
	(track > cdrom_data.high_audio))
	return -1;
     
    startpos = start_position(track);
    endpos = cdrom_data.endofdisk;
    return (!play(startpos, endpos)) ? -1 : 0;
}


int cd_current_track(void)
{
    unsigned long loc;
    short i;
    
    if (!audio_busy() || !get_audio_info())
	return 0;

    loc = head_position();
    for (i = cdrom_data.high_audio; i > cdrom_data.low_audio; i--) {
	set_track(i);
	if (loc > cdrom_data.track_position)
	    return i;
    }
    
    return cdrom_data.low_audio;
}


void cd_pause(void)
{
    if (!paused) {
	stop();
	paused = 1;
    }
}


void cd_resume(void)
{
    struct {
	unsigned char length;
	unsigned char subunit;
	unsigned char comcode;
	unsigned short status;
	char ununsed[8];
    } stop_request;

    stop_request.length = sizeof stop_request;
    stop_request.subunit = 0;
    stop_request.comcode = 136;
    device_request(&stop_request);
    cdrom_data.error = stop_request.status;

    paused = 0;
}


int cd_is_paused(void)
{
    return paused;
}


void cd_stop(void)
{
    stop();	/* Two stops is correct. */
    stop();
    paused = 0;
}


int cd_get_tracks(int *first, int *last)
{
    if (!get_audio_info())
	return -1;
    if (first) *first = cdrom_data.low_audio;
    if (last)  *last  = cdrom_data.high_audio;
    return 0;
}


int cd_is_audio(int track)
{
    if (!get_audio_info())
	return -1;

    set_track(track);
    return (cdrom_data.track_type == 64) ? 0 : 1;
}


void cd_get_volume(int *c0, int *c1)
{
    struct {
	unsigned char length;
	unsigned char subunit;
	unsigned char comcode;
	unsigned short status;
	char ununsed[8];
	unsigned char media;
	unsigned long address;
	unsigned short bytes;
	unsigned short sector;
	unsigned long  volid;
    } tray_request;

    memset(&tray_request, 0, sizeof tray_request);
    tray_request.length = sizeof tray_request;
    tray_request.comcode = 3;
    tray_request.address = (unsigned long)&vol;
    tray_request.bytes = sizeof vol;
    vol.mode = 4;
    device_request(&tray_request);

    if (c0) *c0 = vol.volume0;
    if (c1) *c1 = vol.volume1;
}


void cd_set_volume(int c0, int c1)
{
    struct {
	unsigned char length;
	unsigned char subunit;
	unsigned char comcode;
	unsigned short status;
	char ununsed[8];
	unsigned char media;
	unsigned long address;
	unsigned short bytes;
	unsigned char unused[4];
    } cd_request;

    vol.mode = 3;
    vol.volume0 = MID(0, c0, 255);  
    vol.volume1 = MID(0, c1, 255);  
    vol.volume2 = 0;  
    vol.volume3 = 0; 
    vol.input0 = 0;
    vol.input1 = 1;
    vol.input2 = 2;
    vol.input3 = 3;

    cd_request.length = sizeof cd_request;
    cd_request.subunit = 0;
    cd_request.comcode = 12;
    cd_request.media = 0;
    cd_request.address = (unsigned long)&vol;
    cd_request.bytes = 9;
    device_request(&cd_request);
}


void cd_eject(void)
{
    cd_cmd(EJECT_TRAY);
    paused = 0;
}


void cd_close(void)
{
    cd_cmd(CLOSE_TRAY);
    paused = 0;
}
