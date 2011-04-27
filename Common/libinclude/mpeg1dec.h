/* mpeg1dec.h, internal defines */
#ifndef MPEG1DEC_H
#define MPEG1DEC_H

#include <setjmp.h>

#include "apeg.h"
#include "mpg123.h"

#define PICTURE_START_CODE      0x100

#define SLICE_START_CODE_MIN    0x101
#define SLICE_START_CODE_MAX    0x1AF

#define SEQUENCE_HEADER_CODE    0x1B3
#define SEQUENCE_END_CODE       0x1B7
#define GROUP_START_CODE        0x1B8

#define SYSTEM_START_CODE_MIN	0x1B9
#define ISO_END_CODE			0x1B9
#define PACK_START_CODE			0x1BA
#define SYSTEM_START_CODE		0x1BB
#define AUDIO_ELEMENTARY_STREAM	0x1C0
#define VIDEO_ELEMENTARY_STREAM	0x1E0
#define SYSTEM_START_CODE_MAX	0x1FF

/* picture coding type */
#define I_TYPE 1
#define P_TYPE 2
#define B_TYPE 3
#define D_TYPE 4

/* macroblock type */
#define MACROBLOCK_INTRA                        1
#define MACROBLOCK_PATTERN                      2
#define MACROBLOCK_MOTION_BACKWARD              4
#define MACROBLOCK_MOTION_FORWARD               8
#define MACROBLOCK_QUANT                        16

/* Frame reconstruction quality options */
#define RECON_SUBPIXEL		1
#define RECON_AVG_SUBPIXEL	2


struct APEG_LAYER;
struct APEG_STREAM;

extern int _apeg_ignore_video;

/* prototypes of global functions */

/* getbits.c */
unsigned int apeg_get_bits(struct APEG_LAYER*, int);
unsigned int apeg_get_bits1(struct APEG_LAYER*);
unsigned int apeg_get_bits8(struct APEG_LAYER*, int);
unsigned int apeg_get_bits32(struct APEG_LAYER*);
void apeg_flush_bits(struct APEG_LAYER*, int);
void apeg_flush_bits1(struct APEG_LAYER*);
void apeg_flush_bits8(struct APEG_LAYER*, int);
void apeg_flush_bits32(struct APEG_LAYER*);

#define show_bits(l, n)		((l)->Bfr >> (32-(n)))
#define show_bits32(l)		((l)->Bfr)
#define MASK_BITS(n)		((1<<(n))-1)
#define marker_bit(l, t)	apeg_flush_bits1(l)

/* getblk.c */
void apeg_decode_intra_blocks(struct APEG_LAYER*, int*);
void apeg_decode_non_intra_block(struct APEG_LAYER*, int);

/* gethdr.c */
int apeg_get_header(struct APEG_LAYER*);
int apeg_start_code(struct APEG_LAYER*);

/* getpic.c */
unsigned char **apeg_get_frame(struct APEG_LAYER*);

/* idct.c */
void apeg_fast_idct(short*);

/* motion.c */
void apeg_motion_vector(struct APEG_LAYER*, int*, int, int);

/* mpeg2dec.c */
void apeg_error_jump(char*);
void _apeg_initialize_buffer(struct APEG_LAYER*);

/* recon.c */
void apeg_form_f_pred(struct APEG_LAYER*, int, int, int*);
void apeg_form_b_pred(struct APEG_LAYER*, int, int, int*);
void apeg_form_fb_pred(struct APEG_LAYER*, int, int, int*);
void apeg_empty_pred(unsigned char**, unsigned char**, int, int, int);

/* display.c */
void apeg_initialize_display(struct APEG_LAYER*, int);
void apeg_display_frame(struct APEG_LAYER*, unsigned char**);

/* ogg.c */
int alvorbis_update(struct APEG_LAYER*);
unsigned char **altheora_get_frame(struct APEG_LAYER*);
int alogg_open(struct APEG_LAYER*);
int alogg_reopen(struct APEG_LAYER*);
void alogg_cleanup(struct APEG_LAYER*);
void alvorbis_close(struct APEG_LAYER*);

/* global variables */
extern int _apeg_skip_length;

/* zig-zag and alternate scan patterns */
extern const unsigned char apeg_scan[64];

/* buffers for multiuse purposes */
extern short apeg_block[6][64];

extern char apeg_error[256];


// layer specific variables
typedef struct APEG_LAYER {
	struct APEG_STREAM stream;

	// bit input
	PACKFILE *pf;

	void *ogg_info;

	// from mpeg2play
	unsigned int Bfr;
	unsigned int Rdmax;
	int Incnt;

	// sequence header
	int intra_quantizer_matrix[64];
	int non_intra_quantizer_matrix[64];

	// slice/macroblock
	int quantizer_scale;

	// System layer type
	enum {
		NO_SYSTEM = 0,
		MPEG_SYSTEM,
		OGG_SYSTEM
	} system_stream_flag;

	// pointer to non-interlaced YCrCb picture buffers
	unsigned char *image_ptr;

	// pointers derived from the picture buffer
	unsigned char *backward_frame[3];
	unsigned char *forward_frame[3];
	unsigned char *auxframe[3];

	// current MPEG buffer
	unsigned char *current_frame[3];

	// current buffer to be converted
	unsigned char **picture;

	// if we got the last MPEG frame
	int got_last;

	// non-normative variables derived from normative elements
	int coded_width;
	int coded_height;
	int chroma_width;
	int chroma_height;

	// normative derived variables (as per ISO/IEC 13818-2)
	int mb_cols;
	int mb_rows;

	// headers
	int picture_type;

	// display callback
	void (*display_frame)(APEG_STREAM*, unsigned char**, void*);
	void *display_arg;

	// filename and buffer info
	char *fname;
	int buffer_size;
	enum {
		DISK_BUFFER = 0,
		MEMORY_BUFFER,
		USER_BUFFER
	} buffer_type;

	struct {
		unsigned char *buf;
		unsigned int bytes_left;
	} mem_data;

	struct {
		void (*skip)(int bytes, void *ptr);
		int (*request)(void *bfr, int bytes, void *ptr);
		int (*init)(void *ptr);
		void *ptr;
		int eof;
	} ext_data;

	int quality;

	float multiple;

	struct {
#ifndef DISABLE_MPEG_AUDIO
		struct reader rd;
		struct frame fr;
		struct mpstr mp;
		int frame;
#endif

		int (*callback_init)(APEG_STREAM*, int*, int*, void*);
		int (*callback)(APEG_STREAM*, void*, int, void*);
		void *callback_arg;

		int buf_segment;
		int voice;
		SAMPLE *stream;
		int bufsize;

		int samples_per_update;
		int last_pos;
		int pos;

		struct {
			byte *samples;
			int point;
		} pcm;

		int inited;
	} audio;
} APEG_LAYER;

#endif	// MPEG1DEC_H
