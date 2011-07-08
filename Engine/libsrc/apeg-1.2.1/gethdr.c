/* gethdr.c, header decoding */

#include <stdio.h>
#include <string.h>

#include <allegro.h>
#include "apeg.h"

#include "mpeg1dec.h"


/* private prototypes */
static void sequence_header(APEG_LAYER *layer);
static void group_of_pictures_header(APEG_LAYER *layer);
static void picture_header(APEG_LAYER *layer);

static int fps_Table[16][2] = {
	{ 0, 1 },
	{ 24*1000, 1001 }, { 24, 1 },
	{ 25, 1 },
	{ 30*1000, 1001 }, { 30, 1 },
	{ 50, 1 },
	{ 60*1000, 1001 }, { 60, 1 },
	{ -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 },
	{ -1, -1 }, { -1, -1 }, { -1, -1 }
};

static int aspect_Table[16][2] = {
	{     0,     1 },	/* unknown     */
	{     1,     1 },	/* Square-pel  */
	{ 10000,  6735 },	/* 1.0/0.6735  */
	{ 10000,  7031 },	/* 1.0/0.7031  */
	{ 10000,  7615 },	/* 1.0/0.7615  */
	{ 10000,  8055 },	/* 1.0/0.8055  */
	{ 10000,  8437 },	/* 1.0/0.8437  */
	{ 10000,  8935 },	/* 1.0/0.8935  */
	{    59,    54 },	/* PAL source  */
	{ 10000,  9815 },	/* 1.0/0.9815  */
	{ 10000, 10255 },	/* 1.0/1.0255  */
	{ 10000, 10695 },	/* 1.0/1.0695  */
	{    10,    11 },	/* NTSC source */
	{ 10000, 11575 },	/* 1.0/1.1575  */
	{ 10000, 12015 },	/* 1.0/1.2015  */
	{     0,     1 }	/* unknown     */
};

/*
 * decode headers from one input stream
 * until an End of Sequence or picture start code
 * is found
 */
int apeg_get_header(APEG_LAYER *layer)
{
	unsigned int code;

	for (;;)
	{
		// look for next_start_code
		code = apeg_start_code(layer);
		apeg_flush_bits32(layer);
  
		switch (code)
		{
			case PICTURE_START_CODE:
				picture_header(layer);
				return 1;

			case GROUP_START_CODE:
				group_of_pictures_header(layer);
				break;

			case SEQUENCE_HEADER_CODE:
				sequence_header(layer);
				break;

			case SEQUENCE_END_CODE:
				return 0;

			default:
				if(pack_feof(layer->pf))
					return 0;
                TRACE("Unknown start code (0x%08x)\n", code);
		}
	}
}

/* decode sequence header */
static void sequence_header(APEG_LAYER *layer)
{
	int i;
	int vbv_buffer_size;
	int bit_rate_value;
	int aspect_code;
	int fps_code;
	int constrained_parameters_flag;

	if(!layer->stream.w || !layer->stream.h)
	{
		/* A series of (masked) show_bits followed by one
		 * apeg_flush_bits is quicker than a series of apeg_get_bits.
		 * Note, however, that normally only the first 24 bits are
		 * guaranteed to be correct, but here it's known that we're on
		 * the byte boundry. */
		layer->stream.w = show_bits(layer, 12);
		layer->stream.h = show_bits(layer, 24) & MASK_BITS(12);

		aspect_code = show_bits(layer, 28) & MASK_BITS(4);
		fps_code    = show_bits32(layer)   & MASK_BITS(4);
		apeg_flush_bits32(layer);

		bit_rate_value              = show_bits(layer, 18);
//		marker_bit(layer, "sequence_header()");
		vbv_buffer_size             = show_bits(layer, 29) & MASK_BITS(6);
		constrained_parameters_flag = show_bits(layer, 30) & MASK_BITS(1);
		apeg_flush_bits(layer, 30);

		layer->stream.aspect_numerator   = aspect_Table[aspect_code][0];
		layer->stream.aspect_denominator = aspect_Table[aspect_code][1];
		layer->stream.aspect_ratio = (double)layer->stream.w /
		                             (double)layer->stream.h *
		                             ((double)aspect_Table[aspect_code][0] /
		                              (double)aspect_Table[aspect_code][1]);

		layer->stream.fps_numerator   = fps_Table[fps_code][0];
		layer->stream.fps_denominator = fps_Table[fps_code][1];
		layer->stream.frame_rate = (double)fps_Table[fps_code][0] /
		                           (double)fps_Table[fps_code][1];

		layer->stream.bit_rate = bit_rate_value * 400;
	}
	else
	{
		apeg_flush_bits32(layer);
		apeg_flush_bits(layer, 30);
	}

	if(apeg_get_bits1(layer))
	{
		i = 0;
		do {
			layer->intra_quantizer_matrix[apeg_scan[i]] = apeg_get_bits8(layer,
			                                                             8);
		} while(++i != 64);
	}
	else
	{
		static const int default_intra_quant_matrix[64] = {
			 8, 16, 19, 22, 26, 27, 29, 34, 16, 16, 22, 24, 27, 29, 34, 37,
			19, 22, 26, 27, 29, 34, 34, 38, 22, 22, 26, 27, 29, 34, 37, 40,
			22, 26, 27, 29, 32, 35, 40, 48, 26, 27, 29, 32, 35, 40, 48, 58,
			26, 27, 29, 34, 38, 46, 56, 69, 27, 29, 35, 38, 46, 56, 69, 83
		};
		memcpy(layer->intra_quantizer_matrix, default_intra_quant_matrix,
		       sizeof(int)*64);
	}

	i = 0;
	if(apeg_get_bits1(layer))
	{
		do {
			layer->non_intra_quantizer_matrix[apeg_scan[i]] = apeg_get_bits8(layer, 8);
		} while(++i != 64);
	}
	else
	{
		do {
			layer->non_intra_quantizer_matrix[i] = 16;
		} while(++i != 64);
	}
}

/* decode group of pictures header */
static void group_of_pictures_header(APEG_LAYER *layer)
{
	apeg_flush_bits(layer, 27);
}

/* decode picture header */
static void picture_header(APEG_LAYER *layer)
{
//	temporal_reference  = show_bits(layer, 10);
	layer->picture_type = show_bits(layer, 13) & MASK_BITS(3);
//	vbv_delay           = show_bits(layer, 29) & MASK_BITS(16);
	apeg_flush_bits(layer, 29);
}
