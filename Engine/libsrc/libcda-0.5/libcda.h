/* This file is part of libcda.  See COPYING for licence.
 *
 * Peter Wang <tjaden@users.sf.net>
 */

#ifndef __included_libcda_h
#define __included_libcda_h

#ifdef __cplusplus
extern "C" {
#endif


/* High-byte is major version, low-byte is minor. */
#define LIBCDA_VERSION		0x0005
#define LIBCDA_VERSION_STR	"0.5"


extern const char *cd_error;


int cd_init(void);
void cd_exit(void);

int cd_play(int track);
int cd_play_range(int start, int end);
int cd_play_from(int track);
int cd_current_track(void);
void cd_pause(void);
void cd_resume(void);
int cd_is_paused(void);
void cd_stop(void);

int cd_get_tracks(int *first, int *last);
int cd_is_audio(int track);

void cd_get_volume(int *c0, int *c1);
void cd_set_volume(int c0, int c1);

void cd_eject(void);
void cd_close(void);


#ifdef __cplusplus
}
#endif

#endif
