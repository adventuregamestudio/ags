#include <stdlib.h>
#include <stdio.h>

#include "mpg123.h"
#include "mpglib.h"


volatile int init = 0;


int InitMP3(struct mpstr *mp)
{
    memset(mp,0,sizeof(struct mpstr));

    mp->framesize = 0;
    mp->fsizeold = -1;
    mp->bsize = 0;
    mp->head = mp->tail = NULL;
    mp->fr.single = -1;
    mp->bsnum = 0;
    mp->synth_bo = 1;

    if (!init) {
        make_decode_tables(32767);
        init_layer3(SBLIMIT);

        mp->fr.II_sblimit=SBLIMIT;
        init_layer2();

        init = 1;
    }

    return !0;
}

void ExitMP3(struct mpstr *mp)
{
    struct buf *b,*bn;

    b = mp->tail;
    while(b) {
        free(b->pnt);
        bn = b->next;
        free(b);
        b = bn;
    }
}

static struct buf *addbuf(struct mpstr *mp,char *buf,int size)
{
    struct buf *nbuf;

    nbuf = (struct buf*) malloc( sizeof(struct buf) );
    if(!nbuf) {
        return NULL;
    }
    nbuf->pnt = (unsigned char*) malloc(size);
    if(!nbuf->pnt) {
        free(nbuf);
        return NULL;
    }
    nbuf->size = size;
    memcpy(nbuf->pnt,buf,size);
    nbuf->next = NULL;
    nbuf->prev = mp->head;
    nbuf->pos = 0;

    if(!mp->tail) {
        mp->tail = nbuf;
    }
    else {
        mp->head->next = nbuf;
    }

    mp->head = nbuf;
    mp->bsize += size;

    return nbuf;
}

static void remove_buf(struct mpstr *mp)
{
    struct buf *buf = mp->tail;

    mp->tail = buf->next;
    if(mp->tail)
        mp->tail->prev = NULL;
    else {
        mp->tail = mp->head = NULL;
    }

    free(buf->pnt);
    free(buf);

}

static int read_buf_byte(struct mpstr *mp)
{
    unsigned int b;

    int pos;

    pos = mp->tail->pos;
    while(pos >= mp->tail->size) {
        remove_buf(mp);
        pos = mp->tail->pos;
        if(!mp->tail) {
            exit(1);
        }
    }

    b = mp->tail->pnt[pos];
    mp->bsize--;
    mp->tail->pos++;


    return b;
}

static void read_head(struct mpstr *mp)
{
    unsigned long head;

    head = read_buf_byte(mp);
    head <<= 8;
    head |= read_buf_byte(mp);
    head <<= 8;
    head |= read_buf_byte(mp);
    head <<= 8;
    head |= read_buf_byte(mp);

    mp->header = head;
}

int decodeMP3(struct mpstr *mp, char *in, int isize,
              char *out, int osize, int *done)
{
    int len;

    if(osize < 4608) {
        return MP3_ERR;
    }

    if(in) {
        if(addbuf(mp, in, isize) == NULL) {
            return MP3_ERR;
        }
    }


    /* First decode header */
    if(mp->framesize == 0) {
        if(mp->bsize < 4) {
            return MP3_NEED_MORE;
        }
        read_head(mp);
        if (!head_check(mp->header))
            return MP3_ERR;
        if (decode_header(&mp->fr,mp->header) <= 0)
            return MP3_ERR;

        mp->framesize = mp->fr.framesize;
    }

    if(mp->fr.framesize > mp->bsize) {
        return MP3_NEED_MORE;
    }
    wordpointer = mp->bsspace[mp->bsnum] + 512;
    mp->bsnum = (mp->bsnum + 1) & 0x1;
    bitindex = 0;

    len = 0;
    while(len < mp->framesize) {
        int nlen;
        int blen = mp->tail->size - mp->tail->pos;
        if( (mp->framesize - len) <= blen) {
            nlen = mp->framesize-len;
        }
        else {
            nlen = blen;
        }
        memcpy(wordpointer+len,mp->tail->pnt+mp->tail->pos,nlen);
        len += nlen;
        mp->tail->pos += nlen;
        mp->bsize -= nlen;
        if(mp->tail->pos == mp->tail->size) {
            remove_buf(mp);
        }
    }

    *done = 0;
    if (mp->fr.error_protection)
        getbits(16);

    if ((&mp->fr)->do_layer(mp, &mp->fr, (unsigned char*) out, done) < 0)
        return MP3_ERR;

    mp->fsizeold = mp->framesize;
    mp->framesize = 0;
    return MP3_OK;
}

int almp3_set_pointer(void *mp, long backstep)
{
    struct mpstr *gmp = mp;

    unsigned char *bsbufold;
    if(gmp->fsizeold < 0 && backstep > 0) {
        return MP3_ERR;
    }
    bsbufold = gmp->bsspace[gmp->bsnum] + 512;
    wordpointer -= backstep;
    if (backstep)
        memcpy(wordpointer,bsbufold+gmp->fsizeold-backstep,backstep);
    bitindex = 0;
    return MP3_OK;
}
