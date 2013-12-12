#ifndef DISABLE_MPEG_AUDIO
#define DISABLE_MPEG_AUDIO
#endif
/* mpeg1dec.c, initialization, option processing                    */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <setjmp.h>
#include <math.h>

#include <allegro.h>
#include "apeg.h"

#include "mpeg1dec.h"
#include "mpg123.h"

// Default stream
APEG_STREAM *apeg_stream;

// Error string
char apeg_error[256];

// Static variables for setting buffer sizes
static int mem_buffer_size = -1;

// External data reader callbacks
static int (*_init_func)(void *ptr);
static int (*_request_func)(void *bfr, int bytes, void *ptr);
static void (*_skip_func)(int bytes, void *ptr);

// Quality setting (for frame reconstruction)
static int quality = RECON_SUBPIXEL | RECON_AVG_SUBPIXEL;

// Whether we should drop frames or try to let the CPU catch up
static int framedrop = FALSE;

// Jump point for if the decoder hits a fault
static jmp_buf jmp_buffer;

// private prototypes
static int decode_stream(APEG_LAYER *layer, BITMAP *target);
static void initialize_stream(APEG_LAYER *layer);
static void check_stream_type(APEG_LAYER *layer);
static void setup_stream(APEG_LAYER *layer);


// Packfile vtable to read from memory sources
static int mem_close(void *_f)
{ return 0; (void)_f; }

static int mem_getc(void *_f)
{
	APEG_LAYER *layer = _f;
	if(layer->mem_data.bytes_left > 0)
	{
		layer->mem_data.bytes_left -= 1;
		return *(layer->mem_data.buf++);
	}
	return EOF;
}

static int mem_ungetc(int c, void *_f)
{
	APEG_LAYER *layer = _f;
	layer->mem_data.bytes_left += 1;
	*(--(layer->mem_data.buf)) = c&255;
	return c&255;
}

static long mem_fread(void *p, long n, void *_f)
{
	APEG_LAYER *layer = _f;
	if(layer->mem_data.bytes_left < (unsigned int)n)
		n = layer->mem_data.bytes_left;
	if(n > 0)
		memcpy(p, layer->mem_data.buf, n);
	layer->mem_data.buf += n;
	layer->mem_data.bytes_left -= n;
	return n;
}

static int mem_putc(int c, void *_f)
{ return EOF; (void)c; (void)_f; }

static long mem_fwrite(AL_CONST void *p, long n, void *_f)
{ return EOF; (void)p; (void)n; (void)_f; }

static int mem_fseek(void *_f, int offset)
{
	APEG_LAYER *layer = _f;
	if(layer->mem_data.bytes_left < (unsigned int)offset)
		offset = layer->mem_data.bytes_left;
	layer->mem_data.bytes_left -= offset;
	layer->mem_data.buf += offset;
	return 0;
}

static int mem_feof(void *_f)
{
	APEG_LAYER *layer = _f;
	if(layer->mem_data.bytes_left)
		return FALSE;
	return TRUE;
}

static int mem_ferror(void *_f)
{ return 0; (void)_f; }

static PACKFILE_VTABLE mem_vtable =
{
	mem_close,
	mem_getc,
	mem_ungetc,
	mem_fread,
	mem_putc,
	mem_fwrite,
	mem_fseek,
	mem_feof,
	mem_ferror
};

// Packfile vtable to read from external sources
static int ext_close(void *_f)
{ return 0; (void)_f; }

static int ext_getc(void *_f)
{
	APEG_LAYER *layer = _f;
	unsigned char b;

	if(layer->ext_data.request(&b, 1, layer->ext_data.ptr) == 0)
		return EOF;
	return b;
}

static int ext_ungetc(int c, void *_f)
{ return EOF; (void)c; (void)_f; }

static long ext_fread(void *p, long n, void *_f)
{
	APEG_LAYER *layer = _f;
	if(!layer->ext_data.eof)
	{
		long r = layer->ext_data.request(p, n, layer->ext_data.ptr);
		if(r < n)
			layer->ext_data.eof = TRUE;
		return r;
	}
	return 0;
}

static int ext_putc(int c, void *_f)
{ return EOF; (void)c; (void)_f; }

static long ext_fwrite(AL_CONST void *p, long n, void *_f)
{ return EOF; (void)p; (void)n; (void)_f; }

static int ext_fseek(void *_f, int offset)
{
	APEG_LAYER *layer = _f;
	if(!layer->ext_data.eof)
		layer->ext_data.skip(offset, layer->ext_data.ptr);
	return 0;
}

static int ext_feof(void *_f)
{
	APEG_LAYER *layer = _f;
	return layer->ext_data.eof;
}

static int ext_ferror(void *_f)
{ return 0; (void)_f; }

static PACKFILE_VTABLE ext_vtable =
{
	ext_close,
	ext_getc,
	ext_ungetc,
	ext_fread,
	ext_putc,
	ext_fwrite,
	ext_fseek,
	ext_feof,
	ext_ferror
};


// The timer.. increments the int pointed to by 'param'
static void timer_proc(void *param)
{
	++(*(volatile int*)param);
}
END_OF_STATIC_FUNCTION(timer_proc);


// Gets called at the beginning of every "open" function, to ensure
// things are initialized
static void Initialize_Decoder(void)
{
	static int inited = FALSE;

	if(!inited)
	{
		// Lock the timer function
		LOCK_FUNCTION(timer_proc);
		inited = TRUE;
	}

	memset(apeg_error, 0, sizeof(apeg_error));
}

// The default callback; requires Allegro's keyboard handler
static int default_callback(BITMAP *tempBuffer)
{
	if(keypressed())
		return 1;

	return 0;
}
static int (*callback_proc)(BITMAP* tempBuffer);

static APEG_LAYER *new_apeg_stream(void)
{
	APEG_LAYER *layer;

	// Allocate the layer data, lock it, and hook it into the stream
	layer = calloc(1, sizeof(APEG_LAYER));
	if(!layer)
		return NULL;
	LOCK_DATA(layer, sizeof(APEG_LAYER));

	/* TODO: Should replace this with a dedicated function to clear the audio
	         state */
	_apeg_start_audio(layer, FALSE);

	return layer;
}

// Closes an opened APEG_STREAM
void apeg_close_stream(APEG_STREAM *stream)
{
	APEG_LAYER *layer = (APEG_LAYER*)stream;
	if(layer)
	{
		_apeg_start_audio(layer, FALSE);

		remove_param_int(timer_proc, (void*)&(layer->stream.timer));

		destroy_bitmap(layer->stream.bitmap);

		alogg_cleanup(layer);

		free(layer->fname);

		free(layer->image_ptr);
		free(layer->audio.pcm.samples);

		UNLOCK_DATA(layer, sizeof(APEG_LAYER));
		free(layer);
	}
}


// Open an MPEG or Ogg file. Returns a structure for stream manipulation.
APEG_STREAM *apeg_open_stream(const char *filename)
{
	APEG_LAYER *layer;

	// Initialize the decoder
	Initialize_Decoder();

	// Create a new stream structure
	layer = new_apeg_stream();
	if(!layer)
		return NULL;

	// Set the jump position in case the decoder faults on opening.
	if(setjmp(jmp_buffer))
	{
		apeg_close_stream((APEG_STREAM*)layer);
		return NULL;
	}

	// Copy the filename so we can (re)open the file when the buffer
	// gets initialized
	layer->fname = strdup(filename);
	if(!layer->fname)
		apeg_error_jump("Couldn't duplicate filename");

	// Set the buffer type and size
	layer->buffer_type = DISK_BUFFER;
	layer->buffer_size = F_BUF_SIZE;

	setup_stream(layer);

	return (APEG_STREAM*)layer;
}

APEG_STREAM *apeg_open_memory_stream(void *mpeg_data, int data_len)
{
	APEG_LAYER *layer;

	Initialize_Decoder();

	layer = new_apeg_stream();
	if(!layer)
		return NULL;

	if(setjmp(jmp_buffer))
	{
		apeg_close_stream((APEG_STREAM*)layer);
		return NULL;
	}

	layer->mem_data.buf = mpeg_data;
	layer->mem_data.bytes_left = data_len;
	layer->pf = pack_fopen_vtable(&mem_vtable, layer);
	if(!layer->pf)
		apeg_error_jump("Could not open stream");

	layer->buffer_type = MEMORY_BUFFER;
	layer->buffer_size = data_len;

	setup_stream(layer);

	return (APEG_STREAM*)layer;
}

APEG_STREAM *apeg_open_stream_ex(void *ptr)
{
	APEG_LAYER *layer;

	Initialize_Decoder();

	layer = new_apeg_stream();
	if(!layer)
		return NULL;

	if(setjmp(jmp_buffer))
	{
		apeg_close_stream((APEG_STREAM*)layer);
		return NULL;
	}

	layer->ext_data.init = _init_func;
	layer->ext_data.request = _request_func;
	layer->ext_data.skip = _skip_func;
	layer->ext_data.ptr = ptr;

	layer->pf = pack_fopen_vtable(&ext_vtable, layer);
	if(!layer->pf)
		apeg_error_jump("Could not open stream");
	layer->buffer_type = USER_BUFFER;

	setup_stream(layer);

	return (APEG_STREAM*)layer;
}


int apeg_advance_stream(APEG_STREAM *stream, int loop)
{
	APEG_LAYER *layer = (APEG_LAYER*)stream;
	int ret;

	// Set the jump buffer
	if((ret = setjmp(jmp_buffer)) != 0)
		return ret;

	layer->stream.frame_updated = -1;
	layer->stream.audio.flushed = FALSE;
	ret = APEG_OK;

	if((layer->stream.flags&APEG_HAS_AUDIO) && layer->multiple > 0.0)
	{
		ret = _apeg_audio_poll(layer);
		if((layer->stream.flags&APEG_HAS_VIDEO))
		{
			int t = _apeg_audio_get_position(layer);
			if(t >= 0)
				layer->stream.timer = (double)t*stream->frame_rate/
				                      (double)stream->audio.freq -
				                      stream->frame;
		}
	}

	if((layer->stream.flags&APEG_HAS_VIDEO))
	{
		if(!layer->picture)
		{
			if((layer->stream.flags&APEG_MPG_VIDEO))
			{
				// Get the next MPEG header
				if(apeg_get_header(layer) == 1)
				{
					// Decode the next picture
					if(layer->picture_type != B_TYPE ||
					   !framedrop || layer->stream.timer <= 1)
						layer->picture = apeg_get_frame(layer);
				}
				// If end of stream, display the last frame
				else if((!framedrop || layer->stream.timer <= 1) &&
				        !layer->got_last)
				{
					layer->got_last = TRUE;
					layer->picture = layer->backward_frame;
				}
			}
			else
				layer->picture = altheora_get_frame(layer);

			if(pack_feof(layer->pf) && (!(layer->stream.flags&APEG_HAS_AUDIO)||
			                            layer->multiple==0.0 ||
			                            ret!=APEG_OK))
				ret = APEG_EOF;
		}

		if(layer->stream.timer > 0)
		{
			// Update frame and timer count
			++(layer->stream.frame);
			--(layer->stream.timer);

			// If we're not behind, update the display frame
			layer->stream.frame_updated = 0;
			if(layer->picture && (!framedrop || layer->stream.timer == 0))
				apeg_display_frame(layer, layer->picture);
			layer->picture = NULL;
		}
		if(layer->stream.frame_updated == 1 || layer->picture)
			ret = APEG_OK;
	}

	if(ret == APEG_EOF && loop)
		ret = apeg_reset_stream((APEG_STREAM*)layer);

	return ret;
}

int apeg_reset_stream(APEG_STREAM *stream)
{
	APEG_LAYER *layer = (APEG_LAYER*)stream;
	int ret;

	if((ret = setjmp(jmp_buffer)) != 0)
		return ret;

	// Reset frame count
	layer->stream.frame = 0;
	layer->audio.last_pos = -1;
	layer->audio.pos = -layer->audio.samples_per_update*2;

	// Reinitialize the buffer
	_apeg_initialize_buffer(layer);

	if(layer->system_stream_flag == OGG_SYSTEM)
	{
		if(alogg_reopen(layer) != APEG_OK)
			apeg_error_jump("Could not reopen Ogg stream");

		return APEG_OK;
	}

	if((layer->stream.flags&APEG_HAS_AUDIO))
		_apeg_start_audio(layer, TRUE);

	if(layer->stream.frame_rate > 0.0f)
		return APEG_OK;

	if(apeg_get_header(layer) != 1)
		apeg_error_jump("No video in sequence");

	apeg_get_frame(layer);

	return APEG_OK;
}

int apeg_play_apeg_stream(APEG_STREAM *stream_to_play, BITMAP *bmp, int loop, int (*callback)(BITMAP*tempBuffer))
{
	int ret = APEG_OK;
  apeg_stream = stream_to_play;

	Initialize_Decoder();

	if(bmp)
		clear_to_color(bmp, makecol(0, 0, 0));

	// Install the callback function
	if(callback)
		callback_proc = callback;
	else
		callback_proc = default_callback;

restart_loop:
	ret = decode_stream((APEG_LAYER*)apeg_stream, bmp);
	if(loop && ret == APEG_OK)
	{
		apeg_reset_stream(apeg_stream);
		goto restart_loop;
	}

	apeg_stream = NULL;

	return ret;
}

int apeg_play_memory_mpg(void *mpeg_data, BITMAP *bmp, int loop, int (*callback)(BITMAP *tempBuffer))
{
	int ret;

	Initialize_Decoder();

	apeg_stream = apeg_open_memory_stream(mpeg_data, mem_buffer_size);
	if(!apeg_stream)
		return APEG_ERROR;

	if(bmp)
		clear_to_color(bmp, makecol(0, 0, 0));

	if(callback)
		callback_proc = callback;
	else
		callback_proc = default_callback;

restart_loop:
	ret = decode_stream((APEG_LAYER*)apeg_stream, bmp);
	if(loop && ret == APEG_OK)
	{
		apeg_reset_stream(apeg_stream);
		goto restart_loop;
	}

	apeg_close_stream(apeg_stream);
	apeg_stream = NULL;

	return ret;
}


int apeg_play_mpg_ex(void *ptr, BITMAP *bmp, int loop, int (*callback)(BITMAP*))
{
	int ret;

	Initialize_Decoder();

	apeg_stream = apeg_open_stream_ex(ptr);
	if(!apeg_stream)
		return APEG_ERROR;

	if(bmp)
		clear_to_color(bmp, makecol(0, 0, 0));

	if(callback)
		callback_proc = callback;
	else
		callback_proc = default_callback;

restart_loop:
	ret = decode_stream((APEG_LAYER*)apeg_stream, bmp);
	if(loop && ret == APEG_OK)
	{
		apeg_reset_stream(apeg_stream);
		goto restart_loop;
	}

	apeg_close_stream(apeg_stream);
	apeg_stream = NULL;

	return ret;
}


void apeg_set_disk_buffer_size(int size)
{
	(void)size;
}

void apeg_set_memory_stream_size(int size)
{
	if(size < 4)
		size = -1;

	mem_buffer_size = size;
}

void apeg_set_stream_reader(int (*init_func)(void *ptr), int (*request_func)(void *bfr, int bytes, void *ptr), void (*skip_func)(int bytes, void *ptr))
{
	_init_func = init_func;
	_request_func = request_func;
	_skip_func = skip_func;
}


int _apeg_ignore_video = FALSE;
int apeg_ignore_video(int ignore)
{
	int last = _apeg_ignore_audio;
	_apeg_ignore_video = ignore;
	return last;
}

void apeg_set_quality(int quality)
{
	if(quality == 0)
		quality = 0;
	else if(quality == 1)
		quality = RECON_SUBPIXEL;
	else
		quality = RECON_SUBPIXEL | RECON_AVG_SUBPIXEL;
}

void apeg_get_video_size(APEG_STREAM *stream, int *w, int *h)
{
	ASSERT(stream);

	if(stream->aspect_ratio)
	{
		if(stream->aspect_ratio > (float)stream->w/(float)stream->h)
		{
			*w = (int)floor(stream->h*stream->aspect_ratio);
			*h = stream->h;
		}
		else
		{
			*w = stream->w;
			*h = (int)floor(stream->w/stream->aspect_ratio);
		}
	}
	else
	{
		*w = stream->w;
		*h = stream->h;
	}
}

void apeg_set_stream_rate(APEG_STREAM *stream, float multiple)
{
	if(multiple < 0.0f)
		return;

	if((stream->flags&APEG_HAS_AUDIO))
		_apeg_audio_set_speed_multiple((APEG_LAYER*)stream, multiple);

	if((stream->flags&APEG_HAS_VIDEO))
	{
		// Make sure the multiple is large enough
		float fps = stream->frame_rate * multiple;
		if(fps > 0.0f)
			install_param_int_ex(timer_proc, (void*)&(stream->timer),
			                     FPS_TO_TIMER(fps));
		else
			remove_param_int(timer_proc, (void*)&(stream->timer));
	}

	((APEG_LAYER*)stream)->multiple = multiple;
}


void apeg_enable_framedrop(int enable)
{
	framedrop = !!enable;
}

int _apeg_skip_length;
void apeg_disable_length_detection(int disable)
{
	_apeg_skip_length = !!disable;
}


static int decode_stream(APEG_LAYER *layer, BITMAP *target)
{
	int ret;
	int dw, dh;

	if((ret = setjmp(jmp_buffer)) != 0)
		return ret;

	apeg_get_video_size(&layer->stream, &dw, &dh);

	layer->stream.frame = 0;
	if((layer->stream.flags & APEG_HAS_VIDEO))
	{
		layer->stream.timer = -1;
		while(layer->stream.timer == -1)
			;
	}

	// loop through the pictures in the sequence
	do {
		layer->stream.frame_updated = -1;
		layer->stream.audio.flushed = FALSE;
		ret = APEG_OK;

		if((layer->stream.flags & APEG_HAS_AUDIO) && layer->multiple > 0.0)
		{
			ret = _apeg_audio_poll(layer);
			if((layer->stream.flags & APEG_HAS_VIDEO))
			{
				int t = _apeg_audio_get_position(layer);
				if(t >= 0)
					layer->stream.timer = (double)t*layer->stream.frame_rate/
					                      (double)layer->stream.audio.freq -
					                      layer->stream.frame;
			}
		}

		if((layer->stream.flags & APEG_HAS_VIDEO))
		{
			if(!layer->picture)
			{
				if((layer->stream.flags&APEG_MPG_VIDEO))
				{
					// Get the next MPEG header
					if(apeg_get_header(layer) == 1)
					{
						// Decode the next picture
						if(layer->picture_type != B_TYPE ||
						   !framedrop || layer->stream.timer <= 1)
							layer->picture = apeg_get_frame(layer);
					}
					// If end of stream, display the last frame
					else if((!framedrop || layer->stream.timer <= 1) &&
					        !layer->got_last)
					{
						layer->got_last = TRUE;
						layer->picture = layer->backward_frame;
					}
				}
				else
					layer->picture = altheora_get_frame(layer);

				if(pack_feof(layer->pf) &&
				   (!(layer->stream.flags&APEG_HAS_AUDIO)||
				    layer->multiple==0.0 || ret!=APEG_OK))
					ret = APEG_EOF;
			}

			if(layer->stream.timer > 0)
			{
				// Update frame and timer count
				++(layer->stream.frame);
				--(layer->stream.timer);

				// If we're not behind, update the display frame
				layer->stream.frame_updated = 0;
				if(layer->picture && (!framedrop || layer->stream.timer == 0))
				{
					apeg_display_frame(layer, layer->picture);
				}
				layer->picture = NULL;
			}
			if(layer->stream.frame_updated == 1 || layer->picture)
				ret = APEG_OK;
		}

		if(ret == APEG_OK)
		{
			if((!(layer->stream.flags & APEG_HAS_VIDEO) ||
			    layer->stream.frame_updated >= 0) && (ret = callback_proc(layer->stream.bitmap)))
				break;

			if(layer->stream.frame_updated < 0 && !layer->stream.audio.flushed)
				rest(1);
		}
	} while(ret == APEG_OK);

	return ret;
}


static void setup_stream(APEG_LAYER *layer)
{
	// Set quality level
	layer->quality = quality;

	layer->multiple = 1.0f;

	// Set stream type
	check_stream_type(layer);

	// No frames decoded, frame not updated
	layer->stream.frame = 0;
	layer->stream.frame_updated = -1;

	_apeg_start_audio(layer, TRUE);

	if((layer->stream.flags&APEG_HAS_VIDEO))
	{
		if(layer->system_stream_flag != OGG_SYSTEM)
		{
			if(apeg_get_header(layer) != 1)
				apeg_error_jump("No video in stream");

			// Initialize the stream
			initialize_stream(layer);

			// Decode the first frame(doesn't display!)
			apeg_get_frame(layer);
		}

		// Start the timer
		if(layer->stream.frame_rate <= 0.0)
			apeg_error_jump("Illegal frame rate in stream");

		install_param_int_ex(timer_proc, (void*)&(layer->stream.timer),
		                     FPS_TO_TIMER(layer->stream.frame_rate));

		// Reset the timer and return the stream
		layer->stream.timer = -1;
	}
}

static void initialize_stream(APEG_LAYER *layer)
{
	unsigned char *ptr;
	int size;

	if(layer->stream.w < 16 || layer->stream.h < 16)
		apeg_error_jump("Illegal video size");

	// round to greatest multiple of coded macroblocks
	layer->mb_cols = (layer->stream.w+15)/16;
	layer->mb_rows = (layer->stream.h+15)/16;

	layer->coded_width = 16*layer->mb_cols;
	layer->coded_height = 16*layer->mb_rows;

	layer->chroma_width = layer->coded_width>>1;
	layer->chroma_height = layer->coded_height>>1;

	// Allocate frame pointers
	size = layer->coded_width*layer->coded_height + layer->coded_width*16 +
	       (layer->chroma_width*layer->chroma_height+layer->chroma_width*8)*2;

	free(layer->image_ptr);
	layer->image_ptr = malloc(size*3);
	if(!layer->image_ptr)
		apeg_error_jump("Frame malloc failed");
	ptr = layer->image_ptr;

	layer->forward_frame[0] = ptr;
	ptr += layer->coded_width*layer->coded_height + layer->coded_width*16;
	layer->backward_frame[0] = ptr;
	ptr += layer->coded_width*layer->coded_height + layer->coded_width*16;
	layer->auxframe[0] = ptr;
	ptr += layer->coded_width*layer->coded_height + layer->coded_width*16;

	layer->forward_frame[1] = ptr;
	ptr += layer->chroma_width*layer->chroma_height + layer->chroma_width*8;
	layer->backward_frame[1] = ptr;
	ptr += layer->chroma_width*layer->chroma_height + layer->chroma_width*8;
	layer->auxframe[1] = ptr;
	ptr += layer->chroma_width*layer->chroma_height + layer->chroma_width*8;

	layer->forward_frame[2] = ptr;
	ptr += layer->chroma_width*layer->chroma_height + layer->chroma_width*8;
	layer->backward_frame[2] = ptr;
	ptr += layer->chroma_width*layer->chroma_height + layer->chroma_width*8;
	layer->auxframe[2] = ptr;
	ptr += layer->chroma_width*layer->chroma_height + layer->chroma_width*8;

	// Start display process
	apeg_initialize_display(layer, 1);
}

void apeg_error_jump(char *text)
{
	if(text)
		strncpy(apeg_error, text, sizeof(apeg_error));

	longjmp(jmp_buffer, APEG_ERROR);
}


static void check_stream_type(APEG_LAYER *layer)
{
	// Assume a non-system stream
	layer->system_stream_flag = NO_SYSTEM;

	// Initialize the buffer
	_apeg_initialize_buffer(layer);

	// Transport streams (what'r those?) aren't supported
	if(show_bits(layer, 8) == 0x47)
		apeg_error_jump("Transport streams not supported");

	/* Get the first start code */
recheck:
	switch(show_bits32(layer))
	{
		case VIDEO_ELEMENTARY_STREAM:
			/* Found video, system stream */
			layer->system_stream_flag = MPEG_SYSTEM;
			/* fall-through; set flag and break */
		case SEQUENCE_HEADER_CODE:
			/* Found video */
			if(!_apeg_ignore_video)
				layer->stream.flags |= APEG_MPG_VIDEO;
			break;

		case AUDIO_ELEMENTARY_STREAM:
			/* apeg_start_audio will set APEG_MPG_AUDIO later */
			layer->system_stream_flag = MPEG_SYSTEM;
			apeg_start_code(layer);
			goto recheck;

		default:
			if(layer->system_stream_flag == NO_SYSTEM)
			{
				if(show_bits32(layer) == (('O'<<24)|('g'<<16)|('g'<<8)|('S')))
				{
					layer->system_stream_flag = OGG_SYSTEM;
					_apeg_initialize_buffer(layer);
					if(alogg_open(layer) != APEG_OK)
						apeg_error_jump("Error opening Ogg stream");
					return;
				}
#ifndef DISABLE_MPEG_AUDIO
				if(almpa_head_backcheck(show_bits32(layer)))
					break;
#endif
			}

			/* no positive stream identified; recheck */
			apeg_flush_bits8(layer, 8);
			goto recheck;
	}

	_apeg_initialize_buffer(layer);
}


/* initialize buffer, call once before first getbits or showbits */
void _apeg_initialize_buffer(APEG_LAYER *layer)
{
	layer->Incnt = 0;
	layer->got_last = FALSE;

	switch(layer->buffer_type)
	{
		case DISK_BUFFER:
			// (Re)open the file
			if(layer->pf)
				pack_fclose(layer->pf);

			layer->pf = pack_fopen(layer->fname, F_READ);
			if(!layer->pf)
			{
				sprintf(apeg_error, "Couldn't open %s", layer->fname);
				apeg_error_jump(NULL);
			}
			break;

		case MEMORY_BUFFER:
			layer->mem_data.buf -= layer->buffer_size -
			                       layer->mem_data.bytes_left;
			layer->mem_data.bytes_left = layer->buffer_size;
			break;

		case USER_BUFFER:
			if(!layer->ext_data.request || !layer->ext_data.skip ||
			   !layer->ext_data.init)
				apeg_error_jump("Unable to request data");

			layer->buffer_size = layer->ext_data.init(layer->ext_data.ptr);
			if(layer->buffer_size <= 0)
				apeg_error_jump("Data init failed");
			layer->buffer_size = (layer->buffer_size+3) & (~3);
			break;

		default:
			sprintf(apeg_error, "Unknown buffer type: %d", layer->buffer_type);
			apeg_error_jump(NULL);
	}

	layer->Bfr = 0;
	layer->Incnt = 0;
	layer->Rdmax = 0;
	if(layer->system_stream_flag != MPEG_SYSTEM)
		layer->Rdmax = ~0u;
	if(layer->system_stream_flag != OGG_SYSTEM)
		apeg_flush_bits(layer, 0); /* fills valid data into Bfr */
}
