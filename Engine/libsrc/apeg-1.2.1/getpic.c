/* getpic.c, picture decoding */

#include <stdio.h>
#include <string.h>

#include <allegro.h>
#include "apeg.h"

#include "mpeg1dec.h"

/* buffers for multiuse purposes */
short apeg_block[6][64];

typedef struct {
    char val, len;
} VLCtab;

/* private prototypes*/

// Table B-3, macroblock_type in P-pictures, codes 001..1xx
static const VLCtab PMBtab0[8] = {
    {0,0},
    {MACROBLOCK_MOTION_FORWARD,3},
    {MACROBLOCK_PATTERN,2}, {MACROBLOCK_PATTERN,2},
    {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_PATTERN,1},
    {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_PATTERN,1},
    {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_PATTERN,1},
    {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_PATTERN,1}
};

// Table B-3, macroblock_type in P-pictures, codes 000001..00011x
static const VLCtab PMBtab1[8] = {
    {0,0},
    {MACROBLOCK_QUANT|MACROBLOCK_INTRA,6},
    {MACROBLOCK_QUANT|MACROBLOCK_PATTERN,5},
    {MACROBLOCK_QUANT|MACROBLOCK_PATTERN,5},
    {MACROBLOCK_QUANT|MACROBLOCK_MOTION_FORWARD|MACROBLOCK_PATTERN,5},
    {MACROBLOCK_QUANT|MACROBLOCK_MOTION_FORWARD|MACROBLOCK_PATTERN,5},
    {MACROBLOCK_INTRA,5}, {MACROBLOCK_INTRA,5}
};

// Table B-4, macroblock_type in B-pictures, codes 0010..11xx
static const VLCtab BMBtab0[16] = {
    {0,0}, {0,0},
    {MACROBLOCK_MOTION_FORWARD,4},
    {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_PATTERN,4},
    {MACROBLOCK_MOTION_BACKWARD,3},
    {MACROBLOCK_MOTION_BACKWARD,3},
    {MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,3},
    {MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,3},
    {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD,2},
    {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD,2},
    {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD,2},
    {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD,2},
    {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,2},
    {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,2},
    {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,2},
    {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,2}
};

// Table B-4, macroblock_type in B-pictures, codes 000001..00011x
static const VLCtab BMBtab1[8] = {
    {0,0},
    {MACROBLOCK_QUANT|MACROBLOCK_INTRA,6},
    {MACROBLOCK_QUANT|MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,6},
    {MACROBLOCK_QUANT|MACROBLOCK_MOTION_FORWARD|MACROBLOCK_PATTERN,6},
    {MACROBLOCK_QUANT|MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,5},
    {MACROBLOCK_QUANT|MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,5},
    {MACROBLOCK_INTRA,5}, {MACROBLOCK_INTRA,5}
};


static void Add_Block(APEG_LAYER *layer, int comp, int bx, int by);
static void Move_Blocks(APEG_LAYER *layer, int bx, int by);

static void slice_header(APEG_LAYER *layer);
static int get_mba_inc(APEG_LAYER *layer);
static int get_block_pattern(APEG_LAYER *layer);

static void i_picture(APEG_LAYER *layer);
static void p_picture(APEG_LAYER *layer);
static void b_picture(APEG_LAYER *layer);

static int full_forward_vector;
static int forward_code;
static int full_backward_vector;
static int backward_code;


/* decode one frame */
unsigned char **apeg_get_frame(APEG_LAYER *layer)
{
    unsigned char *tmp;

    switch(layer->picture_type)
    {
    case B_TYPE:
        layer->current_frame[0] = layer->auxframe[0];
        layer->current_frame[1] = layer->auxframe[1];
        layer->current_frame[2] = layer->auxframe[2];

        // Get forward and backward vector codes
        full_forward_vector  = show_bits(layer, 1);
        forward_code         = (show_bits(layer, 4) & 0x7) - 1;
        full_backward_vector = show_bits(layer, 5) & 0x1;
        backward_code        = (apeg_get_bits8(layer, 8) & 0x7) - 1;

        while(show_bits(layer, 1))
            apeg_flush_bits(layer, 9);
        apeg_flush_bits1(layer);

        b_picture(layer);

        /* display current reference frame */
        return layer->auxframe;

    case P_TYPE:
        tmp = layer->forward_frame[0];
        layer->forward_frame[0] = layer->backward_frame[0];
        layer->backward_frame[0] = tmp;
        layer->current_frame[0] = layer->backward_frame[0];

        tmp = layer->forward_frame[1];
        layer->forward_frame[1] = layer->backward_frame[1];
        layer->backward_frame[1] = tmp;
        layer->current_frame[1] = layer->backward_frame[1];

        tmp = layer->forward_frame[2];
        layer->forward_frame[2] = layer->backward_frame[2];
        layer->backward_frame[2] = tmp;
        layer->current_frame[2] = layer->backward_frame[2];

        // Get forward vector code
        full_forward_vector = show_bits(layer, 1);
        forward_code        = (apeg_get_bits8(layer, 4) & 0x7) - 1;

        while(show_bits(layer, 1))
            apeg_flush_bits(layer, 9);
        apeg_flush_bits1(layer);

        p_picture(layer);

        return layer->forward_frame;

    case I_TYPE:
        tmp = layer->forward_frame[0];
        layer->forward_frame[0] = layer->backward_frame[0];
        layer->backward_frame[0] = tmp;
        layer->current_frame[0] = layer->backward_frame[0];

        tmp = layer->forward_frame[1];
        layer->forward_frame[1] = layer->backward_frame[1];
        layer->backward_frame[1] = tmp;
        layer->current_frame[1] = layer->backward_frame[1];

        tmp = layer->forward_frame[2];
        layer->forward_frame[2] = layer->backward_frame[2];
        layer->backward_frame[2] = tmp;
        layer->current_frame[2] = layer->backward_frame[2];

        i_picture(layer);

        return layer->forward_frame;

    default:
        TRACE("Unknown Picture Type (%i)\n", layer->picture_type);
    }

    return NULL;
}


/* decode all macroblocks of the current picture */
static void i_picture(APEG_LAYER *layer)
{
    const int MBAmax = layer->mb_cols*layer->mb_rows;
    int MBA, MBAinc;
    int dc_dct_pred[3];
    int bx, by;
    unsigned int code;

slice_start:
    dc_dct_pred[0] = dc_dct_pred[1] = dc_dct_pred[2] = 0;

    code = apeg_start_code(layer);
    if(code < SLICE_START_CODE_MIN || code > SLICE_START_CODE_MAX)
        return;
    apeg_flush_bits32(layer);

    slice_header(layer);

    bx = get_mba_inc(layer);
    by = (code&255) - 1;

    MBA = by*layer->mb_cols + bx;

    bx <<= 4;
    by <<= 4;

block_start:
    switch(show_bits(layer, 2))
    {
    case 0:
        goto slice_start;

    case 1:
        layer->quantizer_scale = apeg_get_bits(layer, 7) & MASK_BITS(5);
        break;

    default:
        apeg_flush_bits1(layer);
    }

    apeg_decode_intra_blocks(layer, dc_dct_pred);

    apeg_fast_idct(apeg_block[0]);
    apeg_fast_idct(apeg_block[1]);
    apeg_fast_idct(apeg_block[2]);
    apeg_fast_idct(apeg_block[3]);
    apeg_fast_idct(apeg_block[4]);
    apeg_fast_idct(apeg_block[5]);
    Move_Blocks(layer, bx, by);

    if(++MBA >= MBAmax)
        return;

    if(show_bits(layer, 24) == 1)
        goto slice_start;

    MBAinc = get_mba_inc(layer);
    if(MBAinc == 0)
    {
        if((bx += 16) == layer->coded_width)
        {
            bx = 0;
            by += 16;
        }
        goto block_start;
    }

    dc_dct_pred[0] = dc_dct_pred[1] = dc_dct_pred[2] = 0;

    MBA += MBAinc;
    if(MBA >= MBAmax)
        return;

    bx = (MBA%layer->mb_cols) << 4;
    by = (MBA/layer->mb_cols) << 4;

    goto block_start;
}

#define do_block(b) \
    if(coded_block_pattern & (32>>(b))) \
{ \
    apeg_decode_non_intra_block(layer, (b)); \
    apeg_fast_idct(apeg_block[(b)]); \
    Add_Block(layer, (b), bx, by); \
}

static void p_picture(APEG_LAYER *layer)
{
    const int MBAmax = layer->mb_cols*layer->mb_rows;
    int macroblock_type;
    int coded_block_pattern;
    int MBA, MBAinc;
    int dc_dct_pred[3];
    int PMV[2];
    int bx, by;
    unsigned int code;

slice_start:
    dc_dct_pred[0] = dc_dct_pred[1] = dc_dct_pred[2] = 0;
    PMV[0] = PMV[1] = 0;

    code = apeg_start_code(layer);
    if(code < SLICE_START_CODE_MIN || code > SLICE_START_CODE_MAX)
        return;
    apeg_flush_bits32(layer);

    slice_header(layer);

    bx = get_mba_inc(layer);
    by = (code&255) - 1;

    MBA = by*layer->mb_cols + bx;

    bx <<= 4;
    by <<= 4;

block_start:
    code = show_bits(layer, 6);

    if(code >= 8)
    {
        code >>= 3;

        apeg_flush_bits8(layer, PMBtab0[code].len);
        macroblock_type = PMBtab0[code].val;

        dc_dct_pred[0] = dc_dct_pred[1] = dc_dct_pred[2] = 0;

        switch(macroblock_type & (MACROBLOCK_MOTION_FORWARD|MACROBLOCK_PATTERN))
        {
        case MACROBLOCK_MOTION_FORWARD|MACROBLOCK_PATTERN:
            apeg_motion_vector(layer,PMV,forward_code,full_forward_vector);
            apeg_form_f_pred(layer, bx, by, PMV);

            break;

        case MACROBLOCK_PATTERN:
            PMV[0] = PMV[1] = 0;
            apeg_empty_pred(layer->forward_frame, layer->current_frame, bx, by, layer->coded_width);

            break;

        default:
            apeg_motion_vector(layer,PMV,forward_code,full_forward_vector);
            apeg_form_f_pred(layer, bx, by, PMV);

            goto next;
        }
    }
    else
    {
        apeg_flush_bits8(layer, PMBtab1[code].len);
        macroblock_type = PMBtab1[code].val;

        switch(macroblock_type & (MACROBLOCK_QUANT|MACROBLOCK_INTRA))
        {
        case MACROBLOCK_QUANT|MACROBLOCK_INTRA:
            layer->quantizer_scale = apeg_get_bits8(layer, 5);
            // fall-through...
        case MACROBLOCK_INTRA:
            PMV[0] = PMV[1] = 0;

            apeg_decode_intra_blocks(layer, dc_dct_pred);

            apeg_fast_idct(apeg_block[0]);
            apeg_fast_idct(apeg_block[1]);
            apeg_fast_idct(apeg_block[2]);
            apeg_fast_idct(apeg_block[3]);
            apeg_fast_idct(apeg_block[4]);
            apeg_fast_idct(apeg_block[5]);

            Move_Blocks(layer, bx, by);

            goto next;

        case 0:	// Table 1 must have Intra and/or Quant flags set
            goto slice_start;
        }

        layer->quantizer_scale = apeg_get_bits8(layer, 5);
        dc_dct_pred[0] = dc_dct_pred[1] = dc_dct_pred[2] = 0;

        if(macroblock_type & MACROBLOCK_MOTION_FORWARD)
        {
            apeg_motion_vector(layer,PMV,forward_code,full_forward_vector);
            apeg_form_f_pred(layer, bx, by, PMV);
        }
        else
        {
            PMV[0] = PMV[1] = 0;
            apeg_empty_pred(layer->forward_frame, layer->current_frame, bx, by, layer->coded_width);
        }
    }

    coded_block_pattern = get_block_pattern(layer);

    do_block(0);
    do_block(1);
    do_block(2);
    do_block(3);
    do_block(4);
    do_block(5);

next:
    if(++MBA >= MBAmax)
        return;

    if(show_bits(layer, 24) == 1)
        goto slice_start;

    MBAinc = get_mba_inc(layer);
    if(MBAinc != 0)
    {
        int i = MBAinc;

        dc_dct_pred[0] = dc_dct_pred[1] = dc_dct_pred[2] = 0;
        PMV[0] = PMV[1] = 0;

        do {
            if((bx += 16) == layer->coded_width)
            {
                bx = 0;
                by += 16;
            }
            apeg_empty_pred(layer->forward_frame, layer->current_frame, bx, by, layer->coded_width);
        } while(--i);

        MBA += MBAinc;
        if(MBA >= MBAmax)
            return;
    }

    if((bx += 16) == layer->coded_width)
    {
        bx = 0;
        by += 16;
    }

    goto block_start;
}

static void b_picture(APEG_LAYER *layer)
{
    const int MBAmax = layer->mb_cols*layer->mb_rows;
    int macroblock_type;
    int coded_block_pattern;
    int MBA, MBAinc;
    int dc_dct_pred[3];
    int PMV[2][2];
    int bx, by;
    unsigned int code;

slice_start:
    dc_dct_pred[0] = dc_dct_pred[1] = dc_dct_pred[2] = 0;
    PMV[0][0] = PMV[0][1] = PMV[1][0] = PMV[1][1] = 0;

    code = apeg_start_code(layer);
    if(code < SLICE_START_CODE_MIN || code > SLICE_START_CODE_MAX)
        return;
    apeg_flush_bits32(layer);

    slice_header(layer);

    bx = get_mba_inc(layer);
    by = (code&255) - 1;

    MBA = by*layer->mb_cols + bx;

    bx <<= 4;
    by <<= 4;

block_start:
    code = show_bits(layer, 6);

    if(code >= 8)
    {
        code >>= 2;

        apeg_flush_bits8(layer, BMBtab0[code].len);
        macroblock_type = BMBtab0[code].val;

        dc_dct_pred[0] = dc_dct_pred[1] = dc_dct_pred[2] = 0;

        switch(macroblock_type & (MACROBLOCK_MOTION_FORWARD|
            MACROBLOCK_MOTION_BACKWARD))
        {
        case MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD:
            apeg_motion_vector(layer,PMV[0],forward_code,full_forward_vector);
            apeg_motion_vector(layer,PMV[1],backward_code,full_backward_vector);
            apeg_form_fb_pred(layer,bx,by,(int*)PMV);
            break;

        case MACROBLOCK_MOTION_FORWARD:
            apeg_motion_vector(layer,PMV[0],forward_code,full_forward_vector);
            apeg_form_f_pred(layer,bx,by,PMV[0]);
            break;

        case MACROBLOCK_MOTION_BACKWARD:
            apeg_motion_vector(layer,PMV[1],backward_code,full_backward_vector);
            apeg_form_b_pred(layer,bx,by,PMV[1]);
            break;
        }

        if(!(macroblock_type & MACROBLOCK_PATTERN))
            goto next;
    }
    else
    {
        apeg_flush_bits8(layer, BMBtab1[code].len);
        macroblock_type = BMBtab1[code].val;

        switch(macroblock_type & (MACROBLOCK_QUANT|MACROBLOCK_INTRA))
        {
        case MACROBLOCK_QUANT|MACROBLOCK_INTRA:
            layer->quantizer_scale = apeg_get_bits8(layer, 5);
            // fall-through...
        case MACROBLOCK_INTRA:
            PMV[0][0] = PMV[0][1] = PMV[1][0] = PMV[1][1] = 0;

            apeg_decode_intra_blocks(layer, dc_dct_pred);

            apeg_fast_idct(apeg_block[0]);
            apeg_fast_idct(apeg_block[1]);
            apeg_fast_idct(apeg_block[2]);
            apeg_fast_idct(apeg_block[3]);
            apeg_fast_idct(apeg_block[4]);
            apeg_fast_idct(apeg_block[5]);
            Move_Blocks(layer, bx, by);

            goto next;

        case 0:	// Table 1 must have Intra and/or Quant flags set
            goto slice_start;
        }

        layer->quantizer_scale = apeg_get_bits8(layer, 5);
        dc_dct_pred[0] = dc_dct_pred[1] = dc_dct_pred[2] = 0;

        switch(macroblock_type & (MACROBLOCK_MOTION_FORWARD|
            MACROBLOCK_MOTION_BACKWARD))
        {
        case MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD:
            apeg_motion_vector(layer,PMV[0],forward_code,full_forward_vector);
            apeg_motion_vector(layer,PMV[1],backward_code,full_backward_vector);
            apeg_form_fb_pred(layer,bx,by,(int*)PMV);
            break;

        case MACROBLOCK_MOTION_FORWARD:
            apeg_motion_vector(layer,PMV[0],forward_code,full_forward_vector);
            apeg_form_f_pred(layer,bx,by,PMV[0]);
            break;

        case MACROBLOCK_MOTION_BACKWARD:
            apeg_motion_vector(layer,PMV[1],backward_code,full_backward_vector);
            apeg_form_b_pred(layer,bx,by,PMV[1]);
            break;
        }
    }

    coded_block_pattern = get_block_pattern(layer);

    do_block(0);
    do_block(1);
    do_block(2);
    do_block(3);
    do_block(4);
    do_block(5);

next:
    if(++MBA >= MBAmax)
        return;

    if(show_bits(layer, 24) == 1)
        goto slice_start;

    MBAinc = get_mba_inc(layer);
    if(MBAinc != 0)
    {
        int i = MBAinc;

        dc_dct_pred[0] = dc_dct_pred[1] = dc_dct_pred[2] = 0;

        switch(macroblock_type & (MACROBLOCK_MOTION_FORWARD|
            MACROBLOCK_MOTION_BACKWARD))
        {
        case MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD:
            do {
                if((bx += 16) == layer->coded_width)
                {
                    bx = 0;
                    by += 16;
                }
                apeg_form_fb_pred(layer,bx,by,(int*)PMV);
            } while(--i);
            break;

        case MACROBLOCK_MOTION_FORWARD:
            do {
                if((bx += 16) == layer->coded_width)
                {
                    bx = 0;
                    by += 16;
                }
                apeg_form_f_pred(layer,bx,by,PMV[0]);
            } while(--i);
            break;

        case MACROBLOCK_MOTION_BACKWARD:
            do {
                if((bx += 16) == layer->coded_width)
                {
                    bx = 0;
                    by += 16;
                }
                apeg_form_b_pred(layer,bx,by,PMV[1]);
            } while(--i);
            break;
        }

        MBA += MBAinc;
        if(MBA >= MBAmax)
            return;
    }

    if((bx += 16) == layer->coded_width)
    {
        bx = 0;
        by += 16;
    }

    goto block_start;
}
#undef do_block


/* move/add 8x8-Block from block[comp] to apeg_backward_frame */
/* copy reconstructed 8x8 block from block[comp] to current_frame[] */
static void Add_Block(APEG_LAYER *layer, int comp, int bx, int by)
{
    int iincr;
    unsigned char *rfp;
    const short *bp;
    int i, j;

    /* derive color component index */
    switch(comp)
    {
    case 5:
        rfp = layer->current_frame[2] +
            layer->chroma_width*(by>>1) + (bx>>1);
        iincr = layer->chroma_width - 8;
        bp = apeg_block[5];
        break;

    case 4:
        rfp = layer->current_frame[1] +
            layer->chroma_width*(by>>1) + (bx>>1);
        iincr = layer->chroma_width - 8;
        bp = apeg_block[4];
        break;

    case 3:
        rfp = layer->current_frame[0] +
            layer->coded_width*(by+8) + (bx+8);
        iincr = layer->coded_width - 8;
        bp = apeg_block[3];
        break;

    case 2:
        rfp = layer->current_frame[0] +
            layer->coded_width*(by+8) + bx;
        iincr = layer->coded_width - 8;
        bp = apeg_block[2];
        break;

    case 1:
        rfp = layer->current_frame[0] +
            layer->coded_width*by + (bx+8);
        iincr = layer->coded_width - 8;
        bp = apeg_block[1];
        break;

    default:
        rfp = layer->current_frame[0] +
            layer->coded_width*by + bx;
        iincr = layer->coded_width - 8;
        bp = apeg_block[0];
    }

    for(i = 0;i < 8;++i)
    {
        for(j = 0;j < 8;++j)
        {
            *rfp = clamp_val(0, *(bp++) + *rfp, 255);
            ++rfp;
        }
        rfp += iincr;
    }
}
#undef add_block

#define move_block() { \
    for(i = 0;i < 8;++i) \
{ \
    for(j = 0;j < 8;++j) \
    *(rfp++) = clamp_val(0, *(bp++) + 128, 255); \
    rfp += iincr; \
} \
}
static void Move_Blocks(APEG_LAYER *layer, int bx, int by)
{
    int iincr;
    unsigned char *rfp;
    const short *bp = (short*)apeg_block;
    int i, j;

    // luminance
    iincr = layer->coded_width - 8;

    rfp = layer->current_frame[0] + layer->coded_width*by + bx;
    move_block();

    rfp = layer->current_frame[0] + layer->coded_width*by + (bx+8);
    move_block();

    rfp = layer->current_frame[0] + layer->coded_width*(by+8) + bx;
    move_block();

    rfp = layer->current_frame[0] + layer->coded_width*(by+8) + (bx+8);
    move_block();

    // chrominance
    iincr = layer->chroma_width - 8;
    bx >>= 1; by >>= 1;

    rfp = layer->current_frame[1] + layer->chroma_width*by + bx;
    move_block();

    rfp = layer->current_frame[2] + layer->chroma_width*by + bx;
    move_block();
}
#undef move_block


/* decode slice header */
static void slice_header(APEG_LAYER *layer)
{
    layer->quantizer_scale = show_bits(layer, 5);

    /* slice_id introduced in March 1995 as part of the video corridendum
    (after the IS was drafted in November 1994) */
    if(show_bits(layer, 6) & 1)
    {
        apeg_flush_bits(layer, 14);

        while(show_bits(layer, 1))
            apeg_flush_bits(layer, 9);
        apeg_flush_bits1(layer);
    }
    else
        apeg_flush_bits8(layer, 6);
}

static int get_mba_inc(APEG_LAYER *layer)
{
    int code = show_bits(layer, 11);
    int val = 0;

    while(code < 24)
    {
        switch(code)
        {
        case 15: // macroblock_stuffing
            break;
        case 8:  // macroblock_escape
            val += 33;
            break;
        default: // error
            return 0;
        }

        apeg_flush_bits(layer, 11);
        code = show_bits(layer, 11);
    }

    if(code < 128)
    {
        static const VLCtab MBAtab2[104] = {
            {32,11}, {31,11}, {30,11}, {29,11}, {28,11}, {27,11}, {26,11}, {25,11},
            {24,11}, {23,11}, {22,11}, {21,11}, {20,10}, {20,10}, {19,10}, {19,10},
            {18,10}, {18,10}, {17,10}, {17,10}, {16,10}, {18,10}, {15,10}, {15,10},
            {14,8},  {14,8},  {14,8},  {14,8},  {14,8},  {14,8},  {14,8},  {14,8},
            {13,8},  {13,8},  {13,8},  {13,8},  {13,8},  {13,8},  {13,8},  {13,8},
            {12,8},  {12,8},  {12,8},  {12,8},  {12,8},  {12,8},  {12,8},  {12,8},
            {11,8},  {11,8},  {11,8},  {11,8},  {11,8},  {11,8},  {11,8},  {11,8},
            {10,8},  {10,8},  {10,8},  {10,8},  {10,8},  {10,8},  {10,8},  {10,8},
            { 9,8},  { 9,8},  { 9,8},  { 9,8},  { 9,8},  { 9,8},  { 9,8},  { 9,8},
            { 8,7},  { 8,7},  { 8,7},  { 8,7},  { 8,7},  { 8,7},  { 8,7},  { 8,7},
            { 8,7},  { 8,7},  { 8,7},  { 8,7},  { 8,7},  { 8,7},  { 8,7},  { 8,7},
            { 7,7},  { 7,7},  { 7,7},  { 7,7},  { 7,7},  { 7,7},  { 7,7},  { 7,7},
            { 7,7},  { 7,7},  { 7,7},  { 7,7},  { 7,7},  { 7,7},  { 7,7},  { 7,7}
        };

        // remove common base
        code -= 24;

        apeg_flush_bits(layer, MBAtab2[code].len);
        val += MBAtab2[code].val;
    }
    else if(code < 1024)
    {
        static const VLCtab MBAtab1[16] = {
            {0,0}, {0,0}, {6,5}, {5,5}, {4,4}, {4,4}, {3,4}, {3,4},
            {2,3}, {2,3}, {2,3}, {2,3}, {1,3}, {1,3}, {1,3}, {1,3}
        };

        // remove leading zeros
        code >>= 6;

        apeg_flush_bits8(layer, MBAtab1[code].len);
        val += MBAtab1[code].val;
    }
    else
        apeg_flush_bits1(layer);

    return val;
}

static int get_block_pattern(APEG_LAYER *layer)
{
    int code = show_bits(layer, 9);

    if(code >= 128)
    {
        static const VLCtab CBPtab0[32] = {
            { 0,0}, { 0,0}, { 0,0}, { 0,0}, { 0,0}, { 0,0}, { 0,0}, { 0,0},
            {62,5}, { 2,5}, {61,5}, { 1,5}, {56,5}, {52,5}, {44,5}, {28,5},
            {40,5}, {20,5}, {48,5}, {12,5}, {32,4}, {32,4}, {16,4}, {16,4},
            { 8,4}, { 8,4}, { 4,4}, { 4,4}, {60,3}, {60,3}, {60,3}, {60,3}
        };

        code >>= 4;

        apeg_flush_bits(layer, CBPtab0[code].len);
        return CBPtab0[code].val;
    }
    else
    {
        static const VLCtab CBPtab1[128] = {
            { 0,0}, { 0,9}, {39,9}, {27,9}, {59,9}, {55,9}, {47,9}, {31,9},
            {58,8}, {58,8}, {54,8}, {54,8}, {46,8}, {46,8}, {30,8}, {30,8},
            {57,8}, {57,8}, {53,8}, {53,8}, {45,8}, {45,8}, {29,8}, {29,8},
            {38,8}, {38,8}, {26,8}, {26,8}, {37,8}, {37,8}, {25,8}, {25,8},
            {43,8}, {43,8}, {23,8}, {23,8}, {51,8}, {51,8}, {15,8}, {15,8},
            {42,8}, {42,8}, {22,8}, {22,8}, {50,8}, {50,8}, {14,8}, {14,8},
            {41,8}, {41,8}, {21,8}, {21,8}, {49,8}, {49,8}, {13,8}, {13,8},
            {35,8}, {35,8}, {19,8}, {19,8}, {11,8}, {11,8}, { 7,8}, { 7,8},
            {34,7}, {34,7}, {34,7}, {34,7}, {18,7}, {18,7}, {18,7}, {18,7},
            {10,7}, {10,7}, {10,7}, {10,7}, { 6,7}, { 6,7}, { 6,7}, { 6,7},
            {33,7}, {33,7}, {33,7}, {33,7}, {17,7}, {17,7}, {17,7}, {17,7},
            { 9,7}, { 9,7}, { 9,7}, { 9,7}, { 5,7}, { 5,7}, { 5,7}, { 5,7},
            {63,6}, {63,6}, {63,6}, {63,6}, {63,6}, {63,6}, {63,6}, {63,6},
            { 3,6}, { 3,6}, { 3,6}, { 3,6}, { 3,6}, { 3,6}, { 3,6}, { 3,6},
            {36,6}, {36,6}, {36,6}, {36,6}, {36,6}, {36,6}, {36,6}, {36,6},
            {24,6}, {24,6}, {24,6}, {24,6}, {24,6}, {24,6}, {24,6}, {24,6}
        };

        apeg_flush_bits(layer, CBPtab1[code].len);
        return CBPtab1[code].val;
    }
}
