#ifndef DISABLE_MPEG_AUDIO

#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "apeg.h"

#include "mpg123.h"
#include "common.h"

/*******************************************************************
 * stream based operation
 */

static void read_bytes(APEG_LAYER *layer, byte *buf, int count)
{
	struct reader *rds = &(layer->audio.rd);
	int i = 0;

	/* Read bytes from the source if there's anything left */
	if(!rds->eof)
	{
		if(layer->buffer_type == DISK_BUFFER)
			i = pack_fread(buf, count, (PACKFILE*)rds->filept);
		else if(layer->buffer_type == MEMORY_BUFFER)
		{
			i = (unsigned)layer->buffer_size - (unsigned)rds->filepos;
			if(i > count)
				i = count;
			memcpy(buf, rds->filept, i);
			((unsigned char*)rds->filept) += i;
		}

		if(i < count)
		{
			rds->eof = TRUE;
			if(i < 0)
				i = 0;
		}

		rds->filepos += i;
		rds->left_in_packet -= i;
	}

	/* Fill remaining data with end codes */
	while(i < count)
	{
		static int p = 0;
		unsigned int def = ISO_END_CODE;
		buf[i] = ((unsigned char*)&def)[(--p)&3];
		++i;
	}
}


static INLINE unsigned int get_byte(APEG_LAYER *layer)
{
	unsigned char ret = 0;
	read_bytes(layer, (byte*)&ret, 1);
	return (unsigned int)ret;
}
static INLINE unsigned int get_word(APEG_LAYER *layer)
{
	unsigned short ret = 0;
	read_bytes(layer, (byte*)&ret, 2);
#ifndef ALLEGRO_BIG_ENDIAN
	ret = ((ret&0xFF)<<8) | (ret>>8);
#endif
	return (unsigned int)ret;
}
static INLINE unsigned int get_long(APEG_LAYER *layer)
{
	unsigned int ret = 0;
	read_bytes(layer, (byte*)&ret, 4);
#ifndef ALLEGRO_BIG_ENDIAN
	ret = ((ret&0xFF)<<24) | ((ret&0xFF00)<<8) |
	      ((ret&0xFF0000)>>8) | (ret>>24);
#endif
	return ret;
}

static void next_packet(APEG_LAYER *layer)
{
	unsigned int code;

next_code:
	code = get_long(layer);

	/* remove system layer byte stuffing */
	while((code & 0xFFFFFF00) != 0x100)
		code = (code<<8) | get_byte(layer);

	switch(code)
	{
		case PACK_START_CODE: /* pack header */
			/* skip pack header (system_clock_reference and mux_rate) */
			almpa_skip_bytes(layer, 8);
			goto next_code;

		case AUDIO_ELEMENTARY_STREAM:
			code = get_word(layer); // packet_length
			layer->audio.rd.left_in_packet = code;

			code = get_byte(layer);
/*			if((code>>6)==0x02)
			{
				almpa_skip_bytes(layer, 1);
				code = get_byte(layer);
				almpa_skip_bytes(layer, code);
				break;
			}*/

			// parse MPEG-1 packet header
			while(code == 0xFF)
				code = get_byte(layer);

			// stuffing bytes
			if(code>=0x40)
			{
				if(code>=0x80)
					apeg_error_jump("Error in packet header");

				// skip STD_buffer_scale
				almpa_skip_bytes(layer, 1);
				code = get_byte(layer);
				if(code>=0x40)
					apeg_error_jump("Error in packet header");
			}

			// skip presentation and decoding time stamps
			if(code>=0x30)
				almpa_skip_bytes(layer, 9);
			// skip presentation time stamps
			else if(code>=0x20)
				almpa_skip_bytes(layer, 4);
			else if(code!=0x0f)
				apeg_error_jump("Error in packet header");

			break;

		case ISO_END_CODE: /* end */
			/* simulate a buffer of end codes */
			layer->audio.rd.left_in_packet = ~0u;
			layer->audio.rd.eof = TRUE;
			break;

		default:
			if(code>=SYSTEM_START_CODE)
			{
				/* skip system headers and non-video/audio packets */
				code = get_word(layer);
				almpa_skip_bytes(layer, code);
			}
			else
			{
				snprintf(apeg_error, sizeof(apeg_error),
				         "Unknown startcode 0x%08x in system layer (audio)", code);
				apeg_error_jump(NULL);
			}

			goto next_code;
	}
}


/**** APEG datastream readers ****/
struct memdat {
	unsigned char *buf;
	unsigned int bytes_left;
};
static int init_reader(APEG_LAYER *layer)
{
	struct reader *rds = &(layer->audio.rd);
	switch(layer->buffer_type)
	{
		case DISK_BUFFER:
			if(rds->filept)
				pack_fclose(rds->filept);
			rds->filept = pack_fopen(layer->fname, F_READ);
			if(!rds->filept)
				return ALMPA_ERROR;
			break;

		case MEMORY_BUFFER:
			rds->filept = layer->mem_data.buf - (layer->buffer_size -
			                                     layer->mem_data.bytes_left);
			break;

		default:
			return ALMPA_ERROR;
	}

	rds->eof = FALSE;
    rds->filepos = 0;
	rds->left_in_packet = ~0u;
	if(layer->system_stream_flag == MPEG_SYSTEM)
		next_packet(layer);

    return ALMPA_OK;
}

void almpa_close(APEG_LAYER *layer)
{
	struct reader *rds = &(layer->audio.rd);

	if(layer->buffer_type == DISK_BUFFER && rds->filept)
		pack_fclose((PACKFILE*)rds->filept);
	rds->filept = 0;
}


int almpa_read_bytes(APEG_LAYER *layer, byte *buf, unsigned int count)
{
	struct reader *rds = &(layer->audio.rd);
    int cnt = 0;

	/* Make sure there's enough data in this packet to read */
	while(count > rds->left_in_packet)
	{
		if(rds->left_in_packet)
		{
			int s = rds->left_in_packet;
			read_bytes(layer, buf+cnt, s);
			cnt += s;
			count -= s;
		}
		next_packet(layer);
	}
	read_bytes(layer, buf+cnt, count);
	cnt += count;

	return cnt;
}

void almpa_head_read(APEG_LAYER *layer, unsigned long *newhead)
{
	unsigned char hbuf[4];

	almpa_read_bytes(layer, hbuf, 4);
	*newhead = ((unsigned long) hbuf[0] << 24) |
	           ((unsigned long) hbuf[1] << 16) |
	           ((unsigned long) hbuf[2] << 8)  |
	            (unsigned long) hbuf[3];
}

void almpa_head_shift(APEG_LAYER *layer, unsigned long *head)
{
    unsigned char hbuf;

	almpa_read_bytes(layer, &hbuf, 1);
    *head <<= 8;
    *head |= hbuf;
    *head &= 0xffffffff;
}

void almpa_skip_bytes(APEG_LAYER *layer, int count)
{
	struct reader *rds = &(layer->audio.rd);

	if(layer->buffer_type == DISK_BUFFER)
		pack_fseek((PACKFILE*)rds->filept, count);
	else if(layer->buffer_type == MEMORY_BUFFER)
	{
		int i = (unsigned)layer->buffer_size - (unsigned)rds->filepos;
		if(i < count)
			count = i;
		((unsigned char*)rds->filept) += count;
	}

	rds->filepos += count;
	rds->left_in_packet -= count;
}


/* open the device to read the bit stream from it */
int open_stream(APEG_LAYER *layer)
{
	almpa_close(layer);
	return init_reader(layer);
}

#endif
