#include        <stdio.h>
#include        <string.h>
#include        <signal.h>

#if 0
#include        <sys/signal.h>
#include        <unistd.h>
#endif

#include        <math.h>

#if 0
# undef WIN32
# define WIN32

# define REAL_IS_FLOAT
# define NEW_DCT9

# define random rand
# define srandom srand

#endif


#ifndef M_PI
# define M_PI       3.14159265358979323846
#endif

#ifndef M_SQRT2
# define M_SQRT2	1.41421356237309504880
#endif


#ifdef REAL_IS_FLOAT
#  define real float
#elif defined(REAL_IS_LONG_DOUBLE)
#  define real long double
#else
#  define real double
#endif


/* AUDIOBUFSIZE = n*64 with n=1,2,3 ...  */
#define		AUDIOBUFSIZE		16384

#ifndef FALSE
#define         FALSE                   0
#endif
#ifndef TRUE
#define         TRUE                    1
#endif

#define         SBLIMIT                 32
#define         SSLIMIT                 18

#define         SCALE_BLOCK             12 /* Layer 2 */

#define         MPG_MD_STEREO           0
#define         MPG_MD_JOINT_STEREO     1
#define         MPG_MD_DUAL_CHANNEL     2
#define         MPG_MD_MONO             3

#define MAXFRAMESIZE 1792


/* Pre Shift fo 16 to 8 bit converter table */
#define AUSHIFT (3)





struct frame {
    int stereo;
    int jsbound;
    int single;
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
    int framesize;  /* computed framesize */
    int II_sblimit; /* Layer 2 */
    struct al_table *alloc; /* Layer 2 */
    int (*do_layer)(struct frame *fr,unsigned char *, int *);/* Layer 2 */
};

/* extern unsigned int   get1bit(void);*/
extern unsigned int getbits(int);
extern unsigned int getbits_fast(int);
extern int almp3_set_pointer(long);

extern unsigned char *wordpointer;
extern int bitindex;

extern int do_layer3(struct frame *fr,unsigned char *,int *);
extern int do_layer2(struct frame *fr,unsigned char *,int *);

extern int decode_header(struct frame *fr,unsigned long newhead);
extern int head_check(unsigned long head);

struct gr_info_s {
      int scfsi;
      unsigned part2_3_length;
      unsigned big_values;
      unsigned scalefac_compress;
      unsigned block_type;
      unsigned mixed_block_flag;
      unsigned table_select[3];
      unsigned subblock_gain[3];
      unsigned maxband[3];
      unsigned maxbandl;
      unsigned maxb;
      unsigned region1start;
      unsigned region2start;
      unsigned preflag;
      unsigned scalefac_scale;
      unsigned count1table_select;
      real *full_gain[3];
      real *pow2gain;
};

struct III_sideinfo
{
  unsigned main_data_begin;
  unsigned private_bits;
  struct {
    struct gr_info_s gr[2];
  } ch[2];
};

extern int synth_1to1 (real *,int,unsigned char *,int *);
extern int tsynth_1to1 (real *,int,unsigned char *,int *);
extern int synth_1to1_mono (real *,unsigned char *,int *);

extern void init_layer3(int);
extern void init_layer2(void);
extern void make_decode_tables(long scale);
extern void dct64(real *,real *,real *);

extern long almp3freqs[9];
extern int almp3tabsel_123[2][3][16];
extern real muls[27][64];
extern real decwin[512+32];
extern real *pnts[5];
