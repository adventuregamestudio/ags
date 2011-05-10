#ifndef DISABLE_MPEG_AUDIO

/* GPL clean */

#include <ctype.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <time.h>
#else
#include <sys/time.h>
#endif

#include <fcntl.h>

#include "mpg123.h"
#include "common.h"

int tabsel_123[2][3][16] = {
   { {0,32,64,96,128,160,192,224,256,288,320,352,384,416,448,},
     {0,32,48,56, 64, 80, 96,112,128,160,192,224,256,320,384,},
     {0,32,40,48, 56, 64, 80, 96,112,128,160,192,224,256,320,} },

   { {0,32,48,56,64,80,96,112,128,144,160,176,192,224,256,},
     {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,},
     {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,} }
};

long freqs[10] = { 44100, 48000, 32000, 22050, 24000, 16000, 11025, 12000, 8000, 0 };

static int decode_header(struct frame *fr,unsigned long newhead);

void read_frame_init (struct frame *fr)
{
    fr->firsthead = 0;
    fr->thishead = 0;
    fr->freeformatsize = 0;
}

static int head_check(unsigned long head)
{
	if((head & 0xffe00000) != 0xffe00000)
		return FALSE;
	if(!((head>>17)&3))
		return FALSE;
	if( ((head>>12)&0xf) == 0xf)
		return FALSE;
	if( ((head>>10)&0x3) == 0x3 )
		return FALSE;

	return TRUE;
}

int almpa_head_backcheck(unsigned long head)
{
	head = (((head&0xFF)<<24) | ((head&0xFF00)<<8) | ((head&0xFF0000)>>8) |
	        (head>>24));

	if((head & 0xffe00000) != 0xffe00000)
		return FALSE;
	if(!((head>>17)&3))
		return FALSE;
	if( ((head>>12)&0xf) == 0xf)
		return FALSE;
	if( ((head>>10)&0x3) == 0x3 )
		return FALSE;

	return TRUE;
}


/*****************************************************************
 * read next frame
 */
int read_frame(APEG_LAYER *layer, struct frame *fr)
{
	struct mpstr *mp = &(layer->audio.mp);
    unsigned long newhead, oldhead;
	int ret;

    oldhead = fr->thishead;

	almpa_head_read(layer, &newhead);
	while(!decode_header(fr,newhead))
	{
		if(newhead == ISO_END_CODE)
			return ALMPA_EOF;

		almpa_head_shift(layer, &newhead);
	}

	layer->audio.inited = FALSE;
    if(oldhead)
	{
        if((oldhead & 0xc00) == (fr->thishead & 0xc00))
		{
			if( (oldhead & 0xc0) == 0 && (fr->thishead & 0xc0) == 0)
				layer->audio.inited = TRUE;
			else if( (oldhead & 0xc0) > 0 && (fr->thishead & 0xc0) > 0)
				layer->audio.inited = TRUE;
		}
	}

    if(!fr->bitrate_index)
       fr->framesize = fr->freeformatsize + fr->padsize;

//	fprintf(stderr,"Reading %d\n",fr->framesize);

    /* flip/init buffer for Layer 3 */
    /* FIXME for reentrance */
    mp->bsbufold = mp->bsbuf;
    mp->bsbufold_end = mp->bsbufend[mp->bsnum];
    mp->bsbuf = mp->bsspace[mp->bsnum]+512;
    mp->bsnum ^= 1;//(bsnum + 1) & 1;
    mp->bsbufend[mp->bsnum] = fr->framesize;

    /* read main data into memory */
	ret = almpa_read_bytes(layer, mp->bsbuf, fr->framesize);
	if(ret < fr->framesize)
		memset(mp->bsbuf+ret, 0, fr->framesize-ret);

    /* Test */
    if(!fr->vbr) {
        fr->vbr = getVBRHeader(&(fr->vbr_header), mp->bsbuf, fr);
    }

    mp->bsi.bitindex = 0;
    mp->bsi.wordpointer = (unsigned char*)mp->bsbuf;

	/* skip crc, we are byte aligned here */
	if(fr->error_protection)
		mp->bsi.wordpointer += 2;

	return ALMPA_OK;
}

/*
 * decode a header and write the information
 * into the frame structure
 */
static int decode_header(struct frame *fr,unsigned long newhead)
{
    if(!head_check(newhead)) {
//        fprintf(stderr,"Oops header is wrong %08lx\n",newhead);
		return 0;
    }

    if( newhead & (1<<20) ) {
		fr->lsf = (newhead & (1<<19)) ? 0x0 : 0x1;
		fr->mpeg25 = 0;
    }
    else {
		fr->lsf = 1;
		fr->mpeg25 = 1;
    }

    /* 
     * CHECKME: should be add more consistency checks here ?  
     * changed layer, changed CRC bit, changed sampling frequency 
     */
    {
	fr->lay = 4-((newhead>>17)&3);
	if( ((newhead>>10)&0x3) == 0x3) {
	    fprintf(stderr,"Stream error\n");
	    return 0;
	}
	if(fr->mpeg25) {
	    fr->sampling_frequency = 6 + ((newhead>>10)&0x3);
	}
	else
	    fr->sampling_frequency = ((newhead>>10)&0x3) + (fr->lsf*3);
		fr->error_protection = ((newhead>>16)&0x1)^0x1;
    }

    fr->bitrate_index = ((newhead>>12)&0xf);
    fr->padding   = ((newhead>>9)&0x1);
    fr->extension = ((newhead>>8)&0x1);
    fr->mode      = ((newhead>>6)&0x3);
    fr->mode_ext  = ((newhead>>4)&0x3);
    fr->copyright = ((newhead>>3)&0x1);
    fr->original  = ((newhead>>2)&0x1);
    fr->emphasis  = newhead & 0x3;

    fr->stereo    = (fr->mode == MPG_MD_MONO) ? 1 : 2;

    switch(fr->lay) {
    case 1:
        fr->framesize  = (long) tabsel_123[fr->lsf][0][fr->bitrate_index] * 12000;
        fr->framesize /= freqs[fr->sampling_frequency];
        fr->framesize  = ((fr->framesize+fr->padding)<<2)-4;
        fr->sideInfoSize = 0;
        fr->padsize = fr->padding << 2;
        break;
    case 2:
        fr->framesize = (long) tabsel_123[fr->lsf][1][fr->bitrate_index] * 144000;
        fr->framesize /= freqs[fr->sampling_frequency];
        fr->framesize += fr->padding - 4;
        fr->sideInfoSize = 0;
        fr->padsize = fr->padding;
        break;
    case 3:
        if(fr->lsf)
	    fr->sideInfoSize = (fr->stereo == 1) ? 9 : 17;
        else
	    fr->sideInfoSize = (fr->stereo == 1) ? 17 : 32;
        if(fr->error_protection)
			fr->sideInfoSize += 2;
        fr->framesize  = (long) tabsel_123[fr->lsf][2][fr->bitrate_index] * 144000;
        fr->framesize /= freqs[fr->sampling_frequency]<<(fr->lsf);
        fr->framesize = fr->framesize + fr->padding - 4;
        fr->padsize = fr->padding;
        break; 
    default:
        fprintf(stderr,"Sorry, unknown layer type.\n"); 
        return (0);
    }

    if(!fr->bitrate_index) {
        /* fprintf(stderr,"Warning, Free format not heavily tested: (head %08lx)\n",newhead); */
        fr->framesize = 0;
    }
    fr->thishead = newhead;

    return 1;
}

#if 0
void print_rheader(struct frame *fr)
{
    static char *modes[4] = { "Stereo", "Joint-Stereo", "Dual-Channel", "Single-Channel" };
    static char *layers[4] = { "Unknown" , "I", "II", "III" };
    static char *mpeg_type[2] = { "1.0" , "2.0" };

    /* version, layer, freq, mode, channels, bitrate, BPF */
    fprintf(stderr,"@I %s %s %ld %s %d %d %d\n",
	    mpeg_type[fr->lsf],layers[fr->lay],freqs[fr->sampling_frequency],
	    modes[fr->mode],fr->stereo,
	    tabsel_123[fr->lsf][fr->lay-1][fr->bitrate_index],
	    fr->framesize+4);
}

void print_header(struct frame *fr)
{
    static char *modes[4] = { "Stereo", "Joint-Stereo", "Dual-Channel", "Single-Channel" };
    static char *layers[4] = { "Unknown" , "I", "II", "III" };

    fprintf(stderr,"MPEG %s, Layer: %s, Freq: %ld, mode: %s, modext: %d, BPF : %d\n", 
	    fr->mpeg25 ? "2.5" : (fr->lsf ? "2.0" : "1.0"),
	    layers[fr->lay],freqs[fr->sampling_frequency],
	    modes[fr->mode],fr->mode_ext,fr->framesize+4);
    fprintf(stderr,"Channels: %d, copyright: %s, original: %s, CRC: %s, emphasis: %d.\n",
	    fr->stereo,fr->copyright?"Yes":"No",
	    fr->original?"Yes":"No",fr->error_protection?"Yes":"No",
	    fr->emphasis);
    fprintf(stderr,"%sBitrate: %d Kbits/s, Extension value: %d\n",
	    fr->vbr ? "Average " : "",
	    fr->vbr ? 
	    (int) (fr->vbr_header.bytes * 8 / (compute_tpf(fr) * fr->vbr_header.frames * 1000)):
	    (tabsel_123[fr->lsf][fr->lay-1][fr->bitrate_index]),
	    fr->extension);
}

void print_header_compact(struct frame *fr)
{
    static char *modes[4] = { "stereo", "joint-stereo", "dual-channel", "mono" };
    static char *layers[4] = { "Unknown" , "I", "II", "III" };
 
    fprintf(stderr,"MPEG %s layer %s, %d kbit/s, %ld Hz %s\n",
	    fr->mpeg25 ? "2.5" : (fr->lsf ? "2.0" : "1.0"),
	    layers[fr->lay],
	    tabsel_123[fr->lsf][fr->lay-1][fr->bitrate_index],
	    freqs[fr->sampling_frequency], modes[fr->mode]);
}

void print_id3_tag(unsigned char *buf)
{
    struct id3tag {
	char tag[3];
	char title[30];
	char artist[30];
	char album[30];
	char year[4];
	char comment[30];
	unsigned char genre;
    };
    struct id3tag *tag = (struct id3tag *) buf;
    char title[31]={0,};
    char artist[31]={0,};
    char album[31]={0,};
    char year[5]={0,};
    char comment[31]={0,};
    char genre[31]={0,};

    if(1)
	return;

    strncpy(title,tag->title,30);
    strncpy(artist,tag->artist,30);
    strncpy(album,tag->album,30);
    strncpy(year,tag->year,4);
    strncpy(comment,tag->comment,30);

    if ( tag->genre < sizeof(genre_table)/sizeof(*genre_table) ) {
	strncpy(genre, genre_table[tag->genre], 30);
    } else {
	strncpy(genre,"Unknown",30);
    }
	
    fprintf(stderr,"Title  : %-30s  Artist: %s\n",title,artist);
    fprintf(stderr,"Album  : %-30s  Year  : %4s\n",album,year);
    fprintf(stderr,"Comment: %-30s  Genre : %s\n",comment,genre);
}
#endif

void set_pointer(struct mpstr *mp, int ssize, long backstep)
{
    mp->bsi.wordpointer = mp->bsbuf + ssize - backstep;
    if(backstep)
		memcpy(mp->bsi.wordpointer, mp->bsbufold+mp->bsbufold_end-backstep, backstep);
    mp->bsi.bitindex = 0; 
}

/********************************/

double compute_bpf(struct frame *fr)
{
    double bpf;

    if(!fr->bitrate_index) {
		return fr->freeformatsize + 4;
    }

    switch(fr->lay) {
    case 1:
	bpf = tabsel_123[fr->lsf][0][fr->bitrate_index];
	bpf *= 12000.0 * 4.0;
	bpf /= freqs[fr->sampling_frequency] <<(fr->lsf);
	break;
    case 2:
    case 3:
	bpf = tabsel_123[fr->lsf][fr->lay-1][fr->bitrate_index];
        bpf *= 144000;
	bpf /= freqs[fr->sampling_frequency] << (fr->lsf);
	break;
    default:
	bpf = 1.0;
    }

    return bpf;
}

double compute_tpf(struct frame *fr)
{
    static int bs[4] = { 0,384,1152,1152 };
    double tpf;

    tpf = (double) bs[fr->lay];
    tpf /= freqs[fr->sampling_frequency] << (fr->lsf);
    return tpf;
}

/*int get_songlen(st ruct reader *rds, struct frame *fr, int no)
{
	double tpf;
	
	if(!fr)
		return 0;
	
	if(no < 0)
	{
		if(!rds || rds->filelen < 0)
			return 0;
		no = (double) rds->filelen / compute_bpf(fr);
	}

	tpf = compute_tpf(fr);
	return no*tpf;
}*/
#endif
