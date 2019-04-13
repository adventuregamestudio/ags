#ifndef APEG_H
#define APEG_H

#include <allegro.h>


#define APEG_MAKE_VERSION(a, b, c)	(((a)<<16) | ((b)<<8) | (c))

#define APEG_MAJOR_VERSION	1
#define APEG_MINOR_VERSION	2
#define APEG_REVISION		1

#define APEG_VERSION_NUM	APEG_MAKE_VERSION(APEG_MAJOR_VERSION, APEG_MINOR_VERSION, APEG_REVISION)

#define APEG_VERSION_STR	"1.2.1"


#define FPS_TO_TIMER(x)	((long)((float)TIMERS_PER_SECOND / (float)(x)))

#define APEG_OK		(0)
#define APEG_ERROR	(-1)
#define APEG_EOF	(-2)


#ifdef __cplusplus
extern "C" {
#endif

typedef struct APEG_AUDIO_INF {
	int kbps;
	int layer;
	int freq;
	int down_sample;
	int channels;
	int down_channel;
	int flushed;
} APEG_AUDIO_INF;

typedef struct APEG_STREAM {
	BITMAP *bitmap;
	int frame_updated;
	unsigned int frame;

	int w, h;

	double aspect_ratio;
	int aspect_numerator, aspect_denominator;

	double length;
	double pos;

	enum {
		APEG_420,
		APEG_422,
		APEG_444
	} pixel_format;

	double frame_rate;
	int fps_numerator, fps_denominator;

	int bit_rate;

	int sequence;

	volatile int timer;

	int flags;

	APEG_AUDIO_INF audio;
} APEG_STREAM;
#define APEG_MPG_VIDEO	1
#define APEG_MPG_AUDIO	2
#define APEG_VORBIS_AUDIO	4
#define APEG_THEORA_VIDEO	8
#define APEG_HAS_VIDEO	(APEG_MPG_VIDEO|APEG_THEORA_VIDEO)
#define APEG_HAS_AUDIO	(APEG_MPG_AUDIO|APEG_VORBIS_AUDIO)


APEG_STREAM *apeg_open_stream(const char *filename);
APEG_STREAM *apeg_open_memory_stream(void *buffer, int length);
APEG_STREAM *apeg_open_stream_ex(void *ptr);
int apeg_advance_stream(APEG_STREAM *stream, int loop);
int apeg_reset_stream(APEG_STREAM *stream);
void apeg_close_stream(APEG_STREAM *stream);

int apeg_play_mpg(const char *filename, BITMAP *target, int loop,
                  int (*callback)(void));
int apeg_play_memory_mpg(void *buffer, BITMAP *target, int loop,
                         int (*callback)(BITMAP*));
int apeg_play_mpg_ex(void *ptr, BITMAP *target, int loop,
                     int (*callback)(BITMAP*));
int apeg_play_apeg_stream(APEG_STREAM *stream_to_play, BITMAP *bmp, int loop, int (*callback)(BITMAP*tempBuffer));

int apeg_ignore_video(int ignore);
int apeg_ignore_audio(int ignore);

void apeg_set_memory_stream_size(int size);
void apeg_set_stream_reader(int (*init)(void *ptr),
                            int (*read)(void *buffer, int bytes, void *ptr),
                            void (*skip)(int bytes, void *ptr));


void apeg_set_display_callbacks(int (*init)(APEG_STREAM *stream, int coded_w,
                                            int coded_h, void *ptr),
                                void (*callback)(APEG_STREAM *stream,
                                                 unsigned char **src,
                                                 void *ptr),
                                void *ptr);
void apeg_set_display_depth(int depth);
void apeg_set_quality(int quality);

void apeg_enable_framedrop(int framedrop);
void apeg_disable_length_detection(int skipdetect);

void apeg_reset_colors(APEG_STREAM *stream);


void apeg_get_video_size(APEG_STREAM *stream, int *w, int *h);
void apeg_set_stream_rate(APEG_STREAM *stream, float rate);


SAMPLE *apeg_preload_audio(const char *filename);

int apeg_get_stream_voice(APEG_STREAM *stream);

void apeg_set_audio_callbacks(int (*init)(APEG_STREAM *stream, int *channels,
                                          int *frequency, void *ptr),
                              int (*callback)(APEG_STREAM *stream,
                                              void *buffer, int bytes,
                                              void *ptr),
                              void *ptr);

int  apeg_downsample_audio(int mode);
void apeg_downchannel_audio(int mode);

int apeg_set_audio_bufsize(int size);


extern APEG_STREAM *apeg_stream;
extern PALETTE apeg_palette;
extern char apeg_error[256];

#ifdef __cplusplus
}
#endif

#endif	// APEG_H
