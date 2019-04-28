/*
 * mpg123 defines 
 * used source: musicout.h from mpegaudio package
 */
#ifndef MPG123_H
#define MPG123_H

struct APEG_STREAM;
struct APEG_LAYER;

typedef unsigned char byte;
extern int _apeg_ignore_audio;

int alvorbis_init_streaming(struct APEG_LAYER*);
int alvorbis_stop_streaming(struct APEG_LAYER*);
int alvorbis_update(struct APEG_LAYER*);

int _apeg_audio_poll(struct APEG_LAYER*);
int _apeg_audio_get_position(struct APEG_LAYER*);
void _apeg_audio_set_speed_multiple(struct APEG_LAYER*, float);
int _apeg_audio_close(struct APEG_LAYER*);
int _apeg_audio_flush(struct APEG_LAYER*);

int _apeg_audio_reset_parameters(struct APEG_LAYER*);
int _apeg_start_audio(struct APEG_LAYER*, int);

#ifndef DISABLE_MPEG_AUDIO

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <allegro.h>

#define ALMPA_OK	0
#define ALMPA_ERROR	(-1)
#define ALMPA_EOF	(-2)


#ifndef M_PI
#define M_PI	3.14159265358979
#endif

#ifndef M_SQRT2
#define M_SQRT2	1.41421356237
#endif


#define SBLIMIT			32
#define SCALE_BLOCK		12
#define SSLIMIT			18

#define MPG_MD_STEREO		0
#define MPG_MD_JOINT_STEREO	1
#define MPG_MD_DUAL_CHANNEL	2
#define MPG_MD_MONO			3

#define MAXFRAMESIZE	4096


int almpa_head_backcheck(unsigned long);

struct gr_info_s {
      int scfsi;
      unsigned int part2_3_length;
      unsigned int big_values;
      unsigned int scalefac_compress;
      unsigned int block_type;
      unsigned int mixed_block_flag;
      unsigned int table_select[3];
      unsigned int subblock_gain[3];
      unsigned int maxband[3];
      unsigned int maxbandl;
      unsigned int maxb;
      unsigned int region1start;
      unsigned int region2start;
      unsigned int preflag;
      unsigned int scalefac_scale;
      unsigned int count1table_select;
      float *full_gain[3];
      float *pow2gain;
};

struct III_sideinfo
{
	unsigned int main_data_begin;
	unsigned int private_bits;
	struct {
		struct gr_info_s gr[2];
	} ch[2];
};

struct al_table 
{
	short bits;
	short d;
};

extern int tabsel_123[2][3][16];

#define VBR_FRAMES_FLAG	0x0001
#define VBR_BYTES_FLAG	0x0002
#define VBR_TOC_FLAG	0x0004
#define VBR_SCALE_FLAG	0x0008

struct vbrHeader {
	unsigned long flags;
	unsigned long frames;
	unsigned long bytes;
	unsigned long scale;
	unsigned char toc[100];
};

struct frame {
    struct al_table *alloc;
    int stereo;
    int jsbound;
    int II_sblimit;
    int down_sample_sblimit;
    int lsf;
    int mpeg25;
    int header_change;
    int lay;
    int error_protection;
    int bitrate_index;
    int sampling_frequency;
    int padding;
    int extension;
    int mode;
    int mode_ext;
    int copyright;
    int original;
    int emphasis;
    int framesize; /* computed framesize */
    int padsize;   /* */

    int sideInfoSize; /* Layer3 sideInfo Header size */

	int vbr;
	struct vbrHeader vbr_header;

    /* FIXME: move this to another place */
    unsigned long firsthead;
    unsigned long thishead;
    int freeformatsize;
};

struct bitstream_info {
	int bitindex;
	unsigned char *wordpointer;
};

struct mpstr {
	int bsize;
	int framesize;
	int fsizeold;
	struct frame fr;
	unsigned char bsspace[2][MAXFRAMESIZE+512]; /* MAXFRAMESIZE */
	float hybrid_block[2][2][SBLIMIT*SSLIMIT];
	int hybrid_blc[2];
	unsigned long header;
	int bsnum;
	float synth_buffs[2][2][0x110];
	int synth_bo;

	struct bitstream_info bsi;
	int bsbufend[2];
	int bsbufold_end;
	unsigned char *bsbuf, *bsbufold;

	float buffs[2][2][0x110];
	int bo;

	float hybridIn [2][SBLIMIT][SSLIMIT];
	float hybridOut[2][SSLIMIT][SBLIMIT];
	unsigned int balloc[2*SBLIMIT];
	struct III_sideinfo sideinfo;
	int scalefacs[192]; /* max 39 for short[13][3] mode, mixed: 38, long: 22 */
	int gr, ss;
	int return_later;
};

struct reader {
	void *filept;

	unsigned int left_in_packet;
	unsigned int filepos;
	int eof;
};

void almpa_close(struct APEG_LAYER*);
int  almpa_read_bytes(struct APEG_LAYER*, byte*, unsigned int);
void almpa_head_read(struct APEG_LAYER*, unsigned long*);
void almpa_head_shift(struct APEG_LAYER*, unsigned long*);
void almpa_skip_bytes(struct APEG_LAYER*, int);


extern void print_header(struct frame *);
extern void print_header_compact(struct frame *);
extern void print_id3_tag(unsigned char *buf);

extern void set_pointer(struct mpstr*,int,long);

extern int open_stream(struct APEG_LAYER *layer);
extern void read_frame_init (struct frame *fr);
extern int read_frame(struct APEG_LAYER *layer,struct frame *fr);
extern int do_layer3(struct mpstr*, struct frame*, struct APEG_LAYER*);
extern int do_layer2(struct mpstr*, struct frame*, struct APEG_LAYER*);
extern int do_layer1(struct mpstr*, struct frame*, struct APEG_LAYER*);


extern void synth_1to1(float*,int,unsigned char*,int*,float(*)[2][0x110],int*);
extern void synth_1to1_mono(float*,unsigned char*,int*,float(*)[2][0x110],int*);

extern void synth_2to1(float*,int,unsigned char*,int*,float(*)[2][0x110],int*);
extern void synth_2to1_mono(float*,unsigned char*,int*,float(*)[2][0x110],int*);

extern void synth_4to1(float*,int,unsigned char*,int*,float(*)[2][0x110],int*);
extern void synth_4to1_mono(float*,unsigned char*,int*,float(*)[2][0x110],int*);

#define synth_mono(ds, a, b, c, d, e) \
switch((ds)) \
{ \
	case 2: \
		synth_4to1_mono((a), (b), (c), (d), (e)); \
		break; \
	case 1: \
		synth_2to1_mono((a), (b), (c), (d), (e)); \
		break; \
	case 0: \
		synth_1to1_mono((a), (b), (c), (d), (e)); \
		break; \
}

#define synth(ds, a, b, c, d, e, f) \
switch((ds)) \
{ \
	case 2: \
		synth_4to1((a), (b), (c), (d), (e), (f)); \
		break; \
	case 1: \
		synth_2to1((a), (b), (c), (d), (e), (f)); \
		break; \
	case 0: \
		synth_1to1((a), (b), (c), (d), (e), (f)); \
		break; \
}

extern void init_layer3(int);
extern void init_layer2(void);
extern void make_decode_tables();

extern void control_generic(struct mpstr *,struct frame *fr);

extern int getVBRHeader(struct vbrHeader *head,unsigned char *buf,
                        struct frame *fr);


extern long freqs[10];
extern float muls[27][64];
extern float decwin[512+32];
extern float *pnts[5];

extern float equalizer[2][32];

/* 486 optimizations */
#define FIR_BUFFER_SIZE  128
void dct64(float*,float*,float*);
void dct36(float*,float*,float*,float*,float*);


static INLINE void do_equalizer(float *bandPtr, int channel)
{
#if 0
	int i;
	for(i = 0;i < 32;++i)
		bandPtr[i] *= equalizer[channel][i];
#else
	(void)bandPtr;
	(void)channel;
#endif
}

#endif

#include "mpeg1dec.h"

static INLINE int clamp_val(int low, int val, int high)
{
	val -= low;
	val &= (~val) >> 31;
	val += low;

	val -= high;
	val &= val >> 31;
	val += high;

	return val;
}

#endif	// MPG123_H
