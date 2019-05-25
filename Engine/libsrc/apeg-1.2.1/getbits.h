#ifndef DISABLE_MPEG_AUDIO

/* that's the same file as mpg123's getits.c but with defines to
   force inlining */

/*static unsigned long rval;
#define backbits(bitbuf,nob) ((void)( \
  (*(bitbuf)).bitindex    -= (nob), \
  (*(bitbuf)).wordpointer += ((*(bitbuf)).bitindex>>3), \
  (*(bitbuf)).bitindex    &= 0x7 ))
#define getbitoffset(bitbuf) ((-(*(bitbuf)).bitindex)&0x7)
#define getbyte(bitbuf)      (*(*(bitbuf)).wordpointer++)
#define getbits(bitbuf,nob) ( \
  rval = (*(bitbuf)).wordpointer[0], rval <<= 8, rval |= (*(bitbuf)).wordpointer[1], \
  rval <<= 8, rval |= (*(bitbuf)).wordpointer[2], rval <<= (*(bitbuf)).bitindex, \
  rval &= 0xffffff, (*(bitbuf)).bitindex += (nob), \
  rval >>= (24-(nob)), (*(bitbuf)).wordpointer += ((*(bitbuf)).bitindex>>3), \
  (*(bitbuf)).bitindex &= 7,rval)
#define getbits_fast(bitbuf,nob) ( \
  rval = (unsigned char) ((*(bitbuf)).wordpointer[0] << (*(bitbuf)).bitindex), \
  rval |= ((unsigned long) (*(bitbuf)).wordpointer[1]<<(*(bitbuf)).bitindex)>>8, \
  rval <<= (nob), rval >>= 8, \
  (*(bitbuf)).bitindex += (nob), (*(bitbuf)).wordpointer += ((*(bitbuf)).bitindex>>3), \
  (*(bitbuf)).bitindex &= 7, rval )*/

/* Actually, static INLINEs should work fine. Granted, C code in
   a header files is bad, but it does allow real multi-source
   inlining, and the compiler should be smart enough to keep quiet
   about it when certain files don't use some functions */
static INLINE unsigned char get1bit(struct bitstream_info *bitbuf)
{
	unsigned char rval_uc = bitbuf->wordpointer[0] << bitbuf->bitindex;
	bitbuf->bitindex++;
	bitbuf->wordpointer += (bitbuf->bitindex>>3);
	bitbuf->bitindex &= 7;

	return rval_uc >> 7;
}


static INLINE void backbits(struct bitstream_info *bitbuf, int nob)
{
	bitbuf->bitindex    -= nob;
	bitbuf->wordpointer += bitbuf->bitindex >> 3;
	bitbuf->bitindex    &= 0x7;
}

static INLINE int getbitoffset(struct bitstream_info *bitbuf)
{
	return (-bitbuf->bitindex) & 0x7;
}
static INLINE unsigned char getbyte(struct bitstream_info *bitbuf)
{
	return *(bitbuf->wordpointer++);
}

static INLINE unsigned long getbits(struct bitstream_info *bitbuf, int nob)
{
	unsigned long rval = bitbuf->wordpointer[0];
	rval = (rval << 8) | bitbuf->wordpointer[1];
	rval = (rval << 8) | bitbuf->wordpointer[2];
	rval = (rval << bitbuf->bitindex) & 0xffffff;
	rval >>= (24-nob);

	bitbuf->bitindex += nob;

	bitbuf->wordpointer += (bitbuf->bitindex >> 3);
	bitbuf->bitindex &= 7;

	return rval;
}

static INLINE unsigned long getbits_fast(struct bitstream_info *bitbuf, int nob)
{
	unsigned long rval = (bitbuf->wordpointer[0] << bitbuf->bitindex) & 0xFF;
	rval |= (unsigned long)(bitbuf->wordpointer[1] << bitbuf->bitindex) >> 8;
	rval <<= nob;
	rval >>= 8;

	bitbuf->bitindex += nob;
	bitbuf->wordpointer += bitbuf->bitindex >> 3;
	bitbuf->bitindex &= 7;

	return rval;
}

#endif
