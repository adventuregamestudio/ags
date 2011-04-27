#include <string.h>

#include "apeg.h"
#include "mpeg1dec.h"

#define int64_t __int64

#ifndef NO_OGG

#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <theora/theora.h>

/* Info needed for Ogg parsing, and Theora and Vorbis playback */
typedef struct ALOGG_INFO {
	ogg_sync_state osync;
	ogg_page opage;
	ogg_packet opkt;
	ogg_stream_state ostream[2];

	theora_state tstate;
	theora_comment tcomment;
	theora_info tinfo;

	vorbis_info      vinfo;
	vorbis_dsp_state vdsp;
	vorbis_block     vblock;
	vorbis_comment   vcomment;
} ALOGG_INFO;


/* Fills more data into the Ogg stream parser */
static INLINE int buffer_data(APEG_LAYER *layer, ALOGG_INFO *info)
{
	if(!pack_feof(layer->pf))
	{
		int i = 98304;
		unsigned char *buf = ogg_sync_buffer(&info->osync, i);

		i = pack_fread(buf, i, layer->pf);
		if(i < 0)
			i = 0;
		ogg_sync_wrote(&info->osync, i);
		return i;
	}
	return 0;
}


void alvorbis_close(APEG_LAYER *layer)
{
	ALOGG_INFO *info = layer->ogg_info;

	if(info && (layer->stream.flags&APEG_VORBIS_AUDIO))
	{
		vorbis_block_clear(&info->vblock);
		vorbis_dsp_clear(&info->vdsp);
		vorbis_comment_clear(&info->vcomment);
		vorbis_info_clear(&info->vinfo);
		ogg_stream_clear(&info->ostream[1]);
		layer->stream.flags &= ~APEG_VORBIS_AUDIO;
	}
}

static void altheora_close(APEG_LAYER *layer)
{
	ALOGG_INFO *info = layer->ogg_info;

	if(info && (layer->stream.flags&APEG_THEORA_VIDEO))
	{
		theora_clear(&info->tstate);
		theora_info_clear(&info->tinfo);
		theora_comment_clear(&info->tcomment);
		ogg_stream_clear(&info->ostream[0]);
		layer->stream.flags &= ~APEG_THEORA_VIDEO;
	}
}

/* Cleans and closes the Ogg player */
void alogg_cleanup(APEG_LAYER *layer)
{
	ALOGG_INFO *info = layer->ogg_info;

	alvorbis_close(layer);
	altheora_close(layer);

	if(info)
	{
		ogg_sync_clear(&info->osync);
		UNLOCK_DATA(info, sizeof(ALOGG_INFO));
		free(info);
	}

	layer->ogg_info = NULL;
}


/* Opens the Ogg stream, searching for and initializing Theora and Vorbis media
 */
int alogg_open(APEG_LAYER *layer)
{
	ALOGG_INFO *info;
	int vok = 0, aok = 0;
	int flag, cs, size;

	info = calloc(1, sizeof(ALOGG_INFO));
	if(!info)
		return APEG_ERROR;
	LOCK_DATA(info, sizeof(ALOGG_INFO));

	ogg_sync_init(&info->osync);

	theora_comment_init(&info->tcomment);
	theora_info_init(&info->tinfo);

	vorbis_info_init(&info->vinfo);
	vorbis_comment_init(&info->vcomment);

	flag = FALSE;
	while(!flag)
	{
		int ret = buffer_data(layer, info);
		if(ret == 0)
			break;

		while(ogg_sync_pageout(&info->osync, &info->opage) > 0)
		{
			ogg_stream_state test;

			/* is this a mandated initial header? If not, stop parsing */
			if(!ogg_page_bos(&info->opage))
			{
				if(vok > 0)
					ogg_stream_pagein(&info->ostream[0], &info->opage);
				if(aok > 0)
					ogg_stream_pagein(&info->ostream[1], &info->opage);
				flag = TRUE;
				break;
			}

			ogg_stream_init(&test, ogg_page_serialno(&info->opage));
			ogg_stream_pagein(&test, &info->opage);
			ogg_stream_packetout(&test, &info->opkt);

			/* identify the codec: try theora */
			if(!vok && theora_decode_header(&info->tinfo, &info->tcomment,
			                                &info->opkt) >= 0)
			{
				/* it is theora */
				if(!_apeg_ignore_video)
				{
					memcpy(&info->ostream[0], &test, sizeof(test));
					vok = 1;
				}
				else
					ogg_stream_clear(&test);
			}
			else if(!aok && vorbis_synthesis_headerin(&info->vinfo,
			                                &info->vcomment, &info->opkt) >= 0)
			{
				/* it is vorbis */
				if(!_apeg_ignore_audio)
				{
					memcpy(&info->ostream[1], &test, sizeof(test));
					aok = 1;
				}
				else
					ogg_stream_clear(&test);
			}
			/* whatever it is, we don't care about it */
			else
				ogg_stream_clear(&test);
		}
		/* fall through to non-bos page parsing */
	}

	/* look for further theora headers */
	while((vok > 0 && vok < 3) || (aok > 0 && aok < 3))
	{
		int ret;
		// Get the last two of three Theora headers
		while(vok > 0 && vok < 3 &&
		      (ret = ogg_stream_packetout(&info->ostream[0], &info->opkt)))
		{
			if(ret < 0)
				goto error;

			if(theora_decode_header(&info->tinfo, &info->tcomment, &info->opkt))
				goto error;

			++vok;
		}

		// Get the last two of three Vorbis headers
		while(aok > 0 && aok < 3 &&
		      (ret = ogg_stream_packetout(&info->ostream[1], &info->opkt)))
		{
			if(ret < 0)
				goto error;

			if(vorbis_synthesis_headerin(&info->vinfo, &info->vcomment,
			                             &info->opkt))
				goto error;

			++aok;
		}

		if(ogg_sync_pageout(&info->osync, &info->opage) <= 0)
		{
			/* need more data */
			if(buffer_data(layer, info) == 0)
				break;
		}
		else
		{
			if(vok > 0)
				ogg_stream_pagein(&info->ostream[0], &info->opage);
			if(aok > 0)
				ogg_stream_pagein(&info->ostream[1], &info->opage);
		}
    }

	// Neither Vorbis or Theora fully initialized. Error.
	if(vok != 3 && aok != 3)
		goto error;

	layer->ogg_info = info;

	if(aok == 3)
	{
		vorbis_synthesis_init(&info->vdsp, &info->vinfo);
		vorbis_block_init(&info->vdsp, &info->vblock);

		if(info->vinfo.channels == 1)
			layer->stream.audio.down_channel = FALSE;

		layer->stream.audio.channels = info->vinfo.channels;
		layer->stream.audio.freq = info->vinfo.rate >>
		                           layer->stream.audio.down_sample;

		if(_apeg_audio_reset_parameters(layer) != APEG_OK)
		{
			vorbis_block_clear(&info->vblock);
			vorbis_dsp_clear(&info->vdsp);
			goto error;
		}

//		layer->audio.inited = TRUE;
		layer->stream.flags |= APEG_VORBIS_AUDIO;
	}
	else
	{
		if(aok > 0)
			ogg_stream_clear(&info->ostream[1]);
		vorbis_info_clear(&info->vinfo);
		vorbis_comment_clear(&info->vcomment);
	}


	if(vok == 3)
	{
		byte *ptr;

		theora_decode_init(&info->tstate, &info->tinfo);

		layer->stream.w = info->tinfo.frame_width;
		layer->stream.h = info->tinfo.frame_height;
		layer->coded_width = info->tinfo.width;
		layer->coded_height = info->tinfo.height;
		layer->chroma_width = layer->coded_width/2;
		layer->chroma_height = layer->coded_height/2;

		layer->stream.fps_numerator = info->tinfo.fps_numerator;
		layer->stream.fps_denominator = info->tinfo.fps_denominator;
		layer->stream.frame_rate = (double)info->tinfo.fps_numerator /
		                           (double)info->tinfo.fps_denominator;

		layer->stream.aspect_numerator = info->tinfo.aspect_numerator;
		layer->stream.aspect_denominator = info->tinfo.aspect_denominator;
		if(info->tinfo.aspect_denominator)
		{
			double aspect = (double)info->tinfo.aspect_numerator /
			                (double)info->tinfo.aspect_denominator;
			layer->stream.aspect_ratio = (double)layer->stream.w /
			                             (double)layer->stream.h * aspect;
		}

		// Allocate frame pointers
		size = layer->coded_width*layer->coded_height +
			   (layer->chroma_width*layer->chroma_height)*2;

		free(layer->image_ptr);
		layer->image_ptr = malloc(size);
		if(!layer->image_ptr)
		{
			theora_clear(&info->tstate);
			goto error;
		}
		ptr = layer->image_ptr;

		layer->forward_frame[0] = ptr;
		ptr += layer->coded_width*layer->coded_height;
		layer->forward_frame[1] = ptr;
		ptr += layer->chroma_width*layer->chroma_height;
		layer->forward_frame[2] = ptr;
		ptr += layer->chroma_width*layer->chroma_height;

		// Init display
		switch(info->tinfo.colorspace)
		{
			case OC_CS_UNSPECIFIED:
				cs = 0;
				break;
			case OC_CS_ITU_REC_470M:
				cs = 1;
				break;
			case OC_CS_ITU_REC_470BG:
				cs = 2;
				break;
			default:
				cs = 0;
		}

		apeg_initialize_display(layer, cs);
		layer->stream.flags |= APEG_THEORA_VIDEO;
	}
	else
	{
		if(vok > 0)
			ogg_stream_clear(&info->ostream[0]);
		theora_info_clear(&info->tinfo);
		theora_comment_clear(&info->tcomment);
	}

	if(!_apeg_skip_length)
	{
	try_again:
		do {
			int ret;
			do {
				if((ret=ogg_sync_pageout(&info->osync, &info->opage)) > 0)
					break;
			} while((ret=buffer_data(layer, info)) > 0);
			if(ret <= 0)
				goto finish_up;
		} while(!ogg_page_eos(&info->opage));

		if((layer->stream.flags&APEG_THEORA_VIDEO))
		{
			if(ogg_stream_pagein(&info->ostream[0], &info->opage) == 0)
			{
				int64_t granulepos = ogg_page_granulepos(&info->opage);
				// Theora's granularity is dealt with by libtheora
				if(layer->stream.length < granulepos)
					layer->stream.length = theora_granule_time(&info->tstate,
					                                           granulepos);
			}
			else
				goto try_again;
		}
		else if((layer->stream.flags&APEG_VORBIS_AUDIO))
		{
			if(ogg_stream_pagein(&info->ostream[1], &info->opage) == 0)
			{
				int64_t granulepos = ogg_page_granulepos(&info->opage);
				// Vorbis's granularity is the sample rate.
				if(layer->stream.length < granulepos)
					layer->stream.length = (double)granulepos /
					                       (double)info->vinfo.rate;
			}
			else
				goto try_again;
		}

	finish_up:
		_apeg_initialize_buffer(layer);
		return alogg_reopen(layer);
	}

    return APEG_OK;

error:
	layer->ogg_info = NULL;

	ogg_sync_clear(&info->osync);
	if(vok > 0)
		ogg_stream_clear(&info->ostream[0]);
	theora_info_clear(&info->tinfo);
	theora_comment_clear(&info->tcomment);

	if(aok > 0)
		ogg_stream_clear(&info->ostream[1]);
	vorbis_info_clear(&info->vinfo);
	vorbis_comment_clear(&info->vcomment);

	free(info);

	return APEG_ERROR;
}


/* Decodes Theora video into the internal YUV image buffer */
unsigned char **altheora_get_frame(APEG_LAYER *layer)
{
	ALOGG_INFO *info = layer->ogg_info;
	int crop_offset, i;
	yuv_buffer yuv;

	do {
		/* theora is one in, one out... */
		if(ogg_stream_packetout(&info->ostream[0], &info->opkt) > 0)
		{
			theora_decode_packetin(&info->tstate, &info->opkt);
			if(info->tstate.granulepos >= 0)
				layer->stream.pos = theora_granule_time(&info->tstate,
				                                        info->tstate.granulepos);
			break;
		}

		do {
			if(ogg_sync_pageout(&info->osync, &info->opage) > 0)
			{
				if(ogg_stream_pagein(&info->ostream[0], &info->opage) == 0)
					break;

				if((layer->stream.flags&APEG_VORBIS_AUDIO))
					ogg_stream_pagein(&info->ostream[1], &info->opage);
			}
			else
			{
				if(!buffer_data(layer, info))
					return NULL;
			}
		} while(1);
	} while(1);

	theora_decode_YUVout(&info->tstate, &yuv);

	crop_offset = yuv.y_stride*info->tinfo.offset_y + info->tinfo.offset_x;
	for(i = 0;i < layer->stream.h;i++)
		memcpy(layer->forward_frame[0] + layer->coded_width*i,
		       yuv.y + crop_offset + yuv.y_stride*i,
		       layer->stream.w);

	switch(layer->stream.pixel_format)
	{
		case APEG_420:
			crop_offset = yuv.uv_stride*(info->tinfo.offset_y>>1) +
			              (info->tinfo.offset_x>>1);
			break;

		case APEG_422:
			crop_offset = yuv.uv_stride*info->tinfo.offset_y +
			              (info->tinfo.offset_x>>1);
			break;

		case APEG_444:
			crop_offset = yuv.uv_stride*info->tinfo.offset_y +
			              info->tinfo.offset_x;
			break;
	}

	for(i = 0;i < layer->chroma_height;i++)
	{
		memcpy(layer->forward_frame[1] + layer->chroma_width*i,
		       yuv.u + crop_offset + yuv.uv_stride*i,
		       layer->chroma_width);
		memcpy(layer->forward_frame[2] + layer->chroma_width*i,
		       yuv.v + crop_offset + yuv.uv_stride*i,
		       layer->chroma_width);
	}

	return layer->forward_frame;
}

/* Decodes Vorbis audio into the internal PCM sound buffer */
static void alvorbis_get_data(APEG_LAYER *layer)
{
	ALOGG_INFO *info = layer->ogg_info;
	float **pcm;
	int ret;

	do {
		/* if there's pending, decoded audio, grab it */
		if((ret = vorbis_synthesis_pcmout(&info->vdsp, &pcm)) > 0)
		{
			short *audiobuf = (short*)layer->audio.pcm.samples;
			int count = layer->audio.pcm.point / 2;
			int maxsamples = (layer->audio.bufsize-layer->audio.pcm.point) /
			                 2 / layer->stream.audio.channels;
			int step = 1 << layer->stream.audio.down_sample;
			int i, j;

			if(layer->stream.audio.down_channel)
			{
				// Zig-zag the between the channels to downmix into the stream.
				// This prevents channel blending or dropping.
				for(i = 0;i < ret && i < maxsamples;i+=step)
				{
					int val = ((pcm[(i/step)&1][i]+1.0f)*32767.5f);

					// Make sure to mix in the center voice channel
					if((info->vinfo.channels&1))
					{
						val += ((pcm[info->vinfo.channels-1][i]+1.0f)*32767.5f);
						val >>= 1;
					}

					val &= (~val) >> 31;
					val -= 65535;
					val &= val >> 31;
					val += 65535;
					audiobuf[count++] = val;
				}
			}
			else
			{
				if(info->vinfo.channels > layer->stream.audio.channels)
				{
					for(i = 0;i < ret && i < maxsamples;i+=step)
					{
						// Just in case.. if there's a center audio channel,
						// mix it in
						if(!(layer->stream.audio.channels&1) &&
						   (info->vinfo.channels&1))
						{
							int cval = (pcm[info->vinfo.channels-1][i]+1.0f) *
							           32767.5f * 0.70710678f;
							for(j = 0;j < 2;++j)
							{
								int val = (int)((pcm[j][i]+1.0f)*32767.5f) + cval;
								val &= (~val) >> 31;
								val -= 65535;
								val &= val >> 31;
								val += 65535;
								audiobuf[count++] = val;
							}
							for(;j < layer->stream.audio.channels;++j)
							{
								int val = ((pcm[j][i]+1.0f)*32767.5f);
								val &= (~val) >> 31;
								val -= 65535;
								val &= val >> 31;
								val += 65535;
								audiobuf[count++] = val;
							}
						}
						else if((layer->stream.audio.channels&1))
						{
							for(j = 0;j < layer->stream.audio.channels-1;++j)
							{
								int val = (int)((pcm[j][i]+1.0f)*32767.5f);
								val &= (~val) >> 31;
								val -= 65535;
								val &= val >> 31;
								val += 65535;
								audiobuf[count++] = val;
							}
							if((info->vinfo.channels&1))
							{
								int cval = (pcm[info->vinfo.channels-1][i] +
								            1.0f) * 32767.5f;
								cval &= (~cval) >> 31;
								cval -= 65535;
								cval &= cval >> 31;
								cval += 65535;
								audiobuf[count++] = cval;
							}
							else
								audiobuf[count++] = 0x8000;
						}
						else
						{
							for(j = 0;j < layer->stream.audio.channels;++j)
							{
								int val = ((pcm[j][i]+1.0f)*32767.5f);
								val &= (~val) >> 31;
								val -= 65535;
								val &= val >> 31;
								val += 65535;
								audiobuf[count++] = val;
							}
						}
					}
				}
				else
				{
					for(i = 0;i < ret && i < maxsamples;i+=step)
					{
						for(j = 0;j < layer->stream.audio.channels;++j)
						{
							int val = ((pcm[j][i]+1.0f)*32767.5f);
							val &= (~val) >> 31;
							val -= 65535;
							val &= val >> 31;
							val += 65535;
							audiobuf[count++] = val;
						}
					}
				}
			}

			vorbis_synthesis_read(&info->vdsp, i);
			layer->audio.pcm.point = count * 2;
			if(layer->audio.pcm.point >= layer->audio.bufsize)
				return;
			continue;
		}
		else
		{
			// Need another packet
			if(ogg_stream_packetout(&info->ostream[1], &info->opkt) > 0)
			{
				// test for success!
				if(vorbis_synthesis(&info->vblock, &info->opkt) == 0)
				{
					// get another audio block
					vorbis_synthesis_blockin(&info->vdsp, &info->vblock);
					continue;
				}
			}
		}

		// No data. Grab some more until we get an audio page or EOF
		do {
			if(ogg_sync_pageout(&info->osync, &info->opage) > 0)
			{
				if(ogg_stream_pagein(&info->ostream[1], &info->opage) == 0)
					break;

				if((layer->stream.flags&APEG_THEORA_VIDEO))
					ogg_stream_pagein(&info->ostream[0], &info->opage);
			}
			else
			{
				if(!buffer_data(layer, info))
					return;
			}
		} while(1);
	} while(1);
}

int alvorbis_update(APEG_LAYER *layer)
{
	if(layer->audio.pcm.point < layer->audio.bufsize)
		alvorbis_get_data(layer);

	return _apeg_audio_flush(layer);
}


int alogg_reopen(APEG_LAYER *layer)
{
	ALOGG_INFO *info = layer->ogg_info;
	int vok = 0, aok = 0;
	int flag;

	if((layer->stream.flags&APEG_THEORA_VIDEO))
	{
		theora_clear(&info->tstate);
		theora_info_clear(&info->tinfo);
		theora_comment_clear(&info->tcomment);
		ogg_stream_clear(&info->ostream[0]);
		layer->stream.flags &= ~APEG_THEORA_VIDEO;

		theora_comment_init(&info->tcomment);
		theora_info_init(&info->tinfo);
	}
	else
		vok = -1;

	if((layer->stream.flags&APEG_VORBIS_AUDIO))
	{
		vorbis_block_clear(&info->vblock);
		vorbis_dsp_clear(&info->vdsp);
		vorbis_comment_clear(&info->vcomment);
		vorbis_info_clear(&info->vinfo);
		ogg_stream_clear(&info->ostream[1]);
		layer->stream.flags &= ~APEG_VORBIS_AUDIO;

		vorbis_info_init(&info->vinfo);
		vorbis_comment_init(&info->vcomment);
	}
	else
		aok = -1;

	ogg_sync_clear(&info->osync);
	ogg_sync_init(&info->osync);

	flag = FALSE;
	while(!flag)
	{
		int ret = buffer_data(layer, info);
		if(ret == 0)
			break;

		while(ogg_sync_pageout(&info->osync, &info->opage) > 0)
		{
			ogg_stream_state test;

			/* is this a mandated initial header? If not, stop parsing */
			if(!ogg_page_bos(&info->opage))
			{
				if(vok > 0)
					ogg_stream_pagein(&info->ostream[0], &info->opage);
				if(aok > 0)
					ogg_stream_pagein(&info->ostream[1], &info->opage);
				flag = TRUE;
				break;
			}

			ogg_stream_init(&test, ogg_page_serialno(&info->opage));
			ogg_stream_pagein(&test, &info->opage);
			ogg_stream_packetout(&test, &info->opkt);

			/* identify the codec */
			if(!vok && theora_decode_header(&info->tinfo, &info->tcomment, &info->opkt) >= 0)
			{
				/* it is theora */
				memcpy(&info->ostream[0], &test, sizeof(test));
				vok = 1;
			}
			else if(!aok && vorbis_synthesis_headerin(&info->vinfo, &info->vcomment,&info->opkt) >= 0)
			{
				/* it is vorbis */
				memcpy(&info->ostream[1], &test, sizeof(test));
				aok = 1;
			}
			/* whatever it is, we don't care about it */
			else
				ogg_stream_clear(&test);
		}
		/* fall through to non-bos page parsing */
	}

	/* look for further theora headers */
	while((vok > 0 && vok < 3) || (aok > 0 && aok < 3))
	{
		int ret;
		// Get the last two of three Theora headers
		while(vok > 0 && vok < 3 &&
		      (ret = ogg_stream_packetout(&info->ostream[0], &info->opkt)))
		{
			if(ret < 0)
				goto error;

			if(theora_decode_header(&info->tinfo, &info->tcomment, &info->opkt))
				goto error;

			++vok;
		}

		// Get the last two of three Vorbis headers
		while(aok > 0 && aok < 3 &&
		      (ret = ogg_stream_packetout(&info->ostream[1], &info->opkt)))
		{
			if(ret < 0)
				goto error;

			if(vorbis_synthesis_headerin(&info->vinfo, &info->vcomment, &info->opkt))
				goto error;

			++aok;
		}

		if(ogg_sync_pageout(&info->osync, &info->opage) <= 0)
		{
			/* need more data */
			if(buffer_data(layer, info) == 0)
				break;
		}
		else
		{
			if(vok > 0)
				ogg_stream_pagein(&info->ostream[0], &info->opage);
			if(aok > 0)
				ogg_stream_pagein(&info->ostream[1], &info->opage);
		}
    }

	// Neither Vorbis or Theora fully initialized. Error.
	if(vok != 3 && aok != 3)
		goto error;

	if(aok == 3)
	{
		vorbis_synthesis_init(&info->vdsp, &info->vinfo);
		vorbis_block_init(&info->vdsp, &info->vblock);
		layer->stream.flags |= APEG_VORBIS_AUDIO;
	}
	else if(aok >= 0)
	{
		if(aok > 0)
			ogg_stream_clear(&info->ostream[1]);
		vorbis_info_clear(&info->vinfo);
		vorbis_comment_clear(&info->vcomment);
	}

	if(vok == 3)
	{
		theora_decode_init(&info->tstate, &info->tinfo);
		layer->stream.flags |= APEG_THEORA_VIDEO;
	}
	else if(vok >= 0)
	{
		if(vok > 0)
			ogg_stream_clear(&info->ostream[0]);
		theora_info_clear(&info->tinfo);
		theora_comment_clear(&info->tcomment);
	}

    return APEG_OK;

error:
	layer->ogg_info = NULL;

	ogg_sync_clear(&info->osync);
	if(vok >= 0)
	{
		if(vok > 0)
			ogg_stream_clear(&info->ostream[0]);
		theora_info_clear(&info->tinfo);
		theora_comment_clear(&info->tcomment);
	}

	if(aok >= 0)
	{
		if(aok > 0)
			ogg_stream_clear(&info->ostream[1]);
		vorbis_info_clear(&info->vinfo);
		vorbis_comment_clear(&info->vcomment);
	}

	free(info);

	return APEG_ERROR;
}

#else

void alvorbis_close(APEG_LAYER *layer)
{
	(void)layer;
}

void alogg_cleanup(APEG_LAYER *layer)
{
	free(layer->ogg_info);
	layer->ogg_info = NULL;
}

int alogg_open(APEG_LAYER *layer)
{
	return 0;
	(void)layer;
}

unsigned char **altheora_get_frame(APEG_LAYER *layer)
{
	return NULL;
	(void)layer;
}

int alvorbis_update(APEG_LAYER *layer)
{
	return APEG_EOF;
	(void)layer;
}

int alogg_reopen(APEG_LAYER *layer)
{
	return APEG_ERROR;
	(void)layer;
}

#endif
