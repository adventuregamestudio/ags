/*
 * Mpeg Audio Player (see version.h for version number)
 * ------------------------
 * copyright (c) 1995,1996,1997,1998,1999,2000 by Michael Hipp, All rights reserved.
 * See also 'README' !
 *
 */
#ifndef DISABLE_MPEG_AUDIO
#define DISABLE_MPEG_AUDIO // we have ALMP3 for that separately
#endif

#include <string.h>
#include <stdio.h>

#include "openal_support.h"

#include "apeg.h"
#include "mpg123.h"
#include "mpeg1dec.h"
//#include "getbits.h"

int _apeg_ignore_audio = FALSE;
int apeg_ignore_audio(int ignore)
{
	int last = _apeg_ignore_audio;
	_apeg_ignore_audio = ignore;
	return last;
}

float equalizer[2][32];

static int bufsize = 44100 * 2 * 2 * 200 / 1000;  // 200ms seconds of buffer.

static int down_sample = 0;
static int down_channel = FALSE;

static int (*_audio_callback_init)(APEG_STREAM*, int*, int*, void*) = NULL;
static int (*_audio_callback)(APEG_STREAM*, void*, int, void*) = NULL;
static void *_audio_callback_arg = NULL;
void apeg_set_audio_callbacks(int (*callback_init)(APEG_STREAM*,int*,int*,void*),
                              int (*callback)(APEG_STREAM*,void*,int,void*),
                              void *arg)
{
	_audio_callback_init = callback_init;
	_audio_callback = callback;
	_audio_callback_arg = arg;
}

#define check_al_error(X) { ALenum err = alGetError(); if (err) { printf("apeg: %s: " X " error: %d\n", __func__, err); return APEG_ERROR; } }

int _apeg_audio_reset_parameters(APEG_LAYER *layer)
{
	int buffer_padding;

	_apeg_audio_close(layer);

	if(layer->stream.audio.freq <= 0)
	{
		snprintf(apeg_error, sizeof(apeg_error), "Illegal audio frequency (%dhz)",
		         layer->stream.audio.freq);
		return APEG_ERROR;
	}
	if(layer->stream.audio.channels <= 0)
	{
		snprintf(apeg_error, sizeof(apeg_error), "Illegal channel count (%d)",
		         layer->stream.audio.channels);
		return APEG_ERROR;
	}

	buffer_padding = 0;



	free(layer->audio.pcm.samples);
	layer->audio.pcm.samples = malloc(layer->audio.bufsize+buffer_padding);
	if(!layer->audio.pcm.samples)
	{
		snprintf(apeg_error, sizeof(apeg_error), "Error allocating %d bytes for audio buffer",
		         layer->audio.bufsize+buffer_padding);
		return APEG_ERROR;
	}

	if(layer->stream.audio.channels > 2)
		layer->stream.audio.channels = 2;

	layer->audio.samples_per_update = layer->audio.bufsize / 2 /
	                                  layer->stream.audio.channels;

	layer->audio.processedSamples = 0;

	/* clear */ alGetError();

	/* Start the audio stream */
	alGenSources((ALuint)1, &layer->audio.alSource);
	check_al_error("alGenSources")

	alSourceRewind(layer->audio.alSource);
	check_al_error("alSourceRewind")

    alSourcei(layer->audio.alSource, AL_BUFFER, 0);
	check_al_error("alSourcei AL_BUFFER")

	layer->audio.last_pos = -1;
	layer->audio.pos = -layer->audio.samples_per_update*2;

	return APEG_OK;
}


/*
 * The poll function. Checks if more data is needed, and decodes/plays if
 * needed.
 */
int _apeg_audio_poll(APEG_LAYER *layer)
{


	if(layer->stream.flags & APEG_VORBIS_AUDIO)
		return alvorbis_update(layer);

	return APEG_ERROR;
}

/*
 * Main function
 */
int _apeg_start_audio(APEG_LAYER *layer, int enable)
{
	if((layer->stream.flags&APEG_HAS_AUDIO))
	{
		if(enable && layer->system_stream_flag == OGG_SYSTEM)
			return APEG_OK;

		if((layer->stream.flags&APEG_VORBIS_AUDIO))
			alvorbis_close(layer);

		_apeg_audio_close(layer);
		layer->stream.flags &= ~APEG_HAS_AUDIO;

		layer->audio.inited = FALSE;
	}

	layer->stream.audio.freq = -1;
	layer->stream.audio.channels = -1;
	layer->stream.audio.down_sample = down_sample;
	layer->stream.audio.down_channel = down_channel;

	layer->audio.voice = -1;
	layer->audio.buf_segment = 0;
	layer->audio.bufsize = bufsize;
	layer->audio.callback_init = 0;
	layer->audio.callback = 0;
	layer->audio.callback_arg = 0;

	if(!enable || _apeg_ignore_audio)
		return APEG_OK;
	return APEG_ERROR;
}



