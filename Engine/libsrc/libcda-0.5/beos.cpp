/* libcda; BeOS component.
 *
 * Partly based on CDButton sample, by Pavel Cisler.
 * 
 * Peter Wang <tjaden@users.sf.net> 
 */

#include <Entry.h>
#include <Directory.h>
#include <Path.h>
#include <errno.h>
#include <scsi.h>
#include <string.h>
#include <unistd.h>
#include "libcda.h"


#define MID(x,y,z)   MAX((x), MIN((y), (z)))


static int fd = -1;

static char _cd_error[256];
const char *cd_error = _cd_error;


static void copy_cd_error(void)
{
    strncpy(_cd_error, strerror(errno), sizeof _cd_error);
    _cd_error[sizeof _cd_error - 1] = 0;
}


static int search_cd_drive(const char *root)
{
    BDirectory dir;
    BEntry entry;
    BPath path;
    int fd;
    device_geometry geo;

    dir.SetTo(root);
    if (dir.InitCheck() != B_NO_ERROR)
	return -1;

    dir.Rewind();

    while (dir.GetNextEntry(&entry) >= 0) {

	if (entry.GetPath(&path) != B_NO_ERROR)
	    continue;

	if (entry.IsDirectory()) {
	    if (strcmp(path.Leaf(), "floppy") == 0) 
		continue;
	    
	    fd = search_cd_drive(path.Path());
	    if (fd >= 0)
		return fd;
	}
	else if (strcmp(path.Leaf(), "raw") == 0) {
	    
	    fd = open(path.Path(), O_RDONLY);
	    if (fd < 0)
		continue;
		    
	    if (ioctl(fd, B_GET_GEOMETRY, &geo, sizeof geo) >= 0) 
		if (geo.device_type == B_CD)
		    return fd;

	    close(fd);
	}
    }
    
    return -1;
}


int cd_init(void)
{
    fd = search_cd_drive("/dev/disk");
    if (fd < 0) {
	copy_cd_error();
	return -1;
    }
    else {
	return 0;
    }
}


void cd_exit(void)
{
    if (fd >= 0) {
	close(fd);
	fd = -1;
    }
}


/* prevents us from getting a stupid `media changed' error */
static void get_media_status(void)
{
    status_t status;
    ioctl(fd, B_GET_MEDIA_STATUS, &status, sizeof status);
}


static int play(int start, int end) 
{
    scsi_play_track p;
    int first, last;

    get_media_status();

    cd_get_tracks(&first, &last);
    if ((start > end) || (start < first) || (end > last)) {
	strcpy(_cd_error, "Invalid track range");
	return -1;
    }

    p.start_track = start;
    p.start_index = 1;
    p.end_track = end;
    p.end_index = 1;
    
    if (ioctl(fd, B_SCSI_PLAY_TRACK, &p) != B_NO_ERROR) {
	copy_cd_error();
	return -1;
    }

    return 0;
}


int cd_play(int track)
{
    return play(track, track);
}


int cd_play_range(int start, int end)
{
    return play(start, end);
}


int cd_play_from(int track)
{
    return play(track, 100);
}


int cd_current_track(void)
{
    scsi_position pos;
    
    if ((ioctl(fd, B_SCSI_GET_POSITION, &pos) != B_NO_ERROR)
	|| (!pos.position[1]) || (pos.position[1] >= 0x13)
	|| ((pos.position[1] == 0x12) && (!pos.position[6])))
	return 0;
		
    return pos.position[6];
}


void cd_pause(void)
{
    ioctl(fd, B_SCSI_PAUSE_AUDIO);
}


void cd_resume(void)
{
    ioctl(fd, B_SCSI_RESUME_AUDIO);
}


int cd_is_paused(void)
{
    scsi_position pos;
    
    if (ioctl(fd, B_SCSI_GET_POSITION, &pos) != B_NO_ERROR)
	return 0;

    if ((!pos.position[1]) || (pos.position[1] >= 0x13) 
	|| ((pos.position[1] == 0x12) && (!pos.position[6]))
	|| (pos.position[1] == 0x11))
	return 0;

    return 1;
}


void cd_stop(void)
{
    ioctl(fd, B_SCSI_STOP_AUDIO);
}


int cd_get_tracks(int *first, int *last)
{
    scsi_toc toc;

    get_media_status();
    
    if (ioctl(fd, B_SCSI_GET_TOC, &toc) != B_NO_ERROR) {
	copy_cd_error();
	return -1;
    }

    if (first) *first = 1; /* I don't have the SCSI standard on me. */
    if (last)  *last = toc.toc_data[3];
    
    return 0;
}


int cd_is_audio(int track)
{
    scsi_toc toc;
    int first, last;

    get_media_status();
    
    if (ioctl(fd, B_SCSI_GET_TOC, &toc) != B_NO_ERROR) {
	copy_cd_error();
	return -1;
    }

    if (cd_get_tracks(&first, &last) < 0)
	return -1;

    if ((track < first) || (track > last)) {
	strcpy(_cd_error, "Track out of range");
	return -1;
    }
    
    /*
     * I derived from a snippet from a mailing list archive somewhere.
     * Don't ask me how it works :-)
     */
    return !(toc.toc_data[((track - toc.toc_data[2]) * 8) + 4 + 1] & 4);
}


void cd_get_volume(int *c0, int *c1)
{
    scsi_volume vol;

    get_media_status();

    ioctl(fd, B_SCSI_GET_VOLUME, &vol, sizeof vol);
    if (c0) *c0 = vol.port0_volume;
    if (c1) *c1 = vol.port1_volume;
}


void cd_set_volume(int c0, int c1)
{
    scsi_volume vol;

    get_media_status();

    vol.flags = B_SCSI_PORT0_VOLUME | B_SCSI_PORT1_VOLUME;
    vol.port0_volume = MID(0, c0, 255);
    vol.port1_volume = MID(0, c1, 255);
    
    ioctl(fd, B_SCSI_SET_VOLUME, &vol, sizeof vol);
}


static int door_status(void)
{
    status_t status;
    
    if (ioctl(fd, B_GET_MEDIA_STATUS, &status, sizeof status) == B_NO_ERROR)
	return -1;	
    
    return (status == B_DEV_DOOR_OPEN) ? 1 : 0;
}

#define door_open()	(door_status() == 1)
#define door_closed()	(door_status() == 0)

    
void cd_eject(void)
{
    if (!door_open()) 
	ioctl(fd, B_EJECT_DEVICE);
}


void cd_close(void)
{
    if (door_open())
	ioctl(fd, B_LOAD_MEDIA);
}
