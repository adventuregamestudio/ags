#include <ctype.h>
#include <stdlib.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "mpg123.h"


int almp3tabsel_123[2][3][16] = {
    { {0,32,64,96,128,160,192,224,256,288,320,352,384,416,448,},
    {0,32,48,56, 64, 80, 96,112,128,160,192,224,256,320,384,},
    {0,32,40,48, 56, 64, 80, 96,112,128,160,192,224,256,320,} },
    { {0,32,48,56,64,80,96,112,128,144,160,176,192,224,256,},
    {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,},
    {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,} }
};

long almp3freqs[9] = { 44100, 48000, 32000,
22050, 24000, 16000 ,
11025 , 12000 , 8000 };

int bitindex;
unsigned char *wordpointer;


#define HDRCMPMASK 0xfffffd00


/*
* the code a header and write the information
* into the frame structure
*/
int decode_header(struct frame *fr,unsigned long newhead)
{
    if( newhead & (1<<20) ) {
        fr->lsf = (newhead & (1<<19)) ? 0x0 : 0x1;
        fr->mpeg25 = 0;
    }
    else {
        fr->lsf = 1;
        fr->mpeg25 = 1;
    }


    fr->lay = 4-((newhead>>17)&3);
    if( ((newhead>>10)&0x3) == 0x3) {
        return (0);
    }
    if(fr->mpeg25) {
        fr->sampling_frequency = 6 + ((newhead>>10)&0x3);
    }
    else
        fr->sampling_frequency = ((newhead>>10)&0x3) + (fr->lsf*3);
    fr->error_protection = ((newhead>>16)&0x1)^0x1;

    if(fr->mpeg25) /* allow Bitrate change for 2.5 ... */
        fr->bitrate_index = ((newhead>>12)&0xf);

    fr->bitrate_index = ((newhead>>12)&0xf);
    fr->padding   = ((newhead>>9)&0x1);
    fr->extension = ((newhead>>8)&0x1);
    fr->mode      = ((newhead>>6)&0x3);
    fr->mode_ext  = ((newhead>>4)&0x3);
    fr->copyright = ((newhead>>3)&0x1);
    fr->original  = ((newhead>>2)&0x1);
    fr->emphasis  = newhead & 0x3;

    fr->stereo    = (fr->mode == MPG_MD_MONO) ? 1 : 2;

    if(!fr->bitrate_index)
    {
        return (0);
    }

    switch(fr->lay)
    {
    case 1:
#if 0
        fr->do_layer = do_layer1;
        fr->jsbound = (fr->mode == MPG_MD_JOINT_STEREO) ?
            (fr->mode_ext<<2)+4 : 32;
        fr->framesize  = (long) almp3tabsel_123[fr->lsf][0][fr->bitrate_index] * 12000;
        fr->framesize /= almp3freqs[fr->sampling_frequency];
        fr->framesize  = ((fr->framesize+fr->padding)<<2)-4;
#endif
        break;
    case 2:
#if 1
        fr->do_layer = do_layer2;
        // in layer2.c
        //        II_select_table(fr);
        //        fr->jsbound = (fr->mode == MPG_MD_JOINT_STEREO) ?
        //                         (fr->mode_ext<<2)+4 : fr->II_sblimit;
        fr->framesize = (long) almp3tabsel_123[fr->lsf][1][fr->bitrate_index] * 144000;
        fr->framesize /= almp3freqs[fr->sampling_frequency];
        fr->framesize += fr->padding - 4;
#endif
        break;
    case 3:
        fr->do_layer = do_layer3;
#if 0
        if(fr->lsf)
            ssize = (fr->stereo == 1) ? 9 : 17;
        else
            ssize = (fr->stereo == 1) ? 17 : 32;
#endif

#if 0
        if(fr->error_protection)
            ssize += 2;
#endif
        {
            long ltmp;
            ltmp = (long)almp3tabsel_123[fr->lsf][2][fr->bitrate_index] * 144000;
            ltmp /= almp3freqs[fr->sampling_frequency]<<(fr->lsf);
            ltmp += fr->padding - 4;
            fr->framesize = ltmp;
        }

        break;
    default:
        return (0);
    }

    return 1;
}


unsigned int getbits(int number_of_bits)
{
    unsigned long rval;

    if(!number_of_bits)
        return 0;

    {
        rval = wordpointer[0];
        rval <<= 8;
        rval |= wordpointer[1];
        rval <<= 8;
        rval |= wordpointer[2];
        rval <<= bitindex;
        rval &= 0xffffff;

        bitindex += number_of_bits;

        rval >>= (24-number_of_bits);

        wordpointer += (bitindex>>3);
        bitindex &= 7;
    }
    return rval;
}

unsigned int getbits_fast(int number_of_bits)
{
    unsigned long rval;

    {
        rval = wordpointer[0];
        rval <<= 8;
        rval |= wordpointer[1];
        rval <<= bitindex;
        rval &= 0xffff;
        bitindex += number_of_bits;

        rval >>= (16-number_of_bits);

        wordpointer += (bitindex>>3);
        bitindex &= 7;
    }
    return rval;
}

int head_check(unsigned long head)
{

    if((head & 0xffe00000) != 0xffe00000)
        return FALSE;
    if(!((head>>17)&3))
        return FALSE;
    if(((head>>12)&0xf) == 0xf)
        return FALSE;
    if(((head>>10)&0x3) == 0x3)
        return FALSE;

    return TRUE;
}
