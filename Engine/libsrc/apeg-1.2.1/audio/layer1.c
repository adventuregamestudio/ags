#ifndef DISABLE_MPEG_AUDIO

/* 
 * Mpeg Layer-1 audio decoder 
 * --------------------------
 * copyright (c) 1995 by Michael Hipp, All rights reserved. See also 'README'
 * near unoptimzed ...
 *
 * may have a few bugs after last optimization ... 
 *
 */

#include "apeg.h"
#include "mpg123.h"
#include "getbits.h"

static void I_step_one(unsigned int *balloc, unsigned int (*scale_index)[SBLIMIT],
                struct frame *fr, struct mpstr *mp)
{
  unsigned int *ba=balloc;
  unsigned int *sca = (unsigned int *) scale_index;

  if(fr->stereo) {
    int i;
    int jsbound = fr->jsbound;
    for (i=0;i<jsbound;i++) { 
      *ba++ = getbits(&mp->bsi,4);
      *ba++ = getbits(&mp->bsi,4);
    }
    for (i=jsbound;i<SBLIMIT;i++)
      *ba++ = getbits(&mp->bsi,4);

    ba = balloc;

    for (i=0;i<jsbound;i++) {
      if ((*ba++))
        *sca++ = getbits(&mp->bsi,6);
      if ((*ba++))
        *sca++ = getbits(&mp->bsi,6);
    }
    for (i=jsbound;i<SBLIMIT;i++)
      if ((*ba++)) {
        *sca++ =  getbits(&mp->bsi,6);
        *sca++ =  getbits(&mp->bsi,6);
      }
  }
  else {
    int i;
    for (i=0;i<SBLIMIT;i++)
      *ba++ = getbits(&mp->bsi,4);
    ba = balloc;
    for (i=0;i<SBLIMIT;i++)
      if ((*ba++))
        *sca++ = getbits(&mp->bsi,6);
  }
}

static void I_step_two(float (*fraction)[SBLIMIT], unsigned int *balloc,
                unsigned int (*scale_index)[SBLIMIT], struct frame *fr,
                struct mpstr *mp)
{
  int i,n;
  int smpb[2*SBLIMIT]; /* values: 0-65535 */
  int *sample;
  unsigned int *ba;
  unsigned int *sca = (unsigned int *) scale_index;

  if(fr->stereo) {
    int jsbound = fr->jsbound;
    float *f0 = fraction[0];
    float *f1 = fraction[1];
    ba = balloc;
    for (sample=smpb,i=0;i<jsbound;i++)  {
      if ((n = *ba++))
        *sample++ = getbits(&mp->bsi,n+1);
      if ((n = *ba++))
        *sample++ = getbits(&mp->bsi,n+1);
    }
    for (i=jsbound;i<SBLIMIT;i++) 
      if ((n = *ba++))
        *sample++ = getbits(&mp->bsi,n+1);

    ba = balloc;
    for (sample=smpb,i=0;i<jsbound;i++) {
      if((n=*ba++))
        *f0++ = (float) ( ((-1)<<n) + (*sample++) + 1) * muls[n+1][*sca++];
      else
        *f0++ = 0.0;
      if((n=*ba++))
        *f1++ = (float) ( ((-1)<<n) + (*sample++) + 1) * muls[n+1][*sca++];
      else
        *f1++ = 0.0;
    }
    for (i=jsbound;i<SBLIMIT;i++) {
      if ((n=*ba++)) {
        float samp = ( ((-1)<<n) + (*sample++) + 1);
        *f0++ = samp * muls[n+1][*sca++];
        *f1++ = samp * muls[n+1][*sca++];
      }
      else
        *f0++ = *f1++ = 0.0;
    }
    for(i=fr->down_sample_sblimit;i<32;i++)
      fraction[0][i] = fraction[1][i] = 0.0;
  }
  else {
    float *f0 = fraction[0];
    ba = balloc;
    for (sample=smpb,i=0;i<SBLIMIT;i++)
      if ((n = *ba++))
        *sample++ = getbits(&mp->bsi,n+1);
    ba = balloc;
    for (sample=smpb,i=0;i<SBLIMIT;i++) {
      if((n=*ba++))
        *f0++ = (float) ( ((-1)<<n) + (*sample++) + 1) * muls[n+1][*sca++];
      else
        *f0++ = 0.0;
    }
    for(i=fr->down_sample_sblimit;i<32;i++)
      fraction[0][i] = 0.0;
  }
}

int do_layer1(struct mpstr *mp, struct frame *fr, APEG_LAYER *ai)
{
	const int ds = ai->stream.audio.down_sample;

	unsigned int (*scale_index)[SBLIMIT] = (unsigned int(*)[SBLIMIT])mp->scalefacs;
	float (*fraction)[SBLIMIT] = (float(*)[SBLIMIT])mp->hybridOut;
	unsigned int *balloc = mp->balloc;
	int i;

	fr->jsbound = (fr->mode == MPG_MD_JOINT_STEREO) ? (fr->mode_ext<<2)+4 : 32;

	I_step_one(balloc,scale_index,fr,mp);

	for(i = 0;i < SCALE_BLOCK;i++)
	{
		I_step_two(fraction,balloc,scale_index,fr,mp);

		if(ai->stream.audio.channels == 1)
		{
			synth_mono(ds, (float*)fraction[0], ai->audio.pcm.samples, 
			           &(ai->audio.pcm.point), mp->buffs, &(mp->bo));
		}
		else
		{
			int p1 = ai->audio.pcm.point;
			synth(ds, (float*)fraction[0], 0, ai->audio.pcm.samples, &p1, mp->buffs, &(mp->bo));
			synth(ds, (float*)fraction[1], 1, ai->audio.pcm.samples,
			      &(ai->audio.pcm.point), mp->buffs, &(mp->bo));
		}
	}

	return 1;
}

#endif
