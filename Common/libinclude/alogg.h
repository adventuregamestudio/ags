/* Allegro OGG */
/* to play OGG files with Allegro */

/* OGG decoder part ofOgg Vorbis (Xiph.org Foundation) */
/* Allegro OGG is copyright (c) 2002 Javier Gonz lez */

/* See COPYING.txt for license */


#ifndef ALOGG_H
#define ALOGG_H


#include <stdio.h>
#include <allegro.h>

#include "aloggdll.h"


#ifdef __cplusplus
extern "C" {
#endif


/* common define */

#define ALOGG_MAJOR_VERSION          1
#define ALOGG_MINOR_VERSION          0
#define ALOGG_SUB_VERSION            0
#define ALOGG_VERSION_STR            "1.0.0"
#define ALOGG_DATE_STR               "23/08/2002"
#define ALOGG_DATE                   20020823    /* yyyymmdd */

/* error codes */
                                  
#define ALOGG_OK                     0

#define ALOGG_PLAY_BUFFERTOOSMALL    -1

#define ALOGG_POLL_PLAYJUSTFINISHED  1
#define ALOGG_POLL_NOTPLAYING        -1
#define ALOGG_POLL_FRAMECORRUPT      -2
#define ALOGG_POLL_BUFFERUNDERRUN    -3
#define ALOGG_POLL_INTERNALERROR     -4


/* API - OGG */

typedef struct ALOGG_OGG ALOGG_OGG;


ALOGG_DLL_DECLSPEC ALOGG_OGG *alogg_create_ogg_from_buffer(void *data, int data_len);
ALOGG_DLL_DECLSPEC ALOGG_OGG *alogg_create_ogg_from_file(FILE *f);
ALOGG_DLL_DECLSPEC void alogg_destroy_ogg(ALOGG_OGG *ogg);

ALOGG_DLL_DECLSPEC int alogg_play_ogg(ALOGG_OGG *ogg, int buffer_len, int vol, int pan);
ALOGG_DLL_DECLSPEC int alogg_play_ex_ogg(ALOGG_OGG *ogg, int buffer_len, int vol, int pan, int speed, int loop);
ALOGG_DLL_DECLSPEC void alogg_stop_ogg(ALOGG_OGG *ogg);
ALOGG_DLL_DECLSPEC void alogg_adjust_ogg(ALOGG_OGG *ogg, int vol, int pan, int speed, int loop);

ALOGG_DLL_DECLSPEC void alogg_rewind_ogg(ALOGG_OGG *ogg);
ALOGG_DLL_DECLSPEC void alogg_seek_abs_msecs_ogg(ALOGG_OGG *ogg, int msecs);
ALOGG_DLL_DECLSPEC void alogg_seek_abs_secs_ogg(ALOGG_OGG *ogg, int secs);
ALOGG_DLL_DECLSPEC void alogg_seek_abs_bytes_ogg(ALOGG_OGG *ogg, int bytes);
ALOGG_DLL_DECLSPEC void alogg_seek_rel_msecs_ogg(ALOGG_OGG *ogg, int msec);
ALOGG_DLL_DECLSPEC void alogg_seek_rel_secs_ogg(ALOGG_OGG *ogg, int sec);
ALOGG_DLL_DECLSPEC void alogg_seek_rel_bytes_ogg(ALOGG_OGG *ogg, int bytes);

ALOGG_DLL_DECLSPEC int alogg_poll_ogg(ALOGG_OGG *ogg);
ALOGG_DLL_DECLSPEC void alogg_start_autopoll_ogg(ALOGG_OGG *ogg, int speed);
ALOGG_DLL_DECLSPEC void alogg_stop_autopoll_ogg(ALOGG_OGG *ogg);

ALOGG_DLL_DECLSPEC int alogg_get_pos_msecs_ogg(ALOGG_OGG *ogg);
ALOGG_DLL_DECLSPEC int alogg_get_pos_secs_ogg(ALOGG_OGG *ogg);
ALOGG_DLL_DECLSPEC int alogg_get_pos_bytes_ogg(ALOGG_OGG *ogg);

ALOGG_DLL_DECLSPEC int alogg_get_length_secs_ogg(ALOGG_OGG *ogg);
ALOGG_DLL_DECLSPEC int alogg_get_length_msecs_ogg(ALOGG_OGG *ogg);
ALOGG_DLL_DECLSPEC int alogg_get_length_bytes_ogg(ALOGG_OGG *ogg);

ALOGG_DLL_DECLSPEC int alogg_get_bitrate_ogg(ALOGG_OGG *ogg);
ALOGG_DLL_DECLSPEC int alogg_get_is_stereo_ogg(ALOGG_OGG *ogg);
ALOGG_DLL_DECLSPEC int alogg_get_wave_bits_ogg(ALOGG_OGG *ogg);
ALOGG_DLL_DECLSPEC int alogg_get_wave_is_stereo_ogg(ALOGG_OGG *ogg);
ALOGG_DLL_DECLSPEC int alogg_get_wave_freq_ogg(ALOGG_OGG *ogg);

ALOGG_DLL_DECLSPEC SAMPLE *alogg_create_sample_from_ogg(ALOGG_OGG *ogg);

ALOGG_DLL_DECLSPEC void *aloggget_output_wave_ogg(ALOGG_OGG *ogg, int *buffer_size);

ALOGG_DLL_DECLSPEC int alogg_is_playing_ogg(ALOGG_OGG *ogg);
ALOGG_DLL_DECLSPEC int alogg_is_looping_ogg(ALOGG_OGG *ogg);

ALOGG_DLL_DECLSPEC AUDIOSTREAM *alogg_get_audiostream_ogg(ALOGG_OGG *ogg);



/* API - OGGSTREAM */

typedef struct ALOGG_OGGSTREAM ALOGG_OGGSTREAM;


ALOGG_DLL_DECLSPEC ALOGG_OGGSTREAM *alogg_create_oggstream(void *first_data_buffer, int data_buffer_len, int last_block);
ALOGG_DLL_DECLSPEC ALOGG_OGGSTREAM *alogg_create_oggstream_ex(void *first_data_buffer, int data_buffer_len, int last_block, int downsample, int downmix);
ALOGG_DLL_DECLSPEC void alogg_destroy_oggstream(ALOGG_OGGSTREAM *ogg);

ALOGG_DLL_DECLSPEC int alogg_play_oggstream(ALOGG_OGGSTREAM *ogg, int buffer_len, int vol, int pan);
ALOGG_DLL_DECLSPEC int alogg_play_ex_oggstream(ALOGG_OGGSTREAM *ogg, int buffer_len, int vol, int pan, int speed);
ALOGG_DLL_DECLSPEC void alogg_stop_oggstream(ALOGG_OGGSTREAM *ogg);
ALOGG_DLL_DECLSPEC void alogg_adjust_oggstream(ALOGG_OGGSTREAM *ogg, int vol, int pan, int speed);

ALOGG_DLL_DECLSPEC int alogg_poll_oggstream(ALOGG_OGGSTREAM *ogg);
ALOGG_DLL_DECLSPEC void alogg_start_autopoll_oggstream(ALOGG_OGGSTREAM *ogg, int speed);
ALOGG_DLL_DECLSPEC void alogg_stop_autopoll_oggstream(ALOGG_OGGSTREAM *ogg);

ALOGG_DLL_DECLSPEC void *alogg_get_oggstream_buffer(ALOGG_OGGSTREAM *ogg);
ALOGG_DLL_DECLSPEC void alogg_free_oggstream_buffer(ALOGG_OGGSTREAM *ogg, int bytes_used);

ALOGG_DLL_DECLSPEC int alogg_get_pos_msecs_oggstream(ALOGG_OGGSTREAM *ogg);
ALOGG_DLL_DECLSPEC int alogg_get_pos_secs_oggstream(ALOGG_OGGSTREAM *ogg);
ALOGG_DLL_DECLSPEC int alogg_get_pos_bytes_oggstream(ALOGG_OGGSTREAM *ogg);

ALOGG_DLL_DECLSPEC int alogg_get_bitrate_oggstream(ALOGG_OGGSTREAM *ogg);
ALOGG_DLL_DECLSPEC int alogg_get_is_stereo_oggstream(ALOGG_OGGSTREAM *ogg);
ALOGG_DLL_DECLSPEC int alogg_get_wave_bits_oggstream(ALOGG_OGGSTREAM *ogg);
ALOGG_DLL_DECLSPEC int alogg_get_wave_is_stereo_oggstream(ALOGG_OGGSTREAM *ogg);
ALOGG_DLL_DECLSPEC int alogg_get_wave_freq_oggstream(ALOGG_OGGSTREAM *ogg);

ALOGG_DLL_DECLSPEC void *alogg_get_output_wave_oggstream(ALOGG_OGGSTREAM *ogg, int *buffer_size);

ALOGG_DLL_DECLSPEC int alogg_is_playing_oggstream(ALOGG_OGGSTREAM *ogg);

ALOGG_DLL_DECLSPEC AUDIOSTREAM *alogg_get_audiostream_oggstream(ALOGG_OGGSTREAM *ogg);



#ifdef __cplusplus
}
#endif

#endif
