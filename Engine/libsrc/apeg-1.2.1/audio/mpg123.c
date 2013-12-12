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

#ifndef __GNUC__
#define snprintf _snprintf
#endif
#include <string.h>
#include <stdio.h>

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

static int bufsize = 8192;

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

int apeg_downsample_audio(int ds)
{
	if(ds >= 0)
	{
		if(ds > 2)
			ds = 2;
		down_sample = ds;
	}

	return down_sample;
}

void apeg_downchannel_audio(int dc)
{
	down_channel = !!dc;
}


int apeg_set_audio_bufsize(int size)
{
	if(size > 0)
		bufsize = (size+255)&(~255);

	return bufsize;
}


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
#ifndef DISABLE_MPEG_AUDIO
	if(layer->stream.audio.layer == 3)
		buffer_padding = (128>>layer->stream.audio.down_sample) * SSLIMIT *
		                 layer->stream.audio.channels;
	if(layer->stream.audio.layer == 2)
		buffer_padding = (128>>layer->stream.audio.down_sample) * 3 *
		                 layer->stream.audio.channels;
	if(layer->stream.audio.layer == 1)
		buffer_padding = (128>>layer->stream.audio.down_sample) * SCALE_BLOCK *
		                 layer->stream.audio.channels;
#endif

	if(layer->audio.callback_init)
	{
		int channels = layer->stream.audio.channels;
		int freq = layer->stream.audio.freq;
		int ret;

		ret = layer->audio.callback_init((APEG_STREAM*)layer, &channels, &freq,
		                                 layer->audio.callback_arg);
		if(ret < 0)
		{
			snprintf(apeg_error, sizeof(apeg_error), "Audio callback init failed (code: %d)", ret);
			return APEG_ERROR;
		}

		if(channels > layer->stream.audio.channels || channels <= 0 ||
		   freq <= 0)
		{
			snprintf(apeg_error, sizeof(apeg_error), "Illegal audio mode requested (%dhz %d channels)",
			         freq, channels);
			return APEG_ERROR;
		}

		if(ret > 0)
			layer->audio.bufsize = (ret+255)&(~255);

		free(layer->audio.pcm.samples);
		layer->audio.pcm.samples = malloc(layer->audio.bufsize+buffer_padding);
		if(!layer->audio.pcm.samples)
		{
			snprintf(apeg_error, sizeof(apeg_error), "Error allocating %d bytes for audio buffer",
			         layer->audio.bufsize+buffer_padding);
			return APEG_ERROR;
		}

		if(channels == 1 && layer->stream.audio.channels > 1)
			layer->stream.audio.down_channel = TRUE;

		layer->stream.audio.channels = channels;
		layer->stream.audio.freq = freq;

		layer->audio.samples_per_update = layer->audio.bufsize / 2 /
		                                  layer->stream.audio.channels;

		layer->audio.last_pos = -1;
		layer->audio.pos = -layer->audio.samples_per_update*2;

		return APEG_OK;
	}

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

	/* Start the audio stream */
	layer->audio.stream = create_sample(16, layer->stream.audio.channels != 1,
	                                    layer->stream.audio.freq,
	                                    layer->audio.samples_per_update*2);
	if(layer->audio.stream)
	{
		int i;
		for(i = 0;i < layer->audio.bufsize;++i)
			((short*)layer->audio.stream->data)[i] = 0x8000;
	}
	else
	{
		snprintf(apeg_error, sizeof(apeg_error), "Error starting stream playback");
		return APEG_ERROR;
	}

	layer->audio.voice = play_sample(layer->audio.stream, 255, 127, 1000, TRUE);
	voice_set_priority(layer->audio.voice, 255);

	layer->audio.last_pos = -1;
	layer->audio.pos = -layer->audio.samples_per_update*2;

	_apeg_audio_set_speed_multiple(layer, layer->multiple);

	return APEG_OK;
}

#ifndef DISABLE_MPEG_AUDIO
/*
 * play a frame read by read_frame();
 * (re)initialize audio if necessary.
 */
static void decode_frame(APEG_LAYER *layer, struct frame *fr, struct mpstr *mp)
{
	int clip;

	/* do the decoding */
	switch(fr->lay)
	{
		case 1:
			clip = do_layer1(mp, fr, layer);
			break;
		case 2:
			clip = do_layer2(mp, fr, layer);
			break;
		case 3:
			clip = do_layer3(mp, fr, layer);
			break;
		default:
			// Shouldn't get here...
			clip = 1;
			mp->return_later = FALSE;
	}

	if(clip < 0)
		apeg_error_jump("Error decoding audio frame");
}
#endif


/*
 * The poll function. Checks if more data is needed, and decodes/plays if
 * needed.
 */
int _apeg_audio_poll(APEG_LAYER *layer)
{
#ifndef DISABLE_MPEG_AUDIO
	struct reader *rd = &(layer->audio.rd);
	struct frame *fr = &(layer->audio.fr);
	struct mpstr *mp = &(layer->audio.mp);
	int ret = APEG_OK;
#endif

	if(layer->multiple <= 0.0f)
		return APEG_OK;

	if(layer->stream.flags & APEG_VORBIS_AUDIO)
		return alvorbis_update(layer);

#ifdef DISABLE_MPEG_AUDIO
	return APEG_ERROR;
#else
	// Check if we actually need more data yet
	while(!rd->eof && layer->audio.pcm.point < layer->audio.bufsize)
	{
		// Make sure we're not still in the process of decoding the previous
		// frame
		if(mp->return_later)
		{
			decode_frame(layer, fr, mp);
			continue;
		}

		ret = read_frame(layer, fr);
		if(ret != ALMPA_OK)
		{
			// Stream ended, but check if there's still audio in the buffer
			if(!layer->audio.pcm.point)
				return ret;

			ret = APEG_OK;
			break;
		}

		/* TODO: for fast forwarding */
/*		if(stream->frame > layer->audio.frame)
		{
			if(fr->lay == 3)
				set_pointer(mp, fr->sideInfoSize, 512);
			goto do_again;
		}*/

		// Check for header change. This works, but it will introduce a jump
		// if the frequency/channel count changes.
		if(!layer->audio.inited)
		{
			int old_rate = layer->stream.audio.freq;
			int old_channels = layer->stream.audio.channels;

			int newrate = freqs[fr->sampling_frequency] >>
			              layer->stream.audio.down_sample;

			layer->stream.audio.channels = (layer->stream.audio.down_channel?1:
			                                fr->stereo);
			layer->stream.audio.freq = newrate;

			layer->stream.audio.layer = fr->lay;
			layer->stream.audio.kbps = tabsel_123[fr->lsf][fr->lay-1][fr->bitrate_index];
			fr->down_sample_sblimit = SBLIMIT>>layer->stream.audio.down_sample;

			if(layer->stream.audio.freq != old_rate ||
			   layer->stream.audio.channels != old_channels)
			{
				if(fr->lay == 3)
					init_layer3(layer->stream.audio.down_sample);

				if(_apeg_audio_reset_parameters(layer) != APEG_OK)
					return APEG_ERROR;
			}

			layer->audio.inited = TRUE;
		}

		++(layer->audio.frame);
		decode_frame(layer, fr, mp);
	}

	return _apeg_audio_flush(layer);
#endif
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

#ifndef DISABLE_MPEG_AUDIO
		memset(&(layer->audio.rd), 0, sizeof(layer->audio.rd));
		memset(&(layer->audio.fr), 0, sizeof(layer->audio.fr));
		memset(&(layer->audio.mp), 0, sizeof(layer->audio.mp));
		layer->audio.frame = 0;
#endif

		layer->audio.inited = FALSE;
	}

	layer->stream.audio.freq = -1;
	layer->stream.audio.channels = -1;
	layer->stream.audio.down_sample = down_sample;
	layer->stream.audio.down_channel = down_channel;

	layer->audio.voice = -1;
	layer->audio.buf_segment = 0;
	layer->audio.bufsize = bufsize;
	layer->audio.callback_init = _audio_callback_init;
	layer->audio.callback = _audio_callback;
	layer->audio.callback_arg = _audio_callback_arg;

	if(!enable || _apeg_ignore_audio)
		return APEG_OK;
#ifdef DISABLE_MPEG_AUDIO
	return APEG_ERROR;
#else
	layer->audio.mp.bo = 1;

	if(layer->system_stream_flag == OGG_SYSTEM)
		return APEG_ERROR;

	if(layer->system_stream_flag == NO_SYSTEM &&
	   (layer->stream.flags & APEG_MPG_VIDEO))
		return APEG_ERROR;

	if(sizeof(short) != 2) {
		sprintf(apeg_error, "Ouch SHORT has size of %ud bytes (required: '2')", sizeof(short));
		return APEG_ERROR;
	}
	if(sizeof(long) < 4) {
		sprintf(apeg_error, "Ouch LONG has size of %ud bytes (required: at least 4)", sizeof(long));
		return APEG_ERROR;
	}
	if(sizeof(int) != 4) {
		sprintf(apeg_error, "Ouch INT has size of %ud bytes (required: '4')", sizeof(int));
		return APEG_ERROR;
	}

	/* Open the stream */
	if(open_stream(layer) != ALMPA_OK)
	{
		memset(&(layer->audio.rd), 0, sizeof(layer->audio.rd));
		sprintf(apeg_error, "Error opening audio stream");
		return APEG_ERROR;
	}

	layer->audio.mp.bsbuf = layer->audio.mp.bsspace[1];

	make_decode_tables();
	init_layer2(); /* inits also shared tables with layer1 */
	init_layer3(down_sample);

	if(read_frame(layer, &(layer->audio.fr)) != APEG_OK)
	{
		_apeg_start_audio(layer, FALSE);
		sprintf(apeg_error, "Error reading first audio frame");
		return APEG_ERROR;
	}

	if(layer->audio.fr.stereo == 1)
		layer->stream.audio.down_channel = FALSE;
	layer->stream.audio.channels = (down_channel ? 1 : layer->audio.fr.stereo);
	layer->stream.audio.freq = freqs[layer->audio.fr.sampling_frequency] >>
	                           down_sample;

	layer->stream.audio.layer = layer->audio.fr.lay;
	layer->stream.audio.kbps = tabsel_123[layer->audio.fr.lsf][layer->audio.fr.lay-1][layer->audio.fr.bitrate_index];
	layer->audio.fr.down_sample_sblimit = SBLIMIT >> down_sample;

	if(_apeg_audio_reset_parameters(layer) != APEG_OK)
	{
		_apeg_start_audio(layer, FALSE);
		return APEG_ERROR;
	}

	if(layer->audio.fr.lay == 3)
		init_layer3(down_sample);

	layer->audio.inited = TRUE;

	++(layer->audio.frame);
	decode_frame(layer, &layer->audio.fr, &layer->audio.mp);

	layer->stream.flags |= APEG_MPG_AUDIO;

	return APEG_OK;
#endif
}

int apeg_get_stream_voice(APEG_STREAM *stream)
{
	ASSERT(stream);
	ASSERT(stream->flags & APEG_HAS_AUDIO);
	return ((APEG_LAYER*)stream)->audio.voice;
}


struct smpl_bldr {
	SAMPLE *smp;
	unsigned int pos;
};

static int sample_capture_init(APEG_STREAM *stream, int *channels, int *freq, void *arg)
{
	struct smpl_bldr *sb = (struct smpl_bldr*)arg;

	if(sb->smp)
		return -1;

	if(*channels > 2)
		*channels = 2;

	sb->smp = create_sample(16, (*channels == 2), *freq, (*channels)*(*freq));
	if(!sb->smp)
		return -1;

	sb->pos = 0;
	return 0;
	(void)stream;
}

static int sample_capture(APEG_STREAM *stream, void *buf, int len, void *arg)
{
	struct smpl_bldr *sb = (struct smpl_bldr*)arg;
	if(sb->pos+len > sb->smp->len*2)
	{
		unsigned int nlen = sb->smp->len*2 * 2;
		void *tmp = realloc(sb->smp->data, nlen);
		if(!tmp)
			return -1;
		sb->smp->data = tmp;
		sb->smp->len = nlen/2;
	}

	memcpy((char*)sb->smp->data+sb->pos, buf, len);
	sb->pos += len;
	return len;
	(void)stream;
}

SAMPLE *apeg_preload_audio(const char *filename)
{
	void *holder[3] = { _audio_callback_init, _audio_callback,
	                    _audio_callback_arg };
	int ds = down_sample;
	int dc = down_channel;
	APEG_STREAM *stream = NULL;
	struct smpl_bldr sb = { NULL, 0 };
	int old_ignore_audio = apeg_ignore_audio(FALSE);
	int old_ignore_video = apeg_ignore_video(TRUE);

	down_sample = 0;
	down_channel = 0;
	apeg_set_audio_callbacks(sample_capture_init, sample_capture, &sb);

	stream = apeg_open_stream(filename);

	if(!stream || !(stream->flags & APEG_HAS_AUDIO))
	{
		snprintf(apeg_error, sizeof(apeg_error), "Could not open file");
		goto done;
	}

	while(apeg_advance_stream(stream, FALSE) == APEG_OK)
		;

	apeg_close_stream(stream);

	sb.smp->len = sb.pos/2;
	sb.smp->loop_end = sb.smp->len;
	sb.smp->data = realloc(sb.smp->data, sb.smp->len*2);

done:
	apeg_close_stream(stream);

	apeg_set_audio_callbacks(holder[0], holder[1], holder[2]);
	down_channel = dc;
	down_sample = ds;

	apeg_ignore_audio(old_ignore_audio);
	apeg_ignore_video(old_ignore_video);

#ifndef DISABLE_MPEG_AUDIO
	// Eww.. rebuild the tables if we used a different downsampling mode.
	if(down_sample)
		init_layer3(down_sample);
#endif

	return sb.smp;
}
