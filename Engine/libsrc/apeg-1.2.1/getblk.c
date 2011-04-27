/* getblk.c, DCT block decoding                                             */

#include <stdio.h>
#include <string.h>

#include "mpeg1dec.h"
#include "getblk.h"


const unsigned char apeg_scan[64] = {
	// Zig-Zag scan pattern
	0,1,8,16,9,2,3,10,17,24,32,25,18,11,4,5,
	12,19,26,33,40,48,41,34,27,20,13,6,7,14,21,28,
	35,42,49,56,57,50,43,36,29,22,15,23,30,37,44,51,
	58,59,52,45,38,31,39,46,53,60,61,54,47,55,62,63
};


/* parse VLC and perform dct_diff arithmetic.

   Note: the arithmetic here is presented more elegantly than
   the spec, yet the results, dct_diff, are the same.
*/
static int Get_Luma_DC_dct_diff(APEG_LAYER *layer)
{
	int code, size;

	/* decode length */
	code = show_bits(layer, 5);

	if (code<31)
	{
		/* Table B-12, dct_dc_size_luminance, codes 00xxx ... 11110 */
		static const VLCtab DClumtab0[32] = {
			{1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2},
			{2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2},
			{0, 3}, {0, 3}, {0, 3}, {0, 3}, {3, 3}, {3, 3}, {3, 3}, {3, 3},
			{4, 3}, {4, 3}, {4, 3}, {4, 3}, {5, 4}, {5, 4}, {6, 5}, {0, 0}
		};

		size = DClumtab0[code].val;
		apeg_flush_bits8(layer, DClumtab0[code].len);

		if(size == 0)
			return 0;
	}
	else
	{
		/* Table B-12, dct_dc_size_luminance, codes 111110xxx ... 111111111 */
		static const VLCtab DClumtab1[16] = {
			{7, 6}, {7, 6}, {7, 6}, {7, 6}, {7, 6}, {7, 6}, {7, 6}, { 7,6},
			{8, 7}, {8, 7}, {8, 7}, {8, 7}, {9, 8}, {9, 8}, {10,9}, {11,9}
		};

		code = show_bits(layer, 9) - 496;
		size = DClumtab1[code].val;
		apeg_flush_bits(layer, DClumtab1[code].len);
	}

	{
		const int dct_diff = apeg_get_bits(layer, size);
		const int f = ((dct_diff >> (size-1))&1)^1;
		return dct_diff - ((f<<size) - f);
/*		int dct_diff = apeg_get_bits(layer, size);
		if (( dct_diff & (1 << (size-1)) ) == 0)
			return dct_diff - ((1<<size) - 1);
		return dct_diff;*/
	}
}


static int Get_Chroma_DC_dct_diff(APEG_LAYER *layer)
{
	int code, size;

	/* decode length */
	code = show_bits(layer, 5);

	if (code<31)
	{
		/* Table B-13, dct_dc_size_chrominance, codes 00xxx ... 11110 */
		static const VLCtab DCchromtab0[32] = {
			{0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2},
			{1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2},
			{2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2},
			{3, 3}, {3, 3}, {3, 3}, {3, 3}, {4, 4}, {4, 4}, {5, 5}, {0, 0}
		};

		size = DCchromtab0[code].val;
		apeg_flush_bits8(layer, DCchromtab0[code].len);

		if(size == 0)
			return 0;
	}
	else
	{
		/* Table B-13, dct_dc_size_chrominance, codes 111110xxxx ... 1111111111 */
		static const VLCtab DCchromtab1[32] = {
			{6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, { 6, 6}, { 6, 6},
			{6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, { 6, 6}, { 6, 6},
			{7, 7}, {7, 7}, {7, 7}, {7, 7}, {7, 7}, {7, 7}, { 7, 7}, { 7, 7},
			{8, 8}, {8, 8}, {8, 8}, {8, 8}, {9, 9}, {9, 9}, {10,10}, {11,10}
		};

		code = show_bits(layer, 10) - 992;
		size = DCchromtab1[code].val;
		apeg_flush_bits(layer, DCchromtab1[code].len);
	}

	{
		const int dct_diff = apeg_get_bits(layer, size);
		const int f = ((dct_diff >> (size-1))&1)^1;
		return dct_diff - ((f<<size) - f);
/*		int dct_diff = apeg_get_bits(layer, size);
		if (( dct_diff & (1 << (size-1)) ) == 0)
			return dct_diff - ((1<<size) - 1);
		return dct_diff;*/
	}
}

/* set scratch pad macroblock to zero */
#define Clear_Block(p)	memset((p), 0, sizeof(apeg_block[0]))
#define Clear_Blocks()	memset(apeg_block, 0, sizeof(apeg_block))


/* decode all six intra coded MPEG-1 blocks */
void apeg_decode_intra_blocks(APEG_LAYER *layer, int dc_dct_pred[3])
{
  int val, i, j, sign, comp;
  unsigned int code;
  const DCTtab *tab;
  short *bp;

  Clear_Blocks();

  for(comp = 0;comp != 6;++comp)
  {
    /* decode DC coefficients */
    switch(comp)
    {
      case 5:
        bp = apeg_block[5];
        bp[0] = (dc_dct_pred[2]+=Get_Chroma_DC_dct_diff(layer)) << 3;
        break;
      case 4:
        bp = apeg_block[4];
        bp[0] = (dc_dct_pred[1]+=Get_Chroma_DC_dct_diff(layer)) << 3;
        break;
      default:
        bp = apeg_block[comp];
        bp[0] = (dc_dct_pred[0]+=Get_Luma_DC_dct_diff(layer)) << 3;
    }

    /* decode AC coefficients */
    for (i=1; ; i++)
    {
      code = show_bits(layer, 16);

      if (code >= 16384)
        tab = &DCTtabnext[(code>>12)-4];
      else if (code >= 1024)
        tab = &DCTtab0[(code>>8)-4];
      else if (code >= 512)
        tab = &DCTtab1[(code>>6)-8];
      else if (code >= 256)
        tab = &DCTtab2[(code>>4)-16];
      else if (code >= 128)
        tab = &DCTtab3[(code>>3)-16];
      else if (code >= 64)
        tab = &DCTtab4[(code>>2)-16];
      else if (code >= 32)
        tab = &DCTtab5[(code>>1)-16];
      else if (code >= 16)
        tab = &DCTtab6[code-16];
      else
        break;

      switch(tab->run)
      {
        case 64:
          apeg_flush_bits(layer, tab->len);
		  i = 64;
		  break;

        case 65:
          i += apeg_get_bits(layer, tab->len + 6) & 0x3F;

          val = show_bits(layer, 8);
          switch(val)
          {
            case 0:
              val = apeg_get_bits(layer, 16) & 0xFF;
              sign = 0;
              break;

            case 128:
              val = 256 - (apeg_get_bits(layer, 16) & 0xFF);
              sign = 1;
              break;

            default:
              apeg_flush_bits8(layer, 8);
			  sign = val & 0x80;
			  if(sign)
                val = 256 - val;
          }
          break;

        default:
          i += tab->run;
          val = tab->level;
          sign = apeg_get_bits(layer, tab->len + 1) & 0x1;
      }

      if(i >= 64)
        break;

      j = apeg_scan[i];
      val = (val*layer->quantizer_scale*layer->intra_quantizer_matrix[j]) >> 3;

      /* mismatch control ('oddification') */
      if (val!=0) /* should always be true, but it's not guaranteed */
      {
        val = (val-1) | 1; /* equivalent to: if ((val&1)==0) val = val - 1; */
        /* saturation */
        if (!sign)
		{
			val -= 2047;
			val &= val >> 31;
			bp[j] = val + 2047;
		}
        else
		{
			val -= 2048;
			val &= val >> 31;
			bp[j] = -(val + 2048);
		}
      }
    }
  }
}


/* decode non-intra coded MPEG-1 blocks */
void apeg_decode_non_intra_block(APEG_LAYER *layer, int comp)
{
  int val, i, j, sign;
  unsigned int code;
  const DCTtab *tab;
  short *bp;

  bp = apeg_block[comp];
  Clear_Block(bp);

  /* decode AC coefficients */
  for (i=0; ; i++)
  {
    code = show_bits(layer, 16);

    if (code >= 16384)
    {
      if (i==0)
        tab = &DCTtabfirst[(code>>12)-4];
      else
        tab = &DCTtabnext[(code>>12)-4];
    }
    else if (code >= 1024)
      tab = &DCTtab0[(code>>8)-4];
    else if (code >= 512)
      tab = &DCTtab1[(code>>6)-8];
    else if (code >= 256)
      tab = &DCTtab2[(code>>4)-16];
    else if (code >= 128)
      tab = &DCTtab3[(code>>3)-16];
    else if (code >= 64)
      tab = &DCTtab4[(code>>2)-16];
    else if (code >= 32)
      tab = &DCTtab5[(code>>1)-16];
    else if (code >= 16)
      tab = &DCTtab6[code-16];
    else
      return;

    switch(tab->run)
    {
      case 64:
        apeg_flush_bits(layer, tab->len);
	    i = 64;
	    break;

      case 65:
        i += apeg_get_bits(layer, tab->len + 6) & 0x3F;

        val = show_bits(layer, 8);
        switch(val)
        {
          case 0:
            val = apeg_get_bits(layer, 16) & 0xFF;
            sign = 0;
            break;

          case 128:
            val = 256 - (apeg_get_bits(layer, 16) & 0xFF);
            sign = 1;
            break;

          default:
            apeg_flush_bits8(layer, 8);
			sign = val & 0x80;
	        if(sign)
              val = 256 - val;
        }
        break;

      default:
        i += tab->run;
        val = tab->level;
        sign = apeg_get_bits(layer, tab->len + 1) & 0x1;
    }

    if(i >= 64)
      return;

    j = apeg_scan[i];
    val = (((val<<1)|1)*layer->quantizer_scale*layer->non_intra_quantizer_matrix[j]) >> 4;

    /* mismatch control ('oddification') */
    if (val!=0) /* should always be true, but it's not guaranteed */
    {
      val = (val-1) | 1; /* equivalent to: if ((val&1)==0) val = val - 1; */

      /* saturation */
      if (!sign)
	  {
		val -= 2047;
		val &= val >> 31;
		bp[j] = val + 2047;
	  }
      else
	  {
		val -= 2048;
		val &= val >> 31;
		bp[j] = -(val + 2048);
	  }
    }
  }
}
