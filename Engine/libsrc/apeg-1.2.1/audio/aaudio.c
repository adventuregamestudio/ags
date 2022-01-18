#include <string.h>
#include <stdio.h>

#include "apeg.h"
#include "mpg123.h"
#include "mpeg1dec.h"


#include "openal_support.h"

const int MAX_BUFFERS_QUEUED = 4;

int _apeg_audio_get_position(APEG_LAYER *layer)
{
	ALenum err;
	ALint np;

	if(layer->audio.alSource <= 0)
		return 0;

	alGetSourcei(layer->audio.alSource, AL_SAMPLE_OFFSET, &np);
	err = alGetError();
	if (err) {
		printf("_apeg_audio_get_position: alGetSourcei AL_SAMPLE_OFFSET error: %d\n", err);
		return 0;
	}
	// NOTE: returned offset is relative to the current buffer queue;
	// we must also add accumulated value of processed samples
	np += layer->audio.processedSamples;

	if(np < 0)
	{
		layer->audio.last_pos = np;
		return 0;
	}

	if(np <= layer->audio.last_pos)
		return -1;

	layer->audio.last_pos = np;
	return np;
}

#define check_al_error(X) { ALenum err = alGetError(); if (err) { printf("apeg: %s: " X " error: %d\n", __func__, err); return APEG_ERROR; } }

static int _apeg_audio_unqueue_buffers(APEG_LAYER *layer)
{
	while (1) {
		ALuint processed = 0;
		alGetSourcei(layer->audio.alSource, AL_BUFFERS_PROCESSED, &processed);
		check_al_error("alGetSourcei AL_BUFFERS_PROCESSED")
		if (processed == 0) { break; }

		ALuint bufid = 0;
		alSourceUnqueueBuffers(layer->audio.alSource, 1, &bufid);
		check_al_error("alSourceUnqueueBuffers")

		ALint processedBytes = 0;
		ALint bits = 0;
		ALint channels = 0;
		alGetBufferi(bufid, AL_SIZE, &processedBytes);
		alGetBufferi(bufid, AL_CHANNELS, &channels);
		// IMPORTANT! mojoAL is *LYING* about buffer bitness, because it reports the one
		// passed to alBufferData, but internally always works with Float32; therefore
		// final samples found in the buffer are in float32!
		//alGetBufferi(bufid, AL_BITS, &bits);
		bits = sizeof(ALfloat) * 8;
		check_al_error("alGetBufferi AL_SIZE")

		int bufferSamples = (processedBytes * 8) / (channels * bits);
		layer->audio.processedSamples += bufferSamples;

		alDeleteBuffers(1, &bufid);
		check_al_error("alDeleteBuffers")

		processed--;
	}

	return APEG_OK;
}


int _apeg_audio_flush(APEG_LAYER *layer)
{
	unsigned char *buf = layer->audio.pcm.samples;
	unsigned char *data;
	int hs;
	int ret = APEG_OK;

	/* clear */ alGetError();

	if(layer->audio.pcm.point < layer->audio.bufsize)
	{
		int count = layer->audio.pcm.point / 2;
		int samplesend = layer->audio.bufsize / 2;

		while(count < samplesend)
			((short*)buf)[count++] = 0;

		if(layer->audio.pcm.point == 0)
			ret = APEG_EOF;
	}

	if (_apeg_audio_unqueue_buffers(layer) == APEG_ERROR) { return APEG_ERROR; }

	ALint queued = 0;
	alGetSourcei(layer->audio.alSource, AL_BUFFERS_QUEUED, &queued);
	check_al_error("alGetSourcei AL_BUFFERS_QUEUED")

	if (queued >= MAX_BUFFERS_QUEUED) {
		return ret;
	}

	layer->audio.pos = -1;

	if (layer->audio.pcm.point > 0) {
		ALuint alBuffer;
		alGenBuffers((ALuint)1, &alBuffer);
		check_al_error("alGenBuffers")

		alBufferData(alBuffer, layer->stream.audio.channels == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16, buf, layer->audio.pcm.point, layer->stream.audio.freq);
		check_al_error("alBufferData")

		alSourceQueueBuffers(layer->audio.alSource, 1, &alBuffer);
		check_al_error("alSourceQueueBuffers")

		ALint state;
		alGetSourcei(layer->audio.alSource,AL_SOURCE_STATE,&state);
		check_al_error("alGetSourcei AL_SOURCE_STATE")

		if (state != AL_PLAYING) { 
			alSourcePlay(layer->audio.alSource);
			check_al_error("alSourcePlay")
		}
	}
	layer->audio.pcm.point = 0;
	layer->stream.audio.flushed = TRUE;

	return ret;
}

int _apeg_audio_close(APEG_LAYER *layer)
{
	if (layer->audio.alSource > 0) {
		alSourceStop(layer->audio.alSource);
		check_al_error("alSourceStop")

		alSourcei(layer->audio.alSource, AL_BUFFER, 0);
		check_al_error("alSourcei AL_BUFFER NULL")

		if (_apeg_audio_unqueue_buffers(layer) == APEG_ERROR) { return APEG_ERROR; }

		alDeleteSources(1, &layer->audio.alSource);
		check_al_error("alDeleteSources")
	}
	layer->audio.alSource = 0;
	layer->audio.voice = -1;

	return 0;
}
