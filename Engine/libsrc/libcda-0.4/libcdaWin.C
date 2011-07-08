/* libcda; Windows component.
 *
 * Using the string interface is probably slightly slower, but damned
 * if I'm going to code using the message interface.
 *
 * Peter Wang <tjaden@psynet.net> 
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <windows.h>
#include <mmsystem.h>
#include "libcda.h"


static char _cd_error[256];
const char *cd_error = _cd_error;


/* Hack. */
static int paused = 0;
static char paused_pos[20];
static char end_pos[20];


/* internal: Even the command string interface needs shielding. */

static char ret[256];

static int command(char *fmt, ...)
{
    char buf[256];
    va_list ap;
    DWORD err;
    
    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    va_end(ap);

    err = mciSendString(buf, ret, sizeof ret, 0);
    if (err) 
	mciGetErrorString(err, _cd_error, sizeof _cd_error);
    return err ? -1 : 0;
}


int cd_init(void)
{
    int err;

    err = command("open cdaudio wait");
    if (!err) 
	err = command("set cdaudio time format tmsf");

    paused = 0;
    return err;
}


void cd_exit(void)
{
    command("close cdaudio");
}


/* internal helpers */

#define startof(track)	(MCI_MAKE_TMSF(track, 0, 0, 0))

static char *lengthof(int track)
{
    command("status cdaudio length track %u", track);
    return ret;
}


int cd_play(int track)
{
    cd_stop();
    sprintf(end_pos, "%u:%s", track, lengthof(track));
    return command("play cdaudio from %lu to %s", startof(track), end_pos);
}


int cd_play_range(int start, int end)
{
    cd_stop();
    sprintf(end_pos, "%u:%s", end, lengthof(end));
    return command("play cdaudio from %lu to %s", startof(start), end_pos);
}


int cd_play_from(int track)
{
    cd_stop();
    end_pos[0] = 0;
    return command("play cdaudio from %lu", startof(track));
}


int cd_current_track(void)
{
    if ((command("status cdaudio mode") != 0) || 
	(strcmp(ret, "playing") != 0))
	return 0;
	
    if (command("status cdaudio current track") != 0)
	return 0;
    return atoi(ret);
}


void cd_pause(void)
{
    /* `pause cdaudio' works like `stop' with the MCICDA driver.
     * Therefore we hack around it.
     */
    mciSendString("status cdaudio position", paused_pos, sizeof paused_pos, 0);
    command("pause cdaudio");
    paused = 1;
}


void cd_resume(void)
{
    if (!paused)
	return;

    if (end_pos[0])
	command("play cdaudio from %s to %s", paused_pos, end_pos);
    else
	command("play cdaudio from %s", paused_pos);
    paused = 0;
}


int cd_is_paused(void)
{
    return paused;
}


void cd_stop(void)
{
    command("stop cdaudio wait");
    paused = 0;
}


int cd_get_tracks(int *first, int *last)
{
    int i;
    
    if (command("status cdaudio number of tracks") != 0)
	return -1;

    i = atoi(ret);
    
    if (first) *first = 1;
    if (last) *last = i;
    
    return (i) ? 0 : -1;
}


int cd_is_audio(int track)
{
    if (command("status cdaudio type track %u", track) != 0)
	return -1;
    return (strcmp(ret, "audio") == 0) ? 1 : 0;
}


void cd_get_volume(int *c0, int *c1)
{
    if (c0) *c0 = 128;	/* (shrug) */
    if (c1) *c1 = 128;
}


void cd_set_volume(int c0, int c1)
{
}


void cd_eject(void)
{
    command("set cdaudio door open");
    paused = 0;
}


void cd_close(void)
{
    command("set cdaudio door closed");
    paused = 0;
}
