/* getbits.c, bit level routines                                            */

/*
* All modifications (mpeg2decode -> mpeg2play) are
* Copyright (C) 1996, Stefan Eckart. All Rights Reserved.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <allegro.h>

#include "mpeg1dec.h"


static void read_packet(APEG_LAYER *layer);

/* MPEG-1 system layer demultiplexer */
static INLINE unsigned int get_byte(APEG_LAYER *layer)
{
    layer->Rdmax -= 1;
    return pack_getc(layer->pf);
}

/* extract a 16-bit word from the bitstream buffer */
static INLINE unsigned int get_word(APEG_LAYER *layer)
{
    layer->Rdmax -= 2;
    return pack_mgetw(layer->pf);
}

/* extract a 32-bit long from the bitstream buffer */
static INLINE unsigned int get_long(APEG_LAYER *layer)
{
    layer->Rdmax -= 4;
    return pack_mgetl(layer->pf);
}

static INLINE void skip_bytes(APEG_LAYER *layer, int bytes)
{
    layer->Rdmax -= bytes;
    pack_fseek(layer->pf, bytes);
}

/* parse system layer, ignore everything we don't need */
static void read_packet(APEG_LAYER *layer)
{
    unsigned int code;

next_code:
    code = get_long(layer);

    /* remove system layer byte stuffing */
    while((code & 0xFFFFFF00) != 0x100 && !pack_feof(layer->pf))
        code = (code<<8) | get_byte(layer);

    switch(code)
    {
    case PACK_START_CODE: /* pack header */
        /* skip pack header (system_clock_reference and mux_rate) */
        skip_bytes(layer, 8);
        goto next_code;

    case VIDEO_ELEMENTARY_STREAM:
        //		case AUDIO_ELEMENTARY_STREAM:
        code = get_word(layer); // packet_length
        layer->Rdmax = code;

        code = get_byte(layer);
        /*			if((code>>6)==0x02)
        {
        skip_bytes(layer, 1);
        code = get_byte(layer);
        // advance pointer by PES_header_data_length
        skip_bytes(layer, code);
        return;
        }*/

        // parse MPEG-1 packet header
        while(code == 0xFF)
            code = get_byte(layer);

        // stuffing bytes
        if(code >= 0x40)
        {
            if(code >= 0x80)
                apeg_error_jump("Error in packet header");

            // skip STD_buffer_scale
            skip_bytes(layer, 1);
            code = get_byte(layer);
            if(code >= 0x40)
                apeg_error_jump("Error in packet header");
        }

        // skip presentation and decoding time stamps
        if(code >= 0x30)
            skip_bytes(layer, 9);
        // skip presentation time stamps
        else if(code >= 0x20)
            skip_bytes(layer, 4);
        else if(code != 0x0f)
            apeg_error_jump("Error in packet header");

        return;

    case ISO_END_CODE: /* end */
        while(!pack_feof(layer->pf))
            pack_fseek(layer->pf, ((~0u)>>1u));
        layer->Rdmax = ~0u;
        return;

    default:
        if(pack_feof(layer->pf))
        {
            layer->Rdmax = ~0u;
            return;
        }
        if(code >= SYSTEM_START_CODE)
        {
            /* skip system headers and non-video/audio packets */
            code = get_word(layer);
            skip_bytes(layer, code);
        }
        else
        {
            sprintf(apeg_error, "Unknown startcode 0x%08x in system layer",
                code);
            apeg_error_jump(NULL);
        }
        goto next_code;
    }
}

/* advance by n bits */
void apeg_flush_bits(APEG_LAYER *layer, int n)
{
    int Incnt = (layer->Incnt -= n);

    layer->Bfr <<= n;

    if(Incnt <= 24)
    {
        do {
            if(layer->Rdmax <= 0)
                read_packet(layer);

            layer->Bfr |= get_byte(layer) << (24 - Incnt);
            Incnt += 8;
        } while (Incnt <= 24);

        layer->Incnt = Incnt;
    }
}


void apeg_flush_bits1(APEG_LAYER *layer)
{
    if(--(layer->Incnt) == 24)
    {
        if(layer->Rdmax <= 0)
            read_packet(layer);

        layer->Bfr = (layer->Bfr<<1) | get_byte(layer);
        layer->Incnt = 32;
    }
    else
        layer->Bfr <<= 1;
}

void apeg_flush_bits8(APEG_LAYER *layer, int n)
{
    if((layer->Incnt -= n) <= 24)
    {
        if(layer->Rdmax <= 0)
            read_packet(layer);

        layer->Bfr = (layer->Bfr<<n) | (get_byte(layer) << (24 - layer->Incnt));
        layer->Incnt += 8;
    }
    else
        layer->Bfr <<= n;
}


void apeg_flush_bits32(APEG_LAYER *layer)
{
    const int Incnt = layer->Incnt - 32;
    int i;

    layer->Bfr = 0;

    i = 3;
    do {
        if(layer->Rdmax <= 0)
            read_packet(layer);
        layer->Bfr |= get_byte(layer) << (i*8 - Incnt);
    } while(--i >= 0);
}



/* return next n bits (right adjusted) */
unsigned int apeg_get_bits(APEG_LAYER *layer, int n)
{
    const unsigned int Val = layer->Bfr >> (32-n);
    int Incnt = (layer->Incnt -= n);

    layer->Bfr <<= n;

    if(Incnt <= 24)
    {
        do {
            if(layer->Rdmax <= 0)
                read_packet(layer);

            layer->Bfr |= get_byte(layer) << (24 - Incnt);
            Incnt += 8;
        } while(Incnt <= 24);

        layer->Incnt = Incnt;
    }

    return Val;
}


unsigned int apeg_get_bits1(APEG_LAYER *layer)
{
    const unsigned int val = layer->Bfr >> 31;

    if(--(layer->Incnt) == 24)
    {
        if(layer->Rdmax <= 0)
            read_packet(layer);
        layer->Bfr = (layer->Bfr<<1) | get_byte(layer);

        layer->Incnt = 32;
    }
    else
        layer->Bfr <<= 1;

    return val;
}


unsigned int apeg_get_bits8(APEG_LAYER *layer, int n)
{
    const unsigned int Val = show_bits(layer, n);
    const int Incnt = (layer->Incnt -= n);

    if(Incnt <= 24)
    {
        if(layer->Rdmax <= 0)
            read_packet(layer);
        layer->Bfr = (layer->Bfr<<n) | (get_byte(layer) << (24 - Incnt));

        layer->Incnt = Incnt+8;
    }
    else
        layer->Bfr <<= n;

    return Val;
}

unsigned int apeg_get_bits32(APEG_LAYER *layer)
{
    const unsigned int l = show_bits32(layer);

    apeg_flush_bits32(layer);

    return l;
}

/* align to start of next start code */
int apeg_start_code(APEG_LAYER *layer)
{
    // byte align
    if(layer->Incnt&7)
    {
        if(layer->Rdmax <= 0)
            read_packet(layer);

        layer->Bfr = (layer->Bfr << (layer->Incnt&7)) | get_byte(layer);
        layer->Incnt = 32;
    }

    while((show_bits32(layer) & 0xFFFFFF00) != 0x100)
    {
        if(pack_feof(layer->pf))
            return SEQUENCE_END_CODE;

        if(layer->Rdmax <= 0)
            read_packet(layer);
        layer->Bfr = (layer->Bfr<<8) | get_byte(layer);
    }

    return show_bits32(layer);
}
