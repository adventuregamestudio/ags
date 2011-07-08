/* Predict.c, motion compensation routines                                    */

#include <stdio.h>
#include <string.h>

#include <allegro.h>
#include "mpeg1dec.h"

/* private prototypes */
#define UINT32_PTR(x)	((uint32_t*)(x))
#define UINT16_PTR(x)	((uint16_t*)(x))
static void form_prediction(unsigned char**, unsigned char**, int, int, int, int, int);
static void form_prediction_hq(unsigned char**, unsigned char**, int, int, int, int, int);
static void form_avg_prediction(unsigned char**, unsigned char**, int, int, int, int, int);
static void form_avg_prediction_hq(unsigned char**, unsigned char**, int, int, int, int, int);

static void recon(unsigned char*, unsigned char*, int, int, int, int, int, int);
static void recon_h(unsigned char*, unsigned char*, int, int, int, int, int, int);
static void recon_v(unsigned char*, unsigned char*, int, int, int, int, int, int);
static void recon_hv(unsigned char*, unsigned char*, int, int, int, int, int, int);

static void recon_avg(unsigned char*, unsigned char*, int, int, int, int, int, int);
static void recon_h_avg(unsigned char*, unsigned char*, int, int, int, int, int, int);
static void recon_v_avg(unsigned char*, unsigned char*, int, int, int, int, int, int);
static void recon_hv_avg(unsigned char*, unsigned char*, int, int, int, int, int, int);


void apeg_form_f_pred(APEG_LAYER *layer, int bx, int by, int *PMV)
{
	if((layer->quality & RECON_SUBPIXEL))
		form_prediction_hq(layer->forward_frame, layer->current_frame,
		                   layer->coded_width, bx, by, PMV[0], PMV[1]);
	else
		form_prediction(layer->forward_frame, layer->current_frame,
		                layer->coded_width, bx, by, PMV[0], PMV[1]);
}

void apeg_form_b_pred(APEG_LAYER *layer, int bx, int by, int *PMV)
{
	if((layer->quality & RECON_SUBPIXEL))
		form_prediction_hq(layer->backward_frame, layer->current_frame,
		                   layer->coded_width, bx, by, PMV[0], PMV[1]);
	else
		form_prediction(layer->backward_frame, layer->current_frame,
		                layer->coded_width, bx, by, PMV[0], PMV[1]);
}

void apeg_form_fb_pred(APEG_LAYER *layer, int bx, int by, int *PMV)
{
	if((layer->quality & RECON_SUBPIXEL))
		form_prediction_hq(layer->forward_frame, layer->current_frame,
		                   layer->coded_width, bx, by, PMV[0], PMV[1]);
	else
		form_prediction(layer->forward_frame, layer->current_frame,
		                layer->coded_width, bx, by, PMV[0], PMV[1]);

	if((layer->quality & RECON_AVG_SUBPIXEL))
		form_avg_prediction_hq(layer->backward_frame, layer->current_frame,
		                       layer->coded_width, bx, by, PMV[2], PMV[3]);
	else
		form_avg_prediction(layer->backward_frame, layer->current_frame,
		                    layer->coded_width, bx, by, PMV[2], PMV[3]);
}


static void form_prediction(unsigned char **src, unsigned char **dst, int lx, int x, int y, int dx, int dy)
{
	if((dx|dy) == 0)
	{
		apeg_empty_pred(src, dst, x, y, lx);
		return;
	}

	recon(src[0],dst[0],lx,16,x,y,dx,dy);
	lx >>= 1; x >>= 1; dx >>= 1;
	y >>= 1; dy >>= 1;
	recon(src[1],dst[1],lx,8,x,y,dx,dy);
	recon(src[2],dst[2],lx,8,x,y,dx,dy);
}

static void form_prediction_hq(unsigned char **src, unsigned char **dst, int lx, int x, int y, int dx, int dy)
{
	if((dx|dy) == 0)
	{
		apeg_empty_pred(src, dst, x, y, lx);
		return;
	}

	if(!(dx&1))
	{
		if(!(dy&1))
			recon(src[0],dst[0],lx,16,x,y,dx,dy);
		else
			recon_v(src[0],dst[0],lx,16,x,y,dx,dy);
	}
	else
	{
		if(!(dy&1))
			recon_h(src[0],dst[0],lx,16,x,y,dx,dy);
		else
			recon_hv(src[0],dst[0],lx,16,x,y,dx,dy);
	}

	lx >>= 1; x >>= 1; dx >>= 1;
	y >>= 1; dy >>= 1;

	if((dx&1) == 0)
	{
		if((dy&1) == 0)
		{
			recon(src[1],dst[1],lx,8,x,y,dx,dy);
			recon(src[2],dst[2],lx,8,x,y,dx,dy);
		}
		else
		{
			recon_v(src[1],dst[1],lx,8,x,y,dx,dy);
			recon_v(src[2],dst[2],lx,8,x,y,dx,dy);
		}
	}
	else
	{
		if((dy&1) == 0)
		{
			recon_h(src[1],dst[1],lx,8,x,y,dx,dy);
			recon_h(src[2],dst[2],lx,8,x,y,dx,dy);
		}
		else
		{
			recon_hv(src[1],dst[1],lx,8,x,y,dx,dy);
			recon_hv(src[2],dst[2],lx,8,x,y,dx,dy);
		}
	}
}


static void form_avg_prediction(unsigned char **src, unsigned char **dst, int lx, int x, int y, int dx, int dy)
{
	recon_avg(src[0],dst[0],lx,16,x,y,dx,dy);
	lx >>= 1; x >>= 1; dx >>= 1;
	y >>= 1; dy >>= 1;
	recon_avg(src[1],dst[1],lx,8,x,y,dx,dy);
	recon_avg(src[2],dst[2],lx,8,x,y,dx,dy);
}

static void form_avg_prediction_hq(unsigned char **src, unsigned char **dst, int lx, int x, int y, int dx, int dy)
{
	if(!(dx&1))
	{
		if(!(dy&1))
			recon_avg(src[0],dst[0],lx,16,x,y,dx,dy);
		else
			recon_v_avg(src[0],dst[0],lx,16,x,y,dx,dy);
	}
	else
	{
		if(!(dy&1))
			recon_h_avg(src[0],dst[0],lx,16,x,y,dx,dy);
		else
			recon_hv_avg(src[0],dst[0],lx,16,x,y,dx,dy);
	}

	lx >>= 1; x >>= 1; dx >>= 1;
	y >>= 1; dy >>= 1;

	if(!(dx&1))
	{
		if(!(dy&1))
		{
			recon_avg(src[1],dst[1],lx,8,x,y,dx,dy);
			recon_avg(src[2],dst[2],lx,8,x,y,dx,dy);
		}
		else
		{
			recon_v_avg(src[1],dst[1],lx,8,x,y,dx,dy);
			recon_v_avg(src[2],dst[2],lx,8,x,y,dx,dy);
		}
	}
	else
	{
		if(!(dy&1))
		{
			recon_h_avg(src[1],dst[1],lx,8,x,y,dx,dy);
			recon_h_avg(src[2],dst[2],lx,8,x,y,dx,dy);
		}
		else
		{
			recon_hv_avg(src[1],dst[1],lx,8,x,y,dx,dy);
			recon_hv_avg(src[2],dst[2],lx,8,x,y,dx,dy);
		}
	}
}


static void recon(unsigned char *src, unsigned char *dst, int lx, int bs, int x, int y, int dx, int dy)
{
	/* half pel scaling for integer vectors */
	const int xint = dx>>1;
	const int yint = dy>>1;

	/* compute the linear address of pel_ref[][] and pel_pred[][] 
	   based on cartesian/raster cordinates provided */
	const unsigned char *s = src + lx*(y+yint) + x + xint;
	unsigned char *d = dst + lx*y + x;
	int i;

	for(i = 0;i < bs;++i)
	{
		memcpy(d, s, bs);
		s += lx;  d += lx;
	}
}

static void recon_h(unsigned char *src, unsigned char *dst, int lx, int bs, int x, int y, int dx, int dy)
{
	const int xint = dx>>1;
	const int yint = dy>>1;

	const unsigned char *s = src + lx*(y+yint) + x + xint;
	uint32_t *d = (uint32_t*)(dst + lx*y + x);
	int i;
	
	lx >>= 2;
	if(bs == 16)
	{
		for(i = 0;i < 16;++i)
		{
			d[0] = (UINT32_PTR(s)[0] & UINT32_PTR(s+1)[0]) + (((UINT32_PTR(s)[0] ^ UINT32_PTR(s+1)[0]) >> 1) & 0x7F7F7F7F);
			d[1] = (UINT32_PTR(s)[1] & UINT32_PTR(s+1)[1]) + (((UINT32_PTR(s)[1] ^ UINT32_PTR(s+1)[1]) >> 1) & 0x7F7F7F7F);
			d[2] = (UINT32_PTR(s)[2] & UINT32_PTR(s+1)[2]) + (((UINT32_PTR(s)[2] ^ UINT32_PTR(s+1)[2]) >> 1) & 0x7F7F7F7F);
			d[3] = (UINT32_PTR(s)[3] & UINT32_PTR(s+1)[3]) + (((UINT32_PTR(s)[3] ^ UINT32_PTR(s+1)[3]) >> 1) & 0x7F7F7F7F);
			s += (lx<<2);  d += lx;
		}
	}
	else
	{
		for(i = 0;i < 8;++i)
		{
			d[0] = (UINT32_PTR(s)[0] & UINT32_PTR(s+1)[0]) + (((UINT32_PTR(s)[0] ^ UINT32_PTR(s+1)[0]) >> 1) & 0x7F7F7F7F);
			d[1] = (UINT32_PTR(s)[1] & UINT32_PTR(s+1)[1]) + (((UINT32_PTR(s)[1] ^ UINT32_PTR(s+1)[1]) >> 1) & 0x7F7F7F7F);
			s += (lx<<2);  d += lx;
		}
	}
}

static void recon_v(unsigned char *src, unsigned char *dst, int lx, int bs, int x, int y, int dx, int dy)
{
	const int xint = dx>>1;
	const int yint = dy>>1;
	
	const uint32_t *s = (uint32_t*)(src + lx*(y+yint) + x + xint);
	uint32_t *d = (uint32_t*)(dst + lx*y + x);
	int i;
	
	lx >>= 2;
	if(bs == 16)
	{
		for(i = 0;i < 16;i++)
		{
			d[0] = (s[0] & s[0+lx]) + (((s[0] ^ s[0+lx]) >> 1) & 0x7F7F7F7F);
			d[1] = (s[1] & s[1+lx]) + (((s[1] ^ s[1+lx]) >> 1) & 0x7F7F7F7F);
			d[2] = (s[2] & s[2+lx]) + (((s[2] ^ s[2+lx]) >> 1) & 0x7F7F7F7F);
			d[3] = (s[3] & s[3+lx]) + (((s[3] ^ s[3+lx]) >> 1) & 0x7F7F7F7F);
			s+= lx;  d+= lx;
		}
	}
	else
	{
		for(i = 0;i < 8;i++)
		{
			d[0] = (s[0] & s[0+lx]) + (((s[0] ^ s[0+lx]) >> 1) & 0x7F7F7F7F);
			d[1] = (s[1] & s[1+lx]) + (((s[1] ^ s[1+lx]) >> 1) & 0x7F7F7F7F);
			s+= lx;  d+= lx;
		}
	}
}

static void recon_hv(unsigned char *src, unsigned char *dst, int lx, int bs, int x, int y, int dx, int dy)
{
	const int xint = dx>>1;
	const int yint = dy>>1;

	const unsigned char *s = src + lx*(y+yint) + x + xint;
	unsigned char *d = dst + lx*y + x;
	int i, j;

	for(i = 0;i < bs;i++)
	{
		for(j = 0;j < bs;++j)
			d[j] = (unsigned int)(s[j]+s[j+1]+s[j+lx]+s[j+lx+1])>>2;
		s+= lx; d+= lx;
	}

}

static void recon_avg(unsigned char *src, unsigned char *dst, int lx, int bs, int x, int y, int dx, int dy)
{
	const int xint = dx>>1;
	const int yint = dy>>1;

	const uint32_t *s = (uint32_t*)(src + lx*(y+yint) + x + xint);
	uint32_t *d = (uint32_t*)(dst + lx*y + x);
	int i, j;
	int bs4 = bs>>2;

	lx >>= 2;
	for(i = 0;i < bs;i++)
	{
		for(j = 0;j < bs4;j++)
			d[j] = (s[j] & d[j]) + (((s[j] ^ d[j]) >> 1) & 0x7F7F7F7F);
		s+= lx;  d+= lx;
	}
}

static void recon_h_avg(unsigned char *src, unsigned char *dst, int lx, int bs, int x, int y, int dx, int dy)
{
	int xint = dx>>1;
	int yint = dy>>1;

	const unsigned char *s = src + lx*(y+yint) + x + xint;
	unsigned char *d = dst + lx*y + x;
	int i, j;

	for(i = 0;i < bs;++i)
	{
		for(j = 0;j < bs;++j)
			d[j] = (d[j] + ((unsigned int)(s[j]+s[j+1])>>1)) >> 1;

		s += lx; d += lx;
	}
}


static void recon_v_avg(unsigned char *src, unsigned char *dst, int lx, int bs, int x, int y, int dx, int dy)
{
	int xint = dx>>1;
	int yint = dy>>1;

	const unsigned char *s = src + lx*(y+yint) + x + xint;
	unsigned char *d = dst + lx*y + x;
	int i, j;

	for(i = 0;i < bs;++i)
	{
		for(j = 0;j < bs;++j)
			d[j] = (d[j] + ((unsigned int)(s[j]+s[j+lx])>>1)) >> 1;

		s += lx; d += lx;
	}
}

static void recon_hv_avg(unsigned char *src, unsigned char *dst, int lx, int bs, int x, int y, int dx, int dy)
{
	int xint = dx>>1;
	int yint = dy>>1;

	const unsigned char *s = src + lx*(y+yint) + x + xint;
	unsigned char *d = dst + lx*y + x;
	int i, j;

	for(i = 0;i < bs;++i)
	{
		for(j = 0;j < bs;++j)
			d[j] = (d[j] + ((unsigned int)(s[j]+s[j+1]+s[j+lx]+s[j+1+lx])>>2)) >> 1;

		s += lx; d += lx;
	}
}

void apeg_empty_pred(unsigned char **src, unsigned char **dst, int x, int y, int w)
{
	int offset = (y*w) + x;
	unsigned char *s = src[0] + offset;
	unsigned char *d = dst[0] + offset;
	unsigned char *s2;
	unsigned char *d2;
	int i;

	for(i = 0;i < 16;++i)
	{
		memcpy(d, s, 16);
		s += w; d += w;
	}

	x >>= 1; y >>= 1;
	w >>= 1;
	offset = (y*w) + x;

	s = src[1] + offset;
	d = dst[1] + offset;
	s2 = src[2] + offset;
	d2 = dst[2] + offset;
	for(i = 0;i < 8;++i)
	{
		memcpy(d, s, 8); memcpy(d2, s2, 8);
		s += w; d += w;
		s2 += w; d2 += w;
	}
}
