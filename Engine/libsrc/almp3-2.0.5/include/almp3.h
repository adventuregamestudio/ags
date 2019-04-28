/* Allegro MP3 - a wrapper for mpglib from mpg123 */
/* to play MP3 files with Allegro */

/* MP3 decoder part of mpglib from mpg123 (www.mpg123.com) */
/* Allegro MP3 is copyright (c) 2001, 2002 Javier Gonz lez */

/* See COPYING.txt (GNU Lesser General Public License 2.1) for license */


#ifndef ALMP3_H
#define ALMP3_H


#include <allegro.h>

#include "almp3dll.h"


#ifdef __cplusplus
extern "C" {
#endif


/* common define */

#define ALMP3_MAJOR_VERSION          2
#define ALMP3_MINOR_VERSION          0
#define ALMP3_SUB_VERSION            4
#define ALMP3_VERSION_STR            "2.0.4"
#define ALMP3_DATE_STR               "08/04/2003"
#define ALMP3_DATE                   20030408    /* yyyymmdd */

/* error codes */
                                  
#define ALMP3_OK                     0

#define ALMP3_PLAY_BUFFERTOOSMALL    -1

#define ALMP3_POLL_PLAYJUSTFINISHED  1
#define ALMP3_POLL_NOTPLAYING        -1
#define ALMP3_POLL_FRAMECORRUPT      -2
#define ALMP3_POLL_BUFFERUNDERRUN    -3
#define ALMP3_POLL_INTERNALERROR     -4


/* API - MP3 */

typedef struct ALMP3_MP3 ALMP3_MP3;


ALMP3_DLL_DECLSPEC ALMP3_MP3 *almp3_create_mp3(void *data, int data_len);
ALMP3_DLL_DECLSPEC void almp3_destroy_mp3(ALMP3_MP3 *mp3);

ALMP3_DLL_DECLSPEC int almp3_play_mp3(ALMP3_MP3 *mp3, int buffer_len, int vol, int pan);
ALMP3_DLL_DECLSPEC int almp3_play_ex_mp3(ALMP3_MP3 *mp3, int buffer_len, int vol, int pan, int speed, int loop);
ALMP3_DLL_DECLSPEC void almp3_stop_mp3(ALMP3_MP3 *mp3);
ALMP3_DLL_DECLSPEC void almp3_rewind_mp3(ALMP3_MP3 *mp3);
ALMP3_DLL_DECLSPEC void almp3_seek_abs_frames_mp3(ALMP3_MP3 *mp3, int frame);
ALMP3_DLL_DECLSPEC void almp3_seek_abs_msecs_mp3(ALMP3_MP3 *mp3, int msecs);
ALMP3_DLL_DECLSPEC void almp3_seek_abs_secs_mp3(ALMP3_MP3 *mp3, int secs);
ALMP3_DLL_DECLSPEC void almp3_seek_abs_bytes_mp3(ALMP3_MP3 *mp3, int bytes);
ALMP3_DLL_DECLSPEC void almp3_seek_rel_frames_mp3(ALMP3_MP3 *mp3, int frame);
ALMP3_DLL_DECLSPEC void almp3_seek_rel_msecs_mp3(ALMP3_MP3 *mp3, int msec);
ALMP3_DLL_DECLSPEC void almp3_seek_rel_secs_mp3(ALMP3_MP3 *mp3, int sec);
ALMP3_DLL_DECLSPEC void almp3_seek_rel_bytes_mp3(ALMP3_MP3 *mp3, int bytes);
ALMP3_DLL_DECLSPEC void almp3_adjust_mp3(ALMP3_MP3 *mp3, int vol, int pan, int speed, int loop);

ALMP3_DLL_DECLSPEC int almp3_poll_mp3(ALMP3_MP3 *mp3);
ALMP3_DLL_DECLSPEC void almp3_start_autopoll_mp3(ALMP3_MP3 *mp3, int speed);
ALMP3_DLL_DECLSPEC void almp3_stop_autopoll_mp3(ALMP3_MP3 *mp3);

ALMP3_DLL_DECLSPEC int almp3_get_pos_frames_mp3(ALMP3_MP3 *mp3);
ALMP3_DLL_DECLSPEC int almp3_get_pos_msecs_mp3(ALMP3_MP3 *mp3);
ALMP3_DLL_DECLSPEC int almp3_get_pos_secs_mp3(ALMP3_MP3 *mp3);
ALMP3_DLL_DECLSPEC int almp3_get_pos_bytes_mp3(ALMP3_MP3 *mp3);
ALMP3_DLL_DECLSPEC int almp3_get_length_frames_mp3(ALMP3_MP3 *mp3);
ALMP3_DLL_DECLSPEC int almp3_get_length_secs_mp3(ALMP3_MP3 *mp3);
ALMP3_DLL_DECLSPEC int almp3_get_length_msecs_mp3(ALMP3_MP3 *mp3);
ALMP3_DLL_DECLSPEC int almp3_get_length_bytes_mp3(ALMP3_MP3 *mp3);
ALMP3_DLL_DECLSPEC int almp3_get_msecs_per_frame_mp3(ALMP3_MP3 *mp3);
ALMP3_DLL_DECLSPEC int almp3_get_bitrate_mp3(ALMP3_MP3 *mp3);
ALMP3_DLL_DECLSPEC int almp3_get_layer_mp3(ALMP3_MP3 *mp3);
ALMP3_DLL_DECLSPEC int almp3_get_is_stereo_mp3(ALMP3_MP3 *mp3);
ALMP3_DLL_DECLSPEC int almp3_get_wave_bits_mp3(ALMP3_MP3 *mp3);
ALMP3_DLL_DECLSPEC int almp3_get_wave_is_stereo_mp3(ALMP3_MP3 *mp3);
ALMP3_DLL_DECLSPEC int almp3_get_wave_freq_mp3(ALMP3_MP3 *mp3);

ALMP3_DLL_DECLSPEC SAMPLE *almp3_create_sample_from_mp3(ALMP3_MP3 *mp3);

ALMP3_DLL_DECLSPEC void *almp3get_output_wave_mp3(ALMP3_MP3 *mp3, int *buffer_size);

ALMP3_DLL_DECLSPEC int almp3_is_playing_mp3(ALMP3_MP3 *mp3);
ALMP3_DLL_DECLSPEC int almp3_is_looping_mp3(ALMP3_MP3 *mp3);

ALMP3_DLL_DECLSPEC AUDIOSTREAM *almp3_get_audiostream_mp3(ALMP3_MP3 *mp3);



/* API - MP3STREAM */

typedef struct ALMP3_MP3STREAM ALMP3_MP3STREAM;


ALMP3_DLL_DECLSPEC ALMP3_MP3STREAM *almp3_create_mp3stream(void *first_data_buffer, int data_buffer_len, int last_block);
ALMP3_DLL_DECLSPEC ALMP3_MP3STREAM *almp3_create_mp3stream_ex(void *first_data_buffer, int data_buffer_len, int last_block, int downsample, int downmix);
ALMP3_DLL_DECLSPEC void almp3_destroy_mp3stream(ALMP3_MP3STREAM *mp3);

ALMP3_DLL_DECLSPEC int almp3_play_mp3stream(ALMP3_MP3STREAM *mp3, int buffer_len, int vol, int pan);
ALMP3_DLL_DECLSPEC int almp3_play_ex_mp3stream(ALMP3_MP3STREAM *mp3, int buffer_len, int vol, int pan, int speed);
ALMP3_DLL_DECLSPEC void almp3_stop_mp3stream(ALMP3_MP3STREAM *mp3);
ALMP3_DLL_DECLSPEC void almp3_adjust_mp3stream(ALMP3_MP3STREAM *mp3, int vol, int pan, int speed);

ALMP3_DLL_DECLSPEC int almp3_poll_mp3stream(ALMP3_MP3STREAM *mp3);
ALMP3_DLL_DECLSPEC void almp3_start_autopoll_mp3stream(ALMP3_MP3STREAM *mp3, int speed);
ALMP3_DLL_DECLSPEC void almp3_stop_autopoll_mp3stream(ALMP3_MP3STREAM *mp3);
ALMP3_DLL_DECLSPEC void *almp3_get_mp3stream_buffer(ALMP3_MP3STREAM *mp3);
ALMP3_DLL_DECLSPEC void almp3_free_mp3stream_buffer(ALMP3_MP3STREAM *mp3, int bytes_used);

ALMP3_DLL_DECLSPEC int almp3_get_length_frames_mp3stream(ALMP3_MP3STREAM *mp3, int total_size);
ALMP3_DLL_DECLSPEC int almp3_get_length_secs_mp3stream(ALMP3_MP3STREAM *mp3, int total_size);
ALMP3_DLL_DECLSPEC int almp3_get_length_msecs_mp3stream(ALMP3_MP3STREAM *mp3, int total_size);
ALMP3_DLL_DECLSPEC int almp3_get_length_bytes_mp3stream(ALMP3_MP3STREAM *mp3, int total_size);
ALMP3_DLL_DECLSPEC int almp3_get_pos_frames_mp3stream(ALMP3_MP3STREAM *mp3);
ALMP3_DLL_DECLSPEC int almp3_get_pos_msecs_mp3stream(ALMP3_MP3STREAM *mp3);
ALMP3_DLL_DECLSPEC int almp3_get_pos_secs_mp3stream(ALMP3_MP3STREAM *mp3);
ALMP3_DLL_DECLSPEC int almp3_get_pos_bytes_mp3stream(ALMP3_MP3STREAM *mp3);
ALMP3_DLL_DECLSPEC int almp3_get_msecs_per_frame_mp3stream(ALMP3_MP3STREAM *mp3);
ALMP3_DLL_DECLSPEC int almp3_get_bitrate_mp3stream(ALMP3_MP3STREAM *mp3);
ALMP3_DLL_DECLSPEC int almp3_get_layer_mp3stream(ALMP3_MP3STREAM *mp3);
ALMP3_DLL_DECLSPEC int almp3_get_is_stereo_mp3stream(ALMP3_MP3STREAM *mp3);
ALMP3_DLL_DECLSPEC int almp3_get_wave_bits_mp3stream(ALMP3_MP3STREAM *mp3);
ALMP3_DLL_DECLSPEC int almp3_get_wave_is_stereo_mp3stream(ALMP3_MP3STREAM *mp3);
ALMP3_DLL_DECLSPEC int almp3_get_wave_freq_mp3stream(ALMP3_MP3STREAM *mp3);

ALMP3_DLL_DECLSPEC void *almp3_get_output_wave_mp3stream(ALMP3_MP3STREAM *mp3, int *buffer_size);

ALMP3_DLL_DECLSPEC int almp3_is_playing_mp3stream(ALMP3_MP3STREAM *mp3);

ALMP3_DLL_DECLSPEC AUDIOSTREAM *almp3_get_audiostream_mp3stream(ALMP3_MP3STREAM *mp3);

ALMP3_DLL_DECLSPEC int almp3_seek_abs_frames_mp3stream(ALMP3_MP3STREAM *mp3, int frame, int total_size);
ALMP3_DLL_DECLSPEC int almp3_seek_abs_msecs_mp3stream(ALMP3_MP3STREAM *mp3, int msecs, int total_size);
ALMP3_DLL_DECLSPEC int almp3_seek_abs_secs_mp3stream(ALMP3_MP3STREAM *mp3, int secs, int total_size);
ALMP3_DLL_DECLSPEC int almp3_seek_abs_bytes_mp3stream(ALMP3_MP3STREAM *mp3, int bytes, int total_size);
ALMP3_DLL_DECLSPEC int almp3_seek_rel_frames_mp3stream(ALMP3_MP3STREAM *mp3, int frame, int total_size);
ALMP3_DLL_DECLSPEC int almp3_seek_rel_msecs_mp3stream(ALMP3_MP3STREAM *mp3, int msec, int total_size);
ALMP3_DLL_DECLSPEC int almp3_seek_rel_secs_mp3stream(ALMP3_MP3STREAM *mp3, int sec, int total_size);
ALMP3_DLL_DECLSPEC int almp3_seek_rel_bytes_mp3stream(ALMP3_MP3STREAM *mp3, int bytes, int total_size);


#ifdef __cplusplus
}
#endif

#endif
