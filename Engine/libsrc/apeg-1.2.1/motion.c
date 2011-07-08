/* motion.c, motion vector decoding                                         */

#include "mpeg1dec.h"

typedef struct {
  char val, len;
} VLCtab;

/* calculate motion vector component */
/* Note: the arithmetic here is more elegant than that which is shown 
   in 7.6.3.1.  The end results (PMV[][][]) should, however, be the same.  */
static int decode_motion_vector( int vec, int r_size, int motion_code, int motion_residual, int full_pel_vector)
{
	int lim = 16<<r_size;

	// I don't think MPEG-1 supports full_pel_vectors...
	// Seems they do. At least some encoders let you use it anyway.
	if(full_pel_vector)
	{
		vec >>= 1;
		if(motion_code > 0)
		{
			vec += ((motion_code-1)<<r_size) + motion_residual + 1;
			vec -= ((lim<<1) & ~((vec-lim)>>31));
//			if(vec >= lim)
//				vec -= lim << 1;
		}
		else
		{
			vec -= ((-motion_code-1)<<r_size) + motion_residual + 1;
			vec += ((lim<<1) & ~((-1-lim-vec)>>31));
//			if(-lim > vec)
//				vec += lim << 1;
		}

		vec <<= 1;
	}
	else
	{
		if(motion_code > 0)
		{
			vec += ((motion_code-1)<<r_size) + motion_residual + 1;
			vec -= ((lim<<1) & ~((vec-lim)>>31));
//			if(vec >= lim)
//				vec -= lim << 1;
		}
		else
		{
			vec -= ((-motion_code-1)<<r_size) + motion_residual + 1;
			vec += ((lim<<1) & ~((-1-lim-vec)>>31));
//			if(-lim > vec)
//				vec += lim << 1;
		}
	}

	return vec;
}

// Helper to set a positive number to negative given a sign-bit
#define BIT_TO_SIGN(n,b)	((n) * (1 - ((b)<<1)))

static int get_motion_code(APEG_LAYER *layer)
{
	int code;

	if(apeg_get_bits1(layer))
		return 0;

	if((code = show_bits(layer, 9)) >= 64)
	{
		static const VLCtab MVtab0[8] = {
			{0,1}, {3,4}, {2,3}, {2,3}, {1,2}, {1,2}, {1,2}, {1,2}
		};

		code >>= 6;
		return BIT_TO_SIGN(MVtab0[code].val,
		                   apeg_get_bits8(layer, MVtab0[code].len) & 0x1);
//		return (apeg_get_bits8(layer, MVtab0[code].len) & 0x1) ?
//		       (-MVtab0[code].val) : (MVtab0[code].val);
	}
	else
	{
		static const VLCtab MVtab1[64] = {
			{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},
			{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, {16,10}, {15,10}, {14,10}, {13,10},
			{12,10}, {11,10}, {10, 9}, {10, 9}, { 9, 9}, { 9, 9}, { 8, 9}, { 8, 9},
			{ 7, 7}, { 7, 7}, { 7, 7}, { 7, 7}, { 7, 7}, { 7, 7}, { 7, 7}, { 7, 7},
			{ 6, 7}, { 6, 7}, { 6, 7}, { 6, 7}, { 6, 7}, { 6, 7}, { 6, 7}, { 6, 7},
			{ 5, 7}, { 5, 7}, { 5, 7}, { 5, 7}, { 5, 7}, { 5, 7}, { 5, 7}, { 5, 7},
			{ 4, 6}, { 4, 6}, { 4, 6}, { 4, 6}, { 4, 6}, { 4, 6}, { 4, 6}, { 4, 6},
			{ 4, 6}, { 4, 6}, { 4, 6}, { 4, 6}, { 4, 6}, { 4, 6}, { 4, 6}, { 4, 6}
		};

		return BIT_TO_SIGN(MVtab1[code].val,
		                   apeg_get_bits(layer, MVtab1[code].len) & 0x1);
//		return (apeg_get_bits(layer, MVtab1[code].len) & 0x1) ?
//		       (-MVtab1[code].val) : (MVtab1[code].val);
	}
}

/* get and decode motion vector and differential motion vector 
 * for one prediction */
void apeg_motion_vector(APEG_LAYER *layer, int *PMV, int r_size, int full_pel_vector)
{
	int motion_code;

	if(r_size)
	{
		/* horizontal component */
		motion_code = get_motion_code(layer);
		if(motion_code)
			PMV[0] = decode_motion_vector(PMV[0], r_size, motion_code,
			                              apeg_get_bits8(layer, r_size), full_pel_vector);
		else if(full_pel_vector)
			PMV[0] &= ~1;

		/* vertical component */
		motion_code = get_motion_code(layer);
		if(motion_code)
			PMV[1] = decode_motion_vector(PMV[1], r_size, motion_code,
			                              apeg_get_bits8(layer, r_size), full_pel_vector);
		else if(full_pel_vector)
			PMV[1] &= ~1;
	}
	else
	{
		/* horizontal component */
		motion_code = get_motion_code(layer);
		if(motion_code)
			PMV[0] = decode_motion_vector(PMV[0], r_size, motion_code, 0, full_pel_vector);
		else if(full_pel_vector)
			PMV[0] &= ~1;

		/* vertical component */
		motion_code = get_motion_code(layer);
		if(motion_code)
			PMV[1] = decode_motion_vector(PMV[1], r_size, motion_code, 0, full_pel_vector);
		else if(full_pel_vector)
			PMV[1] &= ~1;
	}
}
