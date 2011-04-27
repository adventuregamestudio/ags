#ifndef DISABLE_MPEG_AUDIO

/*
 * Mpeg Layer-1,2,3 audio decoder
 * ------------------------------
 * copyright (c) 1995 by Michael Hipp, All rights reserved. See also 'README'
 * version for slower machines .. decodes only every second sample
 * sounds like 24000,22050 or 16000 kHz .. (depending on original sample freq.)
 *
 */

#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "mpg123.h"

#define WRITE_SAMPLE(samples, sum) \
	*samples = clamp_val(0, sum+32768, 65535);

void synth_2to1_mono(float *bandPtr,unsigned char *samples,int *pnt,float (*buffs)[2][0x110],int *b)
{
	short tmp1[32];
	short *samples_tmp = tmp1;
	int pnt_tmp = 0;
	int i;

	synth_2to1(bandPtr, 0, (unsigned char*)samples_tmp, &pnt_tmp, buffs, b);

	samples += *pnt;
	for(i=0;i<16;i++)
		((short*)samples)[i] = samples_tmp[i<<1];
	*pnt += 32;
}

void synth_2to1(float *bandPtr,int channel,unsigned char *out,int *pnt,float (*buffs)[2][0x110],int *b)
{
	static const int step = 2;
	unsigned short *samples = (unsigned short*)(out + *pnt);
	int bo = *b;

	float *b0, (*buf)[0x110], *window;
	float sum;
	int bo1, j;

	do_equalizer(bandPtr, channel);

	if(!channel)
	{
		bo = (bo-1) & 0xf;
		buf = buffs[0];
	}
	else
	{
		samples++;
		buf = buffs[1];
	}

	if(bo & 0x1)
	{
		b0 = buf[0];
		bo1 = bo;
		dct64(buf[1]+bo+1, buf[0]+bo, bandPtr);
	}
	else
	{
		b0 = buf[1];
		bo1 = bo+1;
		dct64(buf[0]+bo, buf[1]+bo+1, bandPtr);
	}

	window = decwin + 16 - bo1;
	for (j=8;j;j--,b0+=0x10,window+=0x30,samples+=step)
	{
		sum  = *window++ * *b0++;
		sum -= *window++ * *b0++;
		sum += *window++ * *b0++;
		sum -= *window++ * *b0++;
		sum += *window++ * *b0++;
		sum -= *window++ * *b0++;
		sum += *window++ * *b0++;
		sum -= *window++ * *b0++;
		sum += *window++ * *b0++;
		sum -= *window++ * *b0++;
		sum += *window++ * *b0++;
		sum -= *window++ * *b0++;
		sum += *window++ * *b0++;
		sum -= *window++ * *b0++;
		sum += *window++ * *b0++;
		sum -= *window++ * *b0++;
		WRITE_SAMPLE(samples, sum);
	}

	sum  = window[0x0] * b0[0x0];
	sum += window[0x2] * b0[0x2];
	sum += window[0x4] * b0[0x4];
	sum += window[0x6] * b0[0x6];
	sum += window[0x8] * b0[0x8];
	sum += window[0xA] * b0[0xA];
	sum += window[0xC] * b0[0xC];
	sum += window[0xE] * b0[0xE];
	WRITE_SAMPLE(samples, sum);
	b0-=0x20,window-=0x40,samples+=step;

	window += bo1<<1;
	for (j=7;j;j--,b0-=0x30,window-=0x30,samples+=step)
	{
		sum = -*(--window) * *b0++;
		sum -= *(--window) * *b0++;
		sum -= *(--window) * *b0++;
		sum -= *(--window) * *b0++;
		sum -= *(--window) * *b0++;
		sum -= *(--window) * *b0++;
		sum -= *(--window) * *b0++;
		sum -= *(--window) * *b0++;
		sum -= *(--window) * *b0++;
		sum -= *(--window) * *b0++;
		sum -= *(--window) * *b0++;
		sum -= *(--window) * *b0++;
		sum -= *(--window) * *b0++;
		sum -= *(--window) * *b0++;
		sum -= *(--window) * *b0++;
		sum -= *(--window) * *b0++;
		WRITE_SAMPLE(samples, sum);
	}

	*pnt += 64;
	*b = bo;
}

#endif
