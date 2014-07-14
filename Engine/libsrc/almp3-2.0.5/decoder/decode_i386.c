/*
* Mpeg Layer-1,2,3 audio decoder
* ------------------------------
* copyright (c) 1995,1996,1997 by Michael Hipp, All rights reserved.
* See also 'README'
*
* slighlty optimized for machines without autoincrement/decrement.
* The performance is highly compiler dependend. Maybe
* the decode.c version for 'normal' processor may be faster
* even for Intel processors.
*/

#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "mpg123.h"
#include "mpglib.h"

/* old WRITE_SAMPLE */
#define WRITE_SAMPLE(samples,sum,clip) \
    if( (sum) > 32767.0) { *(samples) = 32767; (clip)++; } \
  else if( (sum) < -32768.0) { *(samples) = -32768; (clip)++; } \
  else { *(samples) = (short)sum; }

int synth_1to1_mono(void *mp,real *bandPtr,unsigned char *samples,int *pnt)
{
    short samples_tmp[64];
    short *tmp1 = samples_tmp;
    int i,ret;
    int pnt1 = 0;

    ret = synth_1to1(mp,bandPtr,0,(unsigned char *) samples_tmp,&pnt1);
    samples += *pnt;

    for(i=0;i<32;i++) {
        *( (short *) samples) = *tmp1;
        samples += 2;
        tmp1 += 2;
    }
    *pnt += 64;

    return ret;
}


int synth_1to1(void *mp,real *bandPtr,int channel,unsigned char *out,int *pnt)
{
    struct mpstr *gmp = mp;

    static const int step = 2;
    int bo;
    short *samples = (short *) (out + *pnt);

    real *b0,(*buf)[0x110];
    int clip = 0;
    int bo1;

    bo = gmp->synth_bo;

    if(!channel) {
        bo--;
        bo &= 0xf;
        buf = gmp->synth_buffs[0];
    }
    else {
        samples++;
        buf = gmp->synth_buffs[1];
    }

    if(bo & 0x1) {
        b0 = buf[0];
        bo1 = bo;
        dct64(buf[1]+((bo+1)&0xf),buf[0]+bo,bandPtr);
    }
    else {
        b0 = buf[1];
        bo1 = bo+1;
        dct64(buf[0]+bo,buf[1]+bo+1,bandPtr);
    }

    gmp->synth_bo = bo;

    {
        register int j;
        real *window = decwin + 16 - bo1;

        for (j=16;j;j--,b0+=0x10,window+=0x20,samples+=step)
        {
            real sum;
            sum  = window[0x0] * b0[0x0];
            sum -= window[0x1] * b0[0x1];
            sum += window[0x2] * b0[0x2];
            sum -= window[0x3] * b0[0x3];
            sum += window[0x4] * b0[0x4];
            sum -= window[0x5] * b0[0x5];
            sum += window[0x6] * b0[0x6];
            sum -= window[0x7] * b0[0x7];
            sum += window[0x8] * b0[0x8];
            sum -= window[0x9] * b0[0x9];
            sum += window[0xA] * b0[0xA];
            sum -= window[0xB] * b0[0xB];
            sum += window[0xC] * b0[0xC];
            sum -= window[0xD] * b0[0xD];
            sum += window[0xE] * b0[0xE];
            sum -= window[0xF] * b0[0xF];

            WRITE_SAMPLE(samples,sum,clip);
        }

        {
            real sum;
            sum  = window[0x0] * b0[0x0];
            sum += window[0x2] * b0[0x2];
            sum += window[0x4] * b0[0x4];
            sum += window[0x6] * b0[0x6];
            sum += window[0x8] * b0[0x8];
            sum += window[0xA] * b0[0xA];
            sum += window[0xC] * b0[0xC];
            sum += window[0xE] * b0[0xE];
            WRITE_SAMPLE(samples,sum,clip);
            b0-=0x10,window-=0x20,samples+=step;
        }
        window += bo1<<1;

        for (j=15;j;j--,b0-=0x10,window-=0x20,samples+=step)
        {
            real sum;
            sum = -window[-0x1] * b0[0x0];
            sum -= window[-0x2] * b0[0x1];
            sum -= window[-0x3] * b0[0x2];
            sum -= window[-0x4] * b0[0x3];
            sum -= window[-0x5] * b0[0x4];
            sum -= window[-0x6] * b0[0x5];
            sum -= window[-0x7] * b0[0x6];
            sum -= window[-0x8] * b0[0x7];
            sum -= window[-0x9] * b0[0x8];
            sum -= window[-0xA] * b0[0x9];
            sum -= window[-0xB] * b0[0xA];
            sum -= window[-0xC] * b0[0xB];
            sum -= window[-0xD] * b0[0xC];
            sum -= window[-0xE] * b0[0xD];
            sum -= window[-0xF] * b0[0xE];
            sum -= window[-0x0] * b0[0xF];

            WRITE_SAMPLE(samples,sum,clip);
        }
    }
    *pnt += 128;

    return clip;
}






int tsynth_1to1(real *bandPtr,int channel,unsigned char *out,int *pnt)
{
    static real buffs[2][2][0x110];
    static const int step = 2;
    static int bo = 1;
    short *samples = (short *) (out + *pnt);

    real *b0,(*buf)[0x110];
    int clip = 0;
    int bo1;

    if(!channel) {
        bo--;
        bo &= 0xf;
        buf = buffs[0];
    }
    else {
        samples++;
        buf = buffs[1];
    }

    if(bo & 0x1) {
        b0 = buf[0];
        bo1 = bo;
        dct64(buf[1]+((bo+1)&0xf),buf[0]+bo,bandPtr);
    }
    else {
        b0 = buf[1];
        bo1 = bo+1;
        dct64(buf[0]+bo,buf[1]+bo+1,bandPtr);
    }

    {
        register int j;
        real *window = decwin + 16 - bo1;

        for (j=16;j;j--,b0+=0x10,window+=0x20,samples+=step)
        {
            real sum;
            sum  = window[0x0] * b0[0x0];
            sum -= window[0x1] * b0[0x1];
            sum += window[0x2] * b0[0x2];
            sum -= window[0x3] * b0[0x3];
            sum += window[0x4] * b0[0x4];
            sum -= window[0x5] * b0[0x5];
            sum += window[0x6] * b0[0x6];
            sum -= window[0x7] * b0[0x7];
            sum += window[0x8] * b0[0x8];
            sum -= window[0x9] * b0[0x9];
            sum += window[0xA] * b0[0xA];
            sum -= window[0xB] * b0[0xB];
            sum += window[0xC] * b0[0xC];
            sum -= window[0xD] * b0[0xD];
            sum += window[0xE] * b0[0xE];
            sum -= window[0xF] * b0[0xF];

            WRITE_SAMPLE(samples,sum,clip);
        }

        {
            real sum;
            sum  = window[0x0] * b0[0x0];
            sum += window[0x2] * b0[0x2];
            sum += window[0x4] * b0[0x4];
            sum += window[0x6] * b0[0x6];
            sum += window[0x8] * b0[0x8];
            sum += window[0xA] * b0[0xA];
            sum += window[0xC] * b0[0xC];
            sum += window[0xE] * b0[0xE];
            WRITE_SAMPLE(samples,sum,clip);
            b0-=0x10,window-=0x20,samples+=step;
        }
        window += bo1<<1;

        for (j=15;j;j--,b0-=0x10,window-=0x20,samples+=step)
        {
            real sum;
            sum = -window[-0x1] * b0[0x0];
            sum -= window[-0x2] * b0[0x1];
            sum -= window[-0x3] * b0[0x2];
            sum -= window[-0x4] * b0[0x3];
            sum -= window[-0x5] * b0[0x4];
            sum -= window[-0x6] * b0[0x5];
            sum -= window[-0x7] * b0[0x6];
            sum -= window[-0x8] * b0[0x7];
            sum -= window[-0x9] * b0[0x8];
            sum -= window[-0xA] * b0[0x9];
            sum -= window[-0xB] * b0[0xA];
            sum -= window[-0xC] * b0[0xB];
            sum -= window[-0xD] * b0[0xC];
            sum -= window[-0xE] * b0[0xD];
            sum -= window[-0xF] * b0[0xE];
            sum -= window[-0x0] * b0[0xF];

            WRITE_SAMPLE(samples,sum,clip);
        }
    }
    *pnt += 128;

    return clip;
}



