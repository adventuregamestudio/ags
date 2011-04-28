/* Allegro MP3 - a wrapper for mpglib from mpg123 */
/* to play MP3 files with Allegro */

/* MP3 decoder part of mpglib from mpg123 (www.mpg123.com) */
/* Allegro MP3 is copyright (c) 2001, 2002 Javier Gonz lez */

/* See COPYING.txt (GNU Lesser General Public License 2.1) for license */


#include <string.h>
#include <allegro.h>
#include <almp3.h>
#include "libsrc/almp3-2.0.5/decoder/mpg123.h"
#include "libsrc/almp3-2.0.5/decoder/mpglib.h"


/* structures */

/* struct typically used in VBR (Variable Bitrate) MP3s */

#define XING_FRAMES_FLAG            1
#define XING_BYTES_FLAG             2
#define XING_TOC_FLAG               4
#define XING_VBR_SCALE_FLAG         8

typedef struct XING_HEADER {
  int flags;
  int frames;                      /* number of frames the MP3 has */
  int bytes;                       /* number of bytes the file has */
  int vbr_scale;                   /* encoded vbr scale */
  unsigned char toc[100];          /* used for seeking */
} XING_HEADER;


/* standard ALMP3_MP3 structure */

struct ALMP3_MP3 {
  /* data info */
  void *data;                      /* mp3 data */
  char *data_cursor;               /* pointer to data being read */
  int data_len;                    /* size of the data */
  int data_start_skipped;          /* used when ID3v2 headers are present */
                                   /* and we are asked to seek or get the */
                                   /* pos in bytes */
  int data_end_skipped;            /* used when ID3v1 headers are present */
                                   /* and we are asked to seek or get the */
                                   /* pos in bytes */
  /* decoder info */
  struct mpstr mpeg;               /* decoder handler */
  XING_HEADER *xing_header;        /* xing header if available */
  int dec_reinit;                  /* true if the decoder needs to be */
                                   /* reinitialized in next poll */
  /* mp3 info */
  int outbytes_per_frame;          /* bytes created per frame */
  int framebytes;                  /* decoder first framebytes */
  int bitrate, layer;              /* mp3 general info */
  int total_frames;                /* number of frames the mp3 has */
  int current_frame;               /* frame currently being played */
  /* playing info */
  AUDIOSTREAM *audiostream;        /* the audiostream we are using to play */
                                   /* also used to know when it's playing */
  int freq, stereo, bits, loop;    /* audio general info */
  int frames_per_poll;             /* number of frames per play poll */
  int auto_polling;                /* set if the mp3 is auto polling */
  int auto_poll_speed;             /* auto poll speed in msecs */
  int wait_for_audio_stop;         /* set if we are just waiting for the
                                      audiobuffer to stop plaing the last
                                      frame */
};


/* standard ALMP3_MP3STREAM structure */

struct ALMP3_MP3STREAM {
  /* data info */
  void *databuf1, *databuf2;       /* data buffers */
  char *full_databuf;              /* to be used when the 2 buffers are filled */
  int data_cursor;                 /* pointer to data being read */
  int databuf_selector;            /* 1 = databuf1, -1 = databuf2 */
  int databuf_len;                 /* size of the data buffer */
  int unsel_databuf_free;          /* TRUE when the unselected databuffer */
                                   /* is free and needs to be filled */
  int bytes_used;                  /* number of bytes to use from the buffer */
                                   /* or -1 if the full buffer */
  int data_start_skipped;          /* used when ID3v2 headers are present */
                                   /* and we are asked to seek or get the */
                                   /* pos in bytes */
  /* decoder info */
  struct mpstr mpeg;               /* decoder handler */
  XING_HEADER *xing_header;        /* xing header if available */
  int dec_reinit;                  /* true if the decoder needs to be */
                                   /* reinitialized in next poll */
  /* mp3 info */
  int outbytes_per_frame;          /* bytes created per frame */
  int framebytes;                  /* decoder first framebytes */
  int bitrate, layer;              /* mp3 general info */
  int current_frame;               /* frame currently being played */
  int current_byte;                /* byte currently being decoded */
  int total_frames;                /* number of frames the mp3 has */
  /* playing info */
  AUDIOSTREAM *audiostream;        /* the audiostream we are using to play */
                                   /* also used to know when it's playing */
  int freq, stereo, bits;          /* audio general info */
  int frames_per_poll;             /* number of frames per play poll */
  int auto_polling;                /* set if the mp3 is auto polling */
  int auto_poll_speed;             /* auto poll speed in msecs */
  int wait_for_audio_stop;         /* set if we are just waiting for the
                                      audiobuffer to stop plaing the last
                                      frame */
};


/* common functions */


static int almp3_check_frame_validity(unsigned int header) {
  /* return 0 if invalid, -1 if valid */
  
  /* test sync */
  if ((header & 0xFFE00000) != 0xFFE00000)
    return 0;

  /* mpeg version */
  if ((header & 0x180000) == 0x80000)
    return 0;

  /* mpeg layer */
  if (((header & 0x60000) != 0x20000) && ((header & 0x60000) != 0x10000))
    return 0;

  /* bitrate */
  if (((header & 0xF000) == 0xF000) || ((header & 0xF000) == 0x0000))
    return 0;

  /* freq */
  if ((header & 0xC00) == 0xC00)
    return 0;

  /* emphasis */
/*if ((header & 0x3) == 0x2)
    return 0;*/

  return -1;
}


static int almp3_get_big_endian(unsigned char *p) {
  int x;
  x = *p; x <<= 8;
  x |= *(p + 1); x <<= 8;
  x |= *(p + 2); x <<= 8;
  x |= *(p + 3);
  return x;
}


static int almp3_get_id3v2_skip_bytes(unsigned char *p) {
  int id3v2_skip_size = 0;
  /* test for ID3v2 and if exists, skip it */
  if (memcmp((unsigned char *)p, "ID3", 3) == 0) {
    int footer_present = p[5] & 0x10;
    p += 6;
    id3v2_skip_size = *(p + 3) | (*(p + 2) << 7) | (*(p + 1) << 14) | (*p << 21);
    if (footer_present)
      id3v2_skip_size += 20;
    else
      id3v2_skip_size += 10;
  }
  else
    id3v2_skip_size = 0;

  return id3v2_skip_size;
}



/* API - MP3 */

/* function called in an interrupt */

static void almp3_autopoll_mp3(void *mp3) {
  almp3_poll_mp3((ALMP3_MP3 *)mp3);
}
END_OF_FUNCTION(almp3_autopoll_mp3);


/* helper functions */

static int almp3_reinit_decoder_mp3(ALMP3_MP3 *mp3) {
  /* reinitialize the decoder */
  ExitMP3(&(mp3->mpeg));
  InitMP3(&(mp3->mpeg));
  mp3->dec_reinit = FALSE;
  
  return 0;
}


static void almp3_convert_to_unsigned_format_mp3(ALMP3_MP3 *mp3, void *buf) {
  /* convert to allegro unsigned format */
  if (mp3->bits == 16) {
    unsigned int i, *j, k;
    k = (mp3->outbytes_per_frame * mp3->frames_per_poll) / 4;
    for (i = 0, j = (unsigned int *)buf; i < k; i++, j++)
      *j ^= 0x80008000;
  }
}


static int almp3_get_bytes_at_frame_using_toc_mp3(ALMP3_MP3 *mp3, int frame) {
  int tof[100], i, flags;
  int bytesmin, bytesmax, bytesdif, bytes;
  int framemin, framemax, framedif;

  /* check for missing data */
  if (mp3->xing_header == NULL)
    return -1;
  flags = mp3->xing_header->flags;
  if (!(flags & XING_FRAMES_FLAG) || !(flags & XING_TOC_FLAG))
    return -1;
  if ((frame >= mp3->xing_header->frames) || (frame < 0))
    return -1;

  /* create a table of frames */
  for (i = 0; i < 100; i++)
    tof[i] = (mp3->xing_header->toc[i] * mp3->xing_header->frames) / 256;
  
  /* check in what part of the table of frames is our frame */
  for (i = 0; i < 100; i++) {
    if (frame <= tof[i])
      break;
  }

  /* our frame is between the i% and (i + 1)% of the file */
  /* let's do more calcules for a "fine" seeking */
  if (i == 100)
    return mp3->data_len;
  bytesmin = (mp3->data_len * i) / 100;
  framemin = tof[i];
  if (i < 99) {
    bytesmax = (mp3->data_len * (i + 1)) / 100;
    framemax = tof[i + 1];
  }
  else {
    bytesmax = mp3->data_len - 1;
    framemax = mp3->xing_header->frames - 1;
  }
  bytesdif = bytesmax - bytesmin;
  framedif = framemax - framemin;

  bytes = bytesmin + (((frame - framemin) * bytesdif) / framedif);

  return bytes;
}


static int almp3_get_outbytes_per_sec_mp3(ALMP3_MP3 *mp3) {
  int bytes_out;
  bytes_out = (mp3->stereo ? 2 : 1) * (mp3->bits / 8) * mp3->freq;
  return bytes_out;
}


static int almp3_decode_frame_mp3(ALMP3_MP3 *mp3, char *inbuf, char *outbuf, int *size_done, int *inread) {
  /* send bytes until it has enough to render a frame */
  int ret;
  *inread = 0;

  ret = decodeMP3(&(mp3->mpeg), NULL, 0, outbuf, 8192, size_done);
  if (ret == MP3_NEED_MORE) {
    int chunk_size, data_left;

    if (mp3->xing_header)
      chunk_size = MAXFRAMESIZE;
    else
      chunk_size = mp3->framebytes + 4;

    data_left = mp3->data_len - ((char *)mp3->data_cursor - (char *)mp3->data);
    do {
      if (data_left <= 0)
        break;

      if (data_left < chunk_size) {
        ret = decodeMP3(&(mp3->mpeg), inbuf, data_left, outbuf, 8192, size_done);
        inbuf += data_left;
        *inread += data_left;
        data_left = 0;
      }
      else {
        ret = decodeMP3(&(mp3->mpeg), inbuf, chunk_size, outbuf, 8192, size_done);
        data_left -= chunk_size;
        inbuf += chunk_size;
        *inread += chunk_size;
      }
    } while (ret == MP3_NEED_MORE);
  }

  {
    int val = almp3_check_frame_validity((mp3->mpeg).header);

    if (ret == MP3_NEED_MORE)
      return -3;

    if ((ret == MP3_ERR) || (val == 0) || ((mp3->mpeg).fr.framesize <= 0))
      return 0;
    else
      return -1;
  }
}


/* API functions */

ALMP3_MP3 *almp3_create_mp3(void *data, int data_len) {
  ALMP3_MP3 *mp3;
  struct mpstr test_mpeg;
  unsigned char *p = (unsigned char *)data;
  int data_end_skipped, data_start_skipped;
  int ret;
  char test_outbuf[8192];
  int test_size, framebytes;

  /* test for ID3v1 and if exists, skip it */
  if (memcmp((void *)&p[data_len - 128], "TAG", 3) == 0) {
    data_len -= 128;
    data_end_skipped = 128;
  }
  else
    data_end_skipped = 0;

  /* test for ID3v2 and if exists, skip it */
  data_start_skipped = almp3_get_id3v2_skip_bytes(p);
  (char *)data += data_start_skipped;
  data_len -= data_start_skipped;

  /* test the next 16 * MAXFRAMESIZE bytes after ID3v2 until we find one valid frame */
  {
    int found, max, skipbytes;
    if (data_len >= (16 * MAXFRAMESIZE))
      max = 16 * MAXFRAMESIZE;
    else
      max = data_len;

    found = FALSE;
    skipbytes = 0;
    while (!found && (skipbytes < max)) {
      int data_left = max - skipbytes;
      char *p = (char *)data + skipbytes;

      if (almp3_check_frame_validity((unsigned int)almp3_get_big_endian(p)) == -1) {
        /* probably found, check */
        InitMP3(&test_mpeg);
        ret = decodeMP3(&test_mpeg, p, data_left, test_outbuf, 8192, &test_size);
        if ((ret == MP3_OK) && (test_mpeg.fr.framesize > 0)) {
          char *p2 = p + test_mpeg.fr.framesize + 4;
          if (p2 < ((char *)data + max)) {
            /* check the next frame to make sure */
            if (almp3_check_frame_validity((unsigned int)almp3_get_big_endian(p2)) != 0)
              found = TRUE;
          }
          else {
            /* guess it is right */
            found = TRUE;
          }
        }
        
        if (!found)
          ExitMP3(&test_mpeg);
      }

      if (!found)
        skipbytes++;
    }

    if (!found) {
      return NULL;
    }

    data_start_skipped += skipbytes;
    data_len -= skipbytes;
    (char *)data += skipbytes;
  }

  /* must be layer III or II */
  if ((test_mpeg.fr.lay != 3) && (test_mpeg.fr.lay != 2)) {
    ExitMP3(&test_mpeg);
    return NULL;
  }

  /* the mp3 is ok, get some info */
  framebytes = test_mpeg.fr.framesize + 4;
  if ((framebytes <= 4)) {
    ExitMP3(&test_mpeg);
    return NULL;
  }

  /* create a new mp3 struct and init the real decoder */
  mp3 = (ALMP3_MP3 *)malloc(sizeof(ALMP3_MP3));
  if (mp3 == NULL) {
    ExitMP3(&test_mpeg);
    return NULL;
  }
  InitMP3(&(mp3->mpeg));


  /* fill in the mp3 struct */
  mp3->data_start_skipped = data_start_skipped;
  mp3->data_end_skipped = data_end_skipped;
  mp3->freq = almp3freqs[test_mpeg.fr.sampling_frequency];
  if (test_mpeg.fr.stereo >= 2)
    mp3->stereo = TRUE;
  else
    mp3->stereo = FALSE;
  mp3->bits = 16;
  mp3->outbytes_per_frame = test_size;
  mp3->audiostream = NULL;
  mp3->data = data;
  mp3->data_cursor = (char *)data;
  mp3->data_len = data_len;
  mp3->current_frame = 0;
  mp3->auto_polling = FALSE;
  mp3->dec_reinit = FALSE;
  mp3->bitrate = almp3tabsel_123[test_mpeg.fr.lsf][test_mpeg.fr.lay - 1][test_mpeg.fr.bitrate_index] * 1000;
  mp3->layer = test_mpeg.fr.lay;
  if (mp3->layer <= 0)
    mp3->layer = -1;
  mp3->xing_header = NULL;
  mp3->framebytes = framebytes;

  /* if outbytes_per_frame is incorrect, correct it */
  if (mp3->outbytes_per_frame <= 0) {
    int samples = 1152;
    if (mp3->layer == 2)
      samples = 1152;
    if (mp3->layer == 3) {
      if ((mp3->freq >= 32000) && (mp3->freq < 48000))
        samples = 1152;
      else if ((mp3->freq >= 8000) && (mp3->freq < 24000))
        samples = 576;
    }
    mp3->outbytes_per_frame = samples * (mp3->stereo ? 2 : 1) * (mp3->bits / 8);
  }

  /* check for the xing header */
  p = (unsigned char *)data;
  if (!test_mpeg.fr.lsf) {             /* mpeg1 */
    if (test_mpeg.fr.mode != 3)
      p += 32 + 4;
    else
      p += 17 + 4;
  }
  else {                               /* mpeg2 */
    if (test_mpeg.fr.mode != 3)
      p += 17 + 4;
    else
      p += 9 + 4;
  }

  if (memcmp(p, "Xing", 4) == 0) {

    /* found */
    int x;
    p += 4;
    mp3->xing_header = (XING_HEADER *)malloc(sizeof(XING_HEADER));
    x = mp3->xing_header->flags = almp3_get_big_endian(p);
    p += 4;
    if (x & XING_FRAMES_FLAG) {
      mp3->xing_header->frames = almp3_get_big_endian(p);
      p += 4;
    }
    if (x & XING_BYTES_FLAG) {
      mp3->xing_header->bytes = almp3_get_big_endian(p);
      p += 4;
    }
    if (x & XING_TOC_FLAG) {
      int j;
      for (j = 0; j < 100; j++, p++)
        mp3->xing_header->toc[j] = *p;
    }
    mp3->xing_header->vbr_scale = -1;
    if (x & XING_VBR_SCALE_FLAG) {
      mp3->xing_header->vbr_scale = almp3_get_big_endian(p);
      p += 4;
    }

  }

  /* calculate the number of total frames */
  if ((mp3->xing_header != NULL) && (mp3->xing_header->flags & XING_FRAMES_FLAG))
    mp3->total_frames = mp3->xing_header->frames;
  else
    mp3->total_frames = mp3->data_len / mp3->framebytes;

  /* deinit the test mpeg */
  ExitMP3(&test_mpeg);

  return mp3;
}


void almp3_destroy_mp3(ALMP3_MP3 *mp3) {
  if (mp3 == NULL)
    return;
  
  almp3_stop_mp3(mp3);          /* note almp3_stop_mp3 will also remove */
                                /* autopolling interrupts */
  ExitMP3(&(mp3->mpeg));
  if (mp3->xing_header)
    free(mp3->xing_header);
  free(mp3);  
}


int almp3_play_mp3(ALMP3_MP3 *mp3, int buffer_len, int vol, int pan) {
  return almp3_play_ex_mp3(mp3, buffer_len, vol, pan, 1000, FALSE);
}


int almp3_play_ex_mp3(ALMP3_MP3 *mp3, int buffer_len, int vol, int pan, int speed, int loop) {
  int samples;

  /* continue only if we are not already playing it */
  if (almp3_is_playing_mp3(mp3))
    return ALMP3_OK;

  /* calculate the frames we can decode for each poll */
  /* using the given buffer_len */
  mp3->frames_per_poll = buffer_len / mp3->outbytes_per_frame;
  if (mp3->frames_per_poll <= 0)
    return ALMP3_PLAY_BUFFERTOOSMALL;
   
  /* create a new audiostream and play it */
  samples = (mp3->outbytes_per_frame * mp3->frames_per_poll) / (mp3->stereo ? 2 : 1) / (mp3->bits / 8);
  mp3->audiostream = play_audio_stream(samples, mp3->bits, mp3->stereo, mp3->freq, vol, pan);

  if (speed != 1000)
    adjust_sample(mp3->audiostream->samp, vol, pan, speed, TRUE);

  mp3->loop = loop;
  mp3->wait_for_audio_stop = 0;

  /* if the user asked for autopolling, install the interrupt now */
  if (mp3->auto_polling) {
    LOCK_FUNCTION(almp3_autopoll_mp3);
    install_param_int(&almp3_autopoll_mp3, (void *)mp3, mp3->auto_poll_speed);
  }

  return ALMP3_OK;
}


void almp3_stop_mp3(ALMP3_MP3 *mp3) {
  /* continue if we are playing it */
  if (!almp3_is_playing_mp3(mp3))
    return;

  /* remove the interrupt, if we were using autopolling */
  if (mp3->auto_polling)
    remove_param_int(&almp3_autopoll_mp3, (void *)mp3);

  /* stop the audio stream */
  stop_audio_stream(mp3->audiostream);
  mp3->audiostream = NULL;
}


void almp3_rewind_mp3(ALMP3_MP3 *mp3) {
  mp3->data_cursor = (char *)mp3->data;
  mp3->current_frame = 0;
  mp3->dec_reinit = TRUE;
}


void almp3_seek_abs_frames_mp3(ALMP3_MP3 *mp3, int frame) {
  int bytes;

  /* check if the frame is valid */
  if ((frame < 0) || (frame >= mp3->total_frames))
    return;

  /* first try using the xing header */
  bytes = almp3_get_bytes_at_frame_using_toc_mp3(mp3, frame);

  /* seems like we are using it */
  if (bytes >= 0)
    mp3->data_cursor = (char *)mp3->data + bytes;

  /* no, use normal method */
  else
    mp3->data_cursor = (char *)mp3->data + (frame * mp3->framebytes);

  mp3->current_frame = frame;
  mp3->dec_reinit = TRUE;
}


void almp3_seek_abs_msecs_mp3(ALMP3_MP3 *mp3, int msec) {
  int frame;
  frame = msec / almp3_get_msecs_per_frame_mp3(mp3);
  almp3_seek_abs_frames_mp3(mp3, frame);
}


void almp3_seek_abs_secs_mp3(ALMP3_MP3 *mp3, int sec) {
  almp3_seek_abs_msecs_mp3(mp3, sec * 1000);
}


void almp3_seek_abs_bytes_mp3(ALMP3_MP3 *mp3, int bytes) {
  /* check the pos in bytes is valid */
  /* we check against -mp3->data_start_skipped because our data buffer */
  /* skipped the very first ID3v2 header */
  if ((bytes < -mp3->data_start_skipped) || (bytes >= mp3->data_len))
    return;

  /* if the poiner wanted was inside the ID3v2 header, skip it and */
  /* point directly to the data */
  if (bytes < 0)
    bytes = 0;

  /* set the cursor to the new position */
  mp3->data_cursor = (char *)mp3->data + bytes;

  mp3->current_frame = bytes / mp3->framebytes;
  mp3->dec_reinit = TRUE;
}


void almp3_seek_rel_frames_mp3(ALMP3_MP3 *mp3, int frame) {
  int bytes;
  int wanted_frame = mp3->current_frame + frame;

  /* check the frame is valid */
  if ((wanted_frame < 0) || (wanted_frame >= mp3->total_frames))
    return;

  /* first try using the xing header */
  bytes = almp3_get_bytes_at_frame_using_toc_mp3(mp3, wanted_frame);

  /* seems like we are using it */
  if (bytes >= 0)
    mp3->data_cursor = (char *)mp3->data + bytes;

  /* no, use normal method */
  else
    mp3->data_cursor += frame * mp3->framebytes;

  mp3->current_frame = wanted_frame;
  mp3->dec_reinit = TRUE;
}


void almp3_seek_rel_msecs_mp3(ALMP3_MP3 *mp3, int msec) {
  int frame;
  frame = msec / almp3_get_msecs_per_frame_mp3(mp3);
  almp3_seek_rel_frames_mp3(mp3, frame);
}


void almp3_seek_rel_secs_mp3(ALMP3_MP3 *mp3, int sec) {
  almp3_seek_rel_msecs_mp3(mp3, sec * 1000);
}


void almp3_seek_rel_bytes_mp3(ALMP3_MP3 *mp3, int bytes) {
  int current_byte = mp3->data_cursor - (char *)mp3->data;
  int wanted_byte = current_byte + bytes;

  /* check the byte is valid */
  /* we check against -mp3->data_start_skipped because our data buffer */
  /* skipped the very first ID3v2 header */
  if ((wanted_byte < -mp3->data_start_skipped) || (wanted_byte >= mp3->data_len))
    return;

  /* if the poiner wanted was inside the ID3v2 header, skip it and */
  /* point directly to the data */
  if (wanted_byte < 0)
    wanted_byte = 0;

  /* set the cursor to the new position */
  mp3->data_cursor = (char *)mp3->data + wanted_byte;

  mp3->current_frame = wanted_byte / mp3->framebytes;
  mp3->dec_reinit = TRUE;
}


int almp3_poll_mp3(ALMP3_MP3 *mp3) {
  void *audiobuf;
  char *audiobuf_p;
  int i, num_data_cursor, size_done;

  /* continue only if we are playing it */
  if (!almp3_is_playing_mp3(mp3))
    return ALMP3_POLL_NOTPLAYING;

  /* get the audio stream buffer and only continue if we need to fill it */
  audiobuf = get_audio_stream_buffer(mp3->audiostream);
  if (audiobuf == NULL)
    return ALMP3_OK;

  /* clear the buffer */
  memset(audiobuf, 0, mp3->outbytes_per_frame * mp3->frames_per_poll);

  /* if we need to fill it, but we were just waiting for it to finish */
  if (!mp3->loop) {
    if (mp3->wait_for_audio_stop > 0) {
      almp3_convert_to_unsigned_format_mp3(mp3, audiobuf);
      free_audio_stream_buffer(mp3->audiostream);
      if (--mp3->wait_for_audio_stop == 0) {
        /* stop it */
        almp3_stop_mp3(mp3);
        return ALMP3_POLL_PLAYJUSTFINISHED;
      }
      else
        return ALMP3_OK;
    }
  }

  /* reinitialize the decoder if needed */
  if (mp3->dec_reinit) {
    mp3->wait_for_audio_stop = 0;
    if (almp3_reinit_decoder_mp3(mp3) < 0) {
      /* an error while reinitializing */
      almp3_stop_mp3(mp3);
      almp3_rewind_mp3(mp3);
      return ALMP3_POLL_INTERNALERROR;
    }
  }

  audiobuf_p = (char *)audiobuf;
  size_done = 0;
  for (i = 0; i < (mp3->outbytes_per_frame * mp3->frames_per_poll); i += size_done) {
    int ret, inread;
    /* if we have reached the end */
    num_data_cursor = mp3->data_cursor - (char *)mp3->data;
    if (num_data_cursor >= mp3->data_len) {
      almp3_rewind_mp3(mp3);
      if (!mp3->loop) {
        /* we don't want clicks at the end, so we convert our empty buffer */
        almp3_convert_to_unsigned_format_mp3(mp3, audiobuf);
        free_audio_stream_buffer(mp3->audiostream);
        mp3->wait_for_audio_stop = 2;
        return ALMP3_OK;
      }
      mp3->dec_reinit = FALSE;
    }
    
    ret = almp3_decode_frame_mp3(mp3, mp3->data_cursor, audiobuf_p, &size_done, &inread);

    /* probably it needs more */
    if ((ret == -3) && mp3->loop) {
      /* rewind and give it more */
      almp3_rewind_mp3(mp3);
      mp3->dec_reinit = FALSE;
      ret = almp3_decode_frame_mp3(mp3, mp3->data_cursor, audiobuf_p, &size_done, &inread);
    }

    /* check if the decoding was not successful */
    if (ret == 0) {
      /* damn, seems like this frame is corrupt */
      /* check the next 16kb just to be sure */
      int j, got_error = TRUE, max, left;
      max = 16 * 1024;
      
      left = mp3->data_len - num_data_cursor;
      
      if (!mp3->loop && (max > left))
        max = left;
      
      for (j = 0; (j < max) && (got_error); j++) {

        /* check if we have to check the start again */
        if (mp3->loop && (j == left)) {
          almp3_rewind_mp3(mp3);
          mp3->dec_reinit = FALSE;
        }

        /* check the header */
        if (almp3_check_frame_validity((unsigned int)almp3_get_big_endian(mp3->data_cursor)) == -1) {
          /* try to decode it */
          almp3_reinit_decoder_mp3(mp3);       
          ret = almp3_decode_frame_mp3(mp3, mp3->data_cursor, audiobuf_p, &size_done, &inread);
          if (ret == -1) {
            char *p = mp3->data_cursor + (mp3->mpeg).fr.framesize + 4;
            if (p < ((char *)mp3->data + mp3->data_len)) {
              /* check the next header too just to make sure */
              if (almp3_check_frame_validity((unsigned int)almp3_get_big_endian(p)) != 0)
                got_error = FALSE;
            }
            else {
              /* if we don't have enough data to check, assume it is */
              got_error = FALSE;
            }
          }
        }
        
        if (got_error)
          mp3->data_cursor++;
      }

      /* if we were unlucky, this is probably corrupt */
      /* (don't take it so seriously if it is the last frame) <- thanks to Bruno Dupuis */
      if (got_error && ((mp3->data_len - num_data_cursor) > (mp3->mpeg).fr.framesize)) {
        almp3_convert_to_unsigned_format_mp3(mp3, audiobuf);
        free_audio_stream_buffer(mp3->audiostream);
        almp3_stop_mp3(mp3);
        almp3_rewind_mp3(mp3);
        return ALMP3_POLL_FRAMECORRUPT;
      }
    }

    audiobuf_p += size_done;
    mp3->current_frame++;
    mp3->data_cursor += inread;
  }

  /* convert to allegro unsigned format */
  almp3_convert_to_unsigned_format_mp3(mp3, audiobuf);

  /* lock the buffer */
  free_audio_stream_buffer(mp3->audiostream);

  /* if we have reached the end */
  num_data_cursor = mp3->data_cursor - (char *)mp3->data;
  if (num_data_cursor >= mp3->data_len) {
    almp3_rewind_mp3(mp3);
    if (!mp3->loop)
      mp3->wait_for_audio_stop = 2;
    return ALMP3_OK;
  }

  return ALMP3_OK;
}


void almp3_start_autopoll_mp3(ALMP3_MP3 *mp3, int speed) {
  /* return if we are already auto polling */
  if (mp3->auto_polling)
    return;

  mp3->auto_polling = TRUE;
  mp3->auto_poll_speed = speed;

  /* only install the interrupt if we are already playing */
  /* otherwise, play will do */
  if (almp3_is_playing_mp3(mp3)) {
    LOCK_FUNCTION(almp3_autopoll_mp3);
    install_param_int(&almp3_autopoll_mp3, (void *)mp3, mp3->auto_poll_speed);
  }
}


void almp3_stop_autopoll_mp3(ALMP3_MP3 *mp3) {
  /* return if we are already not auto polling */
  if (!mp3->auto_polling)
    return;

  mp3->auto_polling = FALSE;

  /* only remove the interrupt if we were already playing */
  /* otherwise, stop will do*/
  if (almp3_is_playing_mp3(mp3))
    remove_param_int(&almp3_autopoll_mp3, (void *)mp3);
}


int almp3_get_pos_frames_mp3(ALMP3_MP3 *mp3) {
  return mp3->current_frame;
}


int almp3_get_pos_msecs_mp3(ALMP3_MP3 *mp3) {
  float second;
  second = mp3->current_frame * mp3->outbytes_per_frame;
  second /= (mp3->stereo ? 2 : 1) * (mp3->bits / 8);
  second /= (float)mp3->freq;
  return (int)(1000 * second);
}


int almp3_get_pos_secs_mp3(ALMP3_MP3 *mp3) {
  return (almp3_get_pos_msecs_mp3(mp3) / 1000);
}


int almp3_get_length_frames_mp3(ALMP3_MP3 *mp3) {
  return mp3->total_frames;
}


int almp3_get_pos_bytes_mp3(ALMP3_MP3 *mp3) {
  return (mp3->data_cursor - (char *)mp3->data) + mp3->data_start_skipped;
}


int almp3_get_length_msecs_mp3(ALMP3_MP3 *mp3) {
  int msecs;
  msecs = almp3_get_length_frames_mp3(mp3) * almp3_get_msecs_per_frame_mp3(mp3);
  return msecs;
}


int almp3_get_length_secs_mp3(ALMP3_MP3 *mp3) {
  return (almp3_get_length_msecs_mp3(mp3) / 1000);
}


int almp3_get_length_bytes_mp3(ALMP3_MP3 *mp3) {
  return (mp3->data_len + mp3->data_start_skipped + mp3->data_end_skipped);
}


int almp3_get_msecs_per_frame_mp3(ALMP3_MP3 *mp3) {
  int msecs;
  msecs = (int)((double)mp3->outbytes_per_frame / (double)almp3_get_outbytes_per_sec_mp3(mp3) * (double)1000);
  return msecs;
}


int almp3_get_bitrate_mp3(ALMP3_MP3 *mp3) {
  return mp3->bitrate;
}


int almp3_get_layer_mp3(ALMP3_MP3 *mp3) {
  return mp3->layer;
}


int almp3_get_wave_bits_mp3(ALMP3_MP3 *mp3) {
  return mp3->bits;
}


int almp3_get_wave_is_stereo_mp3(ALMP3_MP3 *mp3) {
  return mp3->stereo;
}


int almp3_get_wave_freq_mp3(ALMP3_MP3 *mp3) {
  return mp3->freq;
}


SAMPLE *almp3_create_sample_from_mp3(ALMP3_MP3 *mp3) {
  SAMPLE *sample;
  char *data;
  int num_data_cursor, sample_len, sample_len_bytes, size_done;

  /* first we need to calculate the len of the sample (in frames and bytes) */
  sample_len_bytes = almp3_get_length_frames_mp3(mp3) * mp3->outbytes_per_frame;
  sample_len = (sample_len_bytes / (mp3->bits / 8)) / (mp3->stereo ? 2 : 1);
  
  /* create the sample */
  sample = create_sample(mp3->bits, mp3->stereo, mp3->freq, sample_len);

  /* return NULL if we were not able to allocate the memory for it */
  if (sample == NULL)
    return NULL;

  /* we need to stop and rewind the mp3 */
  almp3_stop_mp3(mp3);
  almp3_rewind_mp3(mp3);

  /* reinitialize the decoded if neecesary */
  if (mp3->dec_reinit) {
    if (almp3_reinit_decoder_mp3(mp3) < 0) {
      /* an error while reinitializing */
      return NULL;
    }
  }

  /* set our pointer */
  data = (char *)sample->data;
  /* clear the sample buffer */
  memset(data, 0, sample_len_bytes);

  /* decode */
  num_data_cursor = mp3->data_cursor - (char *)mp3->data;
  size_done = 0;
  while (num_data_cursor < mp3->data_len) {
    int ret, inread;
    
    /* send bytes until it has enough to render a frame */
    ret = almp3_decode_frame_mp3(mp3, mp3->data_cursor, data, &size_done, &inread);

    /* check if the decoding was not successful */
    if (ret == 0) {
      /* damn, seems like this frame is corrupt */
      /* check the next 16kb just to be sure */
      int j, got_error = TRUE, max, left;
      
      max = 16 * 1024;
      
      left = mp3->data_len - num_data_cursor;
      
      if (max > left)
        max = left;
      
      for (j = 0; (j < max) && got_error; j++) {
        /* check the header */
        if (almp3_check_frame_validity((unsigned int)almp3_get_big_endian(mp3->data_cursor)) == -1) {
          /* try to decode it */
          almp3_reinit_decoder_mp3(mp3);       
          ret = almp3_decode_frame_mp3(mp3, mp3->data_cursor, data, &size_done, &inread);
          if (ret == -1) {
            char *p = mp3->data_cursor + (mp3->mpeg).fr.framesize + 4;
            if (p < ((char *)mp3->data + mp3->data_len)) {
              /* check the next header too just to make sure */
              if (almp3_check_frame_validity((unsigned int)almp3_get_big_endian(p)) != 0)
                got_error = FALSE;
            }
            else {
              /* if we don't have enough data to check, assume it is */
              got_error = FALSE;
            }
          }
        }
        
        if (got_error)
          mp3->data_cursor++;
      }

      /* if we were unlucky, this is probably corrupt */
      /* (don't take it so seriously if it is the last frame) <- thanks to Bruno Dupuis */
      if (got_error && ((mp3->data_len - num_data_cursor) > (mp3->mpeg).fr.framesize)) {
        almp3_rewind_mp3(mp3);
        destroy_sample(sample);
        return NULL;
      }
    }

    data += size_done;
    mp3->data_cursor += inread;
    num_data_cursor = mp3->data_cursor - (char *)mp3->data;
    mp3->current_frame++;
  }

  almp3_rewind_mp3(mp3);

  /* convert to allegro unsigned format */
  if (mp3->bits == 16) {
    void *buf = sample->data;
    unsigned int i, *j, k;
    k = sample_len_bytes / 4;
    for (i = 0, j = (unsigned int *)buf; i < k; i++, j++)
      *j ^= 0x80008000;
  }

  return sample;
}


void almp3_adjust_mp3(ALMP3_MP3 *mp3, int vol, int pan, int speed, int loop) {
  /* return if we are not playing */
  if (!almp3_is_playing_mp3(mp3))
    return;

  /* adjust the sample */
  adjust_sample(mp3->audiostream->samp, vol, pan, speed, TRUE);
  mp3->loop = loop;
}


void *almp3_get_output_wave_mp3(ALMP3_MP3 *mp3, int *buffer_size) {
  /* return if we are not playing */
  if (!almp3_is_playing_mp3(mp3))
    return NULL;

  /* set the buffer_size */
  *buffer_size = mp3->outbytes_per_frame * mp3->frames_per_poll;

  /* return the current audiostream sample data */
  return (mp3->audiostream->samp->data);
}


int almp3_is_playing_mp3(ALMP3_MP3 *mp3) {
  if (mp3->audiostream == NULL)
    return FALSE;
  else
    return TRUE;
}


int almp3_is_looping_mp3(ALMP3_MP3 *mp3) {
  if (!almp3_is_playing_mp3(mp3))
    return FALSE;
  return mp3->loop;
}


void almp3_set_loop_mp3(ALMP3_MP3 *mp3, int loop) {
  /* return if we are not playing */
  if (!almp3_is_playing_mp3(mp3))
    return;

  mp3->loop = loop;
}


AUDIOSTREAM *almp3_get_audiostream_mp3(ALMP3_MP3 *mp3) {
  return mp3->audiostream;
}




/* API - MP3STREAM */

/* function called in an interrupt */

static void almp3_autopoll_mp3stream(void *mp3) {
  almp3_poll_mp3stream((ALMP3_MP3STREAM *)mp3);
}
END_OF_FUNCTION(almp3_autopoll_mp3stream);


/* helper functions */

static int almp3_reinit_decoder_mp3stream(ALMP3_MP3STREAM *mp3) {
  /* reinitialize the decoder */
  ExitMP3(&(mp3->mpeg));
  InitMP3(&(mp3->mpeg));

  mp3->dec_reinit = FALSE;

  return 0;
}


static void almp3_convert_to_unsigned_format_mp3stream(ALMP3_MP3STREAM *mp3, void *buf) {
  /* convert to allegro unsigned format */
  if (mp3->bits == 16) {
    unsigned int i, *j, k;
    k = (mp3->outbytes_per_frame * mp3->frames_per_poll) / 4;
    for (i = 0, j = (unsigned int *)buf; i < k; i++, j++)
      *j ^= 0x80008000;
  }
}


static int almp3_get_bytes_at_frame_using_toc_mp3stream(ALMP3_MP3STREAM *mp3, int frame, int data_len) {
  int tof[100], i, flags;
  int bytesmin, bytesmax, bytesdif, bytes;
  int framemin, framemax, framedif;

  /* check for missing data */
  if (mp3->xing_header == NULL)
    return -1;
  flags = mp3->xing_header->flags;
  if (!(flags & XING_FRAMES_FLAG) || !(flags & XING_TOC_FLAG))
    return -1;
  if ((frame >= mp3->xing_header->frames) || (frame < 0))
    return -1;

  /* create a table of frames */
  for (i = 0; i < 100; i++)
    tof[i] = (mp3->xing_header->toc[i] * mp3->xing_header->frames) / 256;
  
  /* check in what part of the table of frames is our frame */
  for (i = 0; i < 100; i++) {
    if (frame <= tof[i])
      break;
  }

  /* our frame is between the i% and (i + 1)% of the file */
  /* let's do more calcules for a "fine" seeking */
  if (i == 100)
    return data_len;
  bytesmin = (data_len * i) / 100;
  framemin = tof[i];
  if (i < 99) {
    bytesmax = (data_len * (i + 1)) / 100;
    framemax = tof[i + 1];
  }
  else {
    bytesmax = data_len - 1;
    framemax = mp3->xing_header->frames - 1;
  }
  bytesdif = bytesmax - bytesmin;
  framedif = framemax - framemin;

  bytes = bytesmin + (((frame - framemin) * bytesdif) / framedif);

  return bytes;
}


static int almp3_get_outbytes_per_sec_mp3stream(ALMP3_MP3STREAM *mp3) {
  int bytes_out;
  bytes_out = (mp3->stereo ? 2 : 1) * (mp3->bits / 8) * mp3->freq;
  return bytes_out;
}


static int almp3_decode_frame_mp3stream(ALMP3_MP3STREAM *mp3, char *inbuf, int inlen, char *outbuf, int *size_done, int *inread) {
  /* send bytes until it has enough to render a frame */
  int ret;
  *inread = 0;
  ret = decodeMP3(&(mp3->mpeg), NULL, 0, outbuf, 8192, size_done);
  if (ret == MP3_NEED_MORE) {
    int chunk_size;

    if (mp3->xing_header)
      chunk_size = MAXFRAMESIZE;
    else
      chunk_size = mp3->framebytes + 4;

    do {
      if (inlen <= 0)
        break;

      else if (inlen < chunk_size) {
        ret = decodeMP3(&(mp3->mpeg), inbuf, inlen, outbuf, 8192, size_done);
        inbuf += inlen;
        *inread += inlen;
        inlen = 0;
      }
      else {
        ret = decodeMP3(&(mp3->mpeg), inbuf, chunk_size, outbuf, 8192, size_done);
        inlen -= chunk_size;
        inbuf += chunk_size;
        *inread += chunk_size;
      }
    } while (ret == MP3_NEED_MORE);
  }

  {
    int val = almp3_check_frame_validity((mp3->mpeg).header);
    if ((ret == MP3_ERR) || (val == 0) || ((mp3->mpeg).fr.framesize <= 0))
      return 0;
    else
      return -1;
  }

}



/* API functions */

ALMP3_MP3STREAM *almp3_create_mp3stream(void *first_data_buffer, int data_buffer_len, int last_block) {
  ALMP3_MP3STREAM *mp3;
  struct mpstr test_mpeg;
  unsigned char *p = (unsigned char *)first_data_buffer;
  int skipbytes;
  void *databuf1, *databuf2, *full_databuf;
  int ret;
  char test_outbuf[8192];
  int test_size, framebytes;

  /* test the first 16 * MAXFRAMESIZE bytes until we find one valid */
  {
    int found, max;
    if (data_buffer_len >= (16 * MAXFRAMESIZE))
      max = 16 * MAXFRAMESIZE;
    else
      max = data_buffer_len;

    found = FALSE;
    skipbytes = 0;
    while (!found && (skipbytes < max)) {
      int data_left = max - skipbytes;
      char *p = (char *)first_data_buffer + skipbytes;

      if (almp3_check_frame_validity((unsigned int)almp3_get_big_endian(p)) == -1) {
        /* probably found, check */
        InitMP3(&test_mpeg);
        ret = decodeMP3(&test_mpeg, p, data_left, test_outbuf, 8192, &test_size);
        if ((ret == MP3_OK) && (test_mpeg.fr.framesize > 0)) {
          char *p2 = p + test_mpeg.fr.framesize + 4;
          if (p2 < ((char *)first_data_buffer + max)) {
            /* check the next frame to make sure */
            if (almp3_check_frame_validity((unsigned int)almp3_get_big_endian(p2)) != 0)
              found = TRUE;
          }
          else {
            /* guess it is right */
            found = TRUE;
          }
        }
          
        if (!found)
          ExitMP3(&test_mpeg);
      }

      if (!found)
        skipbytes++;
    }

    if (!found) {
      return NULL;
    }
  }

  /* must be layer III or II */
  if ((test_mpeg.fr.lay != 3) && (test_mpeg.fr.lay != 2)) {
    ExitMP3(&test_mpeg);
    return NULL;
  }

  /* the mp3 is ok, get some info */
  framebytes = test_mpeg.fr.framesize + 4;
  if ((framebytes <= 4)) {
    ExitMP3(&test_mpeg);
    return NULL;
  }

  /* allocate the buffers */
  databuf1 = malloc(data_buffer_len);
  if (databuf1 == NULL) {
    ExitMP3(&test_mpeg);
    return NULL;
  }
  databuf2 = malloc(data_buffer_len);
  if (databuf2 == NULL) {
    ExitMP3(&test_mpeg);
    free(databuf1);
    return NULL;
  }
  full_databuf = malloc(data_buffer_len * 2);
  if (full_databuf == NULL) {
    ExitMP3(&test_mpeg);
    free(databuf1);
    free(databuf2);
    return NULL;
  }

  /* copy the first data */
  memcpy(databuf1, first_data_buffer, data_buffer_len);

  /* create a new mp3 struct */
  mp3 = (ALMP3_MP3STREAM *)malloc(sizeof(ALMP3_MP3STREAM));
  if (mp3 == NULL) {
    ExitMP3(&test_mpeg);
    free(databuf1);
    free(databuf2);
    free(full_databuf);
    return NULL;
  }

  /* init the real decoder */
  InitMP3(&(mp3->mpeg));

  /* fill in the mp3 struct */
  mp3->data_start_skipped = skipbytes;
  mp3->databuf1 = databuf1;
  mp3->databuf2 = databuf2;
  mp3->full_databuf = (char *)full_databuf;
  mp3->data_cursor = skipbytes;
  mp3->databuf_selector = 1;
  mp3->databuf_len = data_buffer_len;
  mp3->unsel_databuf_free = TRUE;
  if (last_block)
    mp3->bytes_used = data_buffer_len;
  else
    mp3->bytes_used = -1;

  mp3->freq = almp3freqs[test_mpeg.fr.sampling_frequency];
  if (test_mpeg.fr.stereo >= 2)
    mp3->stereo = TRUE;
  else
    mp3->stereo = FALSE;
  mp3->bits = 16;
  mp3->outbytes_per_frame = test_size;
  mp3->audiostream = NULL;
  mp3->current_frame = 0;
  mp3->current_byte = skipbytes;
  mp3->auto_polling = FALSE;
  mp3->bitrate = almp3tabsel_123[test_mpeg.fr.lsf][test_mpeg.fr.lay - 1][test_mpeg.fr.bitrate_index] * 1000;
  mp3->layer = test_mpeg.fr.lay;
  if (mp3->layer <= 0)
    mp3->layer = -1;
  mp3->dec_reinit = FALSE;
  mp3->framebytes = framebytes;
  mp3->xing_header = NULL;

  /* if outbytes_per_frame is incorrect, correct it */
  if (mp3->outbytes_per_frame <= 0) {
    int samples = 1152;
    if (mp3->layer == 2)
      samples = 1152;
    if (mp3->layer == 3) {
      if ((mp3->freq >= 32000) && (mp3->freq < 48000))
        samples = 1152;
      else if ((mp3->freq >= 8000) && (mp3->freq < 24000))
        samples = 576;
    }
    mp3->outbytes_per_frame = samples * (mp3->stereo ? 2 : 1) * (mp3->bits / 8);
  }

  /* check for the xing header */
  p = (unsigned char *)((char *)first_data_buffer + skipbytes);
  if (!test_mpeg.fr.lsf) {             /* mpeg1 */
    if (test_mpeg.fr.mode != 3)
      p += 32 + 4;
    else
      p += 17 + 4;
  }
  else {                               /* mpeg2 */
    if (test_mpeg.fr.mode != 3)
      p += 17 + 4;
    else
      p += 9 + 4;
  }

  if (memcmp(p, "Xing", 4) == 0) {

    /* found */
    int x;
    p += 4;
    mp3->xing_header = (XING_HEADER *)malloc(sizeof(XING_HEADER));
    x = mp3->xing_header->flags = almp3_get_big_endian(p);
    p += 4;
    if (x & XING_FRAMES_FLAG) {
      mp3->xing_header->frames = almp3_get_big_endian(p);
      p += 4;
    }
    if (x & XING_BYTES_FLAG) {
      mp3->xing_header->bytes = almp3_get_big_endian(p);
      p += 4;
    }
    if (x & XING_TOC_FLAG) {
      int j;
      for (j = 0; j < 100; j++, p++)
        mp3->xing_header->toc[j] = *p;
    }
    mp3->xing_header->vbr_scale = -1;
    if (x & XING_VBR_SCALE_FLAG) {
      mp3->xing_header->vbr_scale = almp3_get_big_endian(p);
      p += 4;
    }

  }

  /* get the number of total frames if we are using a XING header*/
  if ((mp3->xing_header != NULL) && (mp3->xing_header->flags & XING_FRAMES_FLAG))
    mp3->total_frames = mp3->xing_header->frames;
  /* else we don't know it */
  else
    mp3->total_frames = -1;

  /* deinit the test decoder */
  ExitMP3(&test_mpeg);

  return mp3;
}


void almp3_destroy_mp3stream(ALMP3_MP3STREAM *mp3) {
  if (mp3 == NULL)
    return;
  
  almp3_stop_mp3stream(mp3);          /* note almp3_stop_mp3stream will */
                                      /* remove autopolling interrupts */
  ExitMP3(&(mp3->mpeg));
  if (mp3->xing_header)
    free(mp3->xing_header);
  free(mp3->full_databuf);
  free(mp3->databuf1);
  free(mp3->databuf2);
  free(mp3);  
}


int almp3_play_mp3stream(ALMP3_MP3STREAM *mp3, int buffer_len, int vol, int pan) {
  return almp3_play_ex_mp3stream(mp3, buffer_len, vol, pan, 1000);
}


int almp3_play_ex_mp3stream(ALMP3_MP3STREAM *mp3, int buffer_len, int vol, int pan, int speed) {
  int samples;

  /* continue only if we are not already playing it */
  if (almp3_is_playing_mp3stream(mp3))
    return ALMP3_OK;

  /* calculate the frames we can decode for each poll */
  /* using the given buffer_len */
  mp3->frames_per_poll = buffer_len / mp3->outbytes_per_frame;
  if (mp3->frames_per_poll <= 0)
    return ALMP3_PLAY_BUFFERTOOSMALL;
   
  /* create a new audiostream and play it */
  samples = (mp3->outbytes_per_frame * mp3->frames_per_poll) / (mp3->stereo ? 2 : 1) / (mp3->bits / 8);
  mp3->audiostream = play_audio_stream(samples, mp3->bits, mp3->stereo, mp3->freq, vol, pan);

  mp3->wait_for_audio_stop = 0;

  if (speed != 1000)
    adjust_sample(mp3->audiostream->samp, vol, pan, speed, TRUE);

  /* if the user asked for autopolling, install the interrupt now */
  if (mp3->auto_polling) {
    LOCK_FUNCTION(almp3_autopoll_mp3stream);
    install_param_int(&almp3_autopoll_mp3stream, (void *)mp3, mp3->auto_poll_speed);
  }

  return ALMP3_OK;
}


void almp3_stop_mp3stream(ALMP3_MP3STREAM *mp3) {
  /* continue if we are playing it */
  if (!almp3_is_playing_mp3stream(mp3))
    return;

  /* remove the interrupt, if we were using autopolling */
  if (mp3->auto_polling)
    remove_param_int(&almp3_autopoll_mp3stream, (void *)mp3);

  /* stop the audio stream */
  stop_audio_stream(mp3->audiostream);
  mp3->audiostream = NULL;
}


int almp3_poll_mp3stream(ALMP3_MP3STREAM *mp3) {
  void *audiobuf;
  char *audiobuf_p;
  char *temp_databuf;
  int temp_buf_size, using_full_databuf;
  int i, size_done;
  int last_block;

  /* continue only if we are playing it */
  if (!almp3_is_playing_mp3stream(mp3))
    return ALMP3_POLL_NOTPLAYING;

  /* get the audio stream buffer and only continue if we need to fill it */
  audiobuf = get_audio_stream_buffer(mp3->audiostream);
  if (audiobuf == NULL)
    return ALMP3_OK;

  /* clear the buffer */
  memset(audiobuf, 0, mp3->outbytes_per_frame * mp3->frames_per_poll);

  /* if we need to fill it, but we were just waiting for it to finish */
  if (mp3->wait_for_audio_stop > 0) {
    almp3_convert_to_unsigned_format_mp3stream(mp3, audiobuf);
    free_audio_stream_buffer(mp3->audiostream);
    if (--mp3->wait_for_audio_stop == 0) {
      /* stop it */
      almp3_stop_mp3stream(mp3);
      return ALMP3_POLL_PLAYJUSTFINISHED;
    }
    else
      return ALMP3_OK;
  }

  /* get if this will be the last block to be processed */
  if (mp3->bytes_used != -1)
    last_block = TRUE;
  else
    last_block = FALSE;

  /* if the 2 buffers are available, use the concatenated one */
  if (!mp3->unsel_databuf_free) {
    temp_databuf = mp3->full_databuf;
    using_full_databuf = TRUE;
    if (!last_block)
      temp_buf_size = mp3->databuf_len * 2;
    else
      temp_buf_size = mp3->databuf_len + mp3->bytes_used;
  }
  /* else just point to the current buffer */
  else {
    using_full_databuf = FALSE;
    if (mp3->databuf_selector == 1)
      temp_databuf = (char *)mp3->databuf1;
    else
      temp_databuf = (char *)mp3->databuf2;
    if (!last_block)
      temp_buf_size = mp3->databuf_len;
    else
      temp_buf_size = mp3->bytes_used;
  }

  /* reinitialize the decoder if needed */
  if (mp3->dec_reinit) {
    mp3->wait_for_audio_stop = 0;
    if (almp3_reinit_decoder_mp3stream(mp3) < 0) {
      /* an error while reinitializing */
      almp3_stop_mp3stream(mp3);
      return ALMP3_POLL_INTERNALERROR;
    }
  }

  audiobuf_p = (char *)audiobuf;
  size_done = 0;
  for (i = 0; i < (mp3->outbytes_per_frame * mp3->frames_per_poll); i += size_done) {
    int ret, temp_buf_left, inread;

    /* check for a ID1 tag - note we do this here cause ID1 tags */
    /* go at the end of file and we have no way to know where is the end */
    if (memcmp(temp_databuf + mp3->data_cursor, "TAG", 3) == 0) {
      mp3->data_cursor += 128;
      mp3->current_byte += 128;
      almp3_reinit_decoder_mp3stream(mp3);
    }

    /* check for buffer underrun or end of the mp3 */
    temp_buf_left = temp_buf_size - mp3->data_cursor;
    if (temp_buf_left <= 0) {
      almp3_convert_to_unsigned_format_mp3stream(mp3, audiobuf);
      free_audio_stream_buffer(mp3->audiostream);
      /* if this was not the last block, buffer underrun */
      if (!last_block) {
        almp3_stop_mp3stream(mp3);
        return ALMP3_POLL_BUFFERUNDERRUN;
      }
      /* else we just finished playing, we need to wait for audio to stop */
      else {
        mp3->wait_for_audio_stop = 2;
        return ALMP3_OK;
      }
    }

    ret = almp3_decode_frame_mp3stream(mp3, (char *)temp_databuf + mp3->data_cursor, temp_buf_left, audiobuf_p, &size_done, &inread);
    
    /* check if the decoding was not successful */
    if (ret == 0) {
      /* damn, seems like this frame is corrupt */
      /* check the next 16kb just to be sure */
      int j, max, got_error = TRUE, left;
      
      max = 16 * 1024;

      left = temp_buf_left;
      
      if (max > left)
        max = left;


      for (j = 0; (j < max) && (got_error); j++) {
        char *p;

        /* check for buffer underrun or end of the mp3 */
        if (mp3->data_cursor >= temp_buf_size) {
          almp3_convert_to_unsigned_format_mp3stream(mp3, audiobuf);
          free_audio_stream_buffer(mp3->audiostream);
          /* if this was not the last block, buffer underrun */
          if (!last_block) {
            almp3_stop_mp3stream(mp3);
            return ALMP3_POLL_BUFFERUNDERRUN;
          }
          /* else we just finished playing, we need to wait for audio to stop */
          else {
            mp3->wait_for_audio_stop = 2;
            return ALMP3_OK;
          }
        }

        /* check the header */
        p = (char *)temp_databuf + mp3->data_cursor;
        if (almp3_check_frame_validity((unsigned int)almp3_get_big_endian(p)) == -1) {
          /* try to decode it */
          almp3_reinit_decoder_mp3stream(mp3);
          ret = almp3_decode_frame_mp3stream(mp3, p, temp_buf_left, audiobuf_p, &size_done, &inread);
          if (ret == -1) {
            char *p = (char *)temp_databuf + mp3->data_cursor + (mp3->mpeg).fr.framesize + 4;
            if (p < ((char *)temp_databuf + temp_buf_size)) {
              /* check the next header too just to make sure */
              if (almp3_check_frame_validity((unsigned int)almp3_get_big_endian(p)) != 0)
                got_error = FALSE;
            }
            else {
              /* if we don't have enough data to check, assume it is */
              got_error = FALSE;
            }
          }
        }

        if (got_error) {
          mp3->data_cursor++;
          mp3->current_byte++;
          temp_buf_left--;
        }

      }
      /* if we are still unlucky, this is probably corrupt */
      if (got_error) {
        almp3_convert_to_unsigned_format_mp3stream(mp3, audiobuf);
        free_audio_stream_buffer(mp3->audiostream);
        almp3_stop_mp3stream(mp3);
        return ALMP3_POLL_FRAMECORRUPT;
      }
    }

    mp3->data_cursor += inread;
    mp3->current_byte += inread;
    audiobuf_p += size_done;
    mp3->current_frame++;
  }

  /* convert to allegro unsigned format */
  almp3_convert_to_unsigned_format_mp3stream(mp3, audiobuf);

  /* lock the buffer */
  free_audio_stream_buffer(mp3->audiostream);

  /* if we were using the full_databuf */
  if (using_full_databuf) {
    /* if the cursor passed the first buffer len */
    if (mp3->data_cursor >= mp3->databuf_len) {
      /* set the cursor to a good position and mark as free the old buffer */
      mp3->data_cursor -= mp3->databuf_len;
      mp3->databuf_selector = -mp3->databuf_selector;
      mp3->unsel_databuf_free = TRUE;
    }
  }
  /* using only one buffer */
  else {
    if (mp3->data_cursor >= mp3->databuf_len) {
      /* if this was not the last block, buffer underrun */
      if (!last_block) {
        almp3_stop_mp3stream(mp3);
        return ALMP3_POLL_BUFFERUNDERRUN;
      }
      /* else we just finished playing, we need to wait for audio to stop */
      else {
        mp3->wait_for_audio_stop = 2;
        return ALMP3_OK;
      }
    }
  }

  return ALMP3_OK;
}


void almp3_start_autopoll_mp3stream(ALMP3_MP3STREAM *mp3, int speed) {
  /* return if we are already auto polling */
  if (mp3->auto_polling)
    return;

  mp3->auto_polling = TRUE;
  mp3->auto_poll_speed = speed;

  /* only install the interrupt if we are already playing */
  /* otherwise, play will do */
  if (almp3_is_playing_mp3stream(mp3)) {
    LOCK_FUNCTION(almp3_autopoll_mp3stream);
    install_param_int(&almp3_autopoll_mp3stream, (void *)mp3, mp3->auto_poll_speed);
  }
}


void almp3_stop_autopoll_mp3stream(ALMP3_MP3STREAM *mp3) {
  /* return if we are already not auto polling */
  if (!mp3->auto_polling)
    return;

  mp3->auto_polling = FALSE;

  /* only remove the interrupt if we were already playing */
  /* otherwise, stop will do*/
  if (almp3_is_playing_mp3stream(mp3))
    remove_param_int(&almp3_autopoll_mp3stream, (void *)mp3);
}


void *almp3_get_mp3stream_buffer(ALMP3_MP3STREAM *mp3) {
  if (!mp3->unsel_databuf_free)
    return NULL;

  /* if the last block was already passed, we don't need more data */
  if (mp3->bytes_used != -1)
    return NULL;

  if (mp3->databuf_selector == 1)
    return mp3->databuf2;
  else
    return mp3->databuf1;
}


void almp3_free_mp3stream_buffer(ALMP3_MP3STREAM *mp3, int bytes_used) {
  if (mp3->unsel_databuf_free) {
    mp3->unsel_databuf_free = FALSE;
    mp3->bytes_used = bytes_used;
    if (mp3->databuf_selector == 1) {
      memcpy(mp3->full_databuf, mp3->databuf1, mp3->databuf_len);
      memcpy(mp3->full_databuf + mp3->databuf_len, mp3->databuf2, mp3->databuf_len);
    }
    else {
      memcpy(mp3->full_databuf, mp3->databuf2, mp3->databuf_len);
      memcpy(mp3->full_databuf + mp3->databuf_len, mp3->databuf1, mp3->databuf_len);
    }
  }
}


int almp3_get_pos_frames_mp3stream(ALMP3_MP3STREAM *mp3) {
  return mp3->current_frame;
}


int almp3_get_pos_msecs_mp3stream(ALMP3_MP3STREAM *mp3) {
  float second;
  second = mp3->current_frame * mp3->outbytes_per_frame;
  second /= (mp3->stereo ? 2 : 1) * (mp3->bits / 8);
  second /= (float)mp3->freq;
  return (int)(1000 * second);
}


int almp3_get_pos_secs_mp3stream(ALMP3_MP3STREAM *mp3) {
  return (almp3_get_pos_msecs_mp3stream(mp3) / 1000);
}


int almp3_get_pos_bytes_mp3stream(ALMP3_MP3STREAM *mp3) {
  return mp3->current_byte;
}


int almp3_get_msecs_per_frame_mp3stream(ALMP3_MP3STREAM *mp3) {
  int msecs;
  msecs = (int)((double)mp3->outbytes_per_frame / (double)almp3_get_outbytes_per_sec_mp3stream(mp3) * (double)1000);
  return msecs;
}


int almp3_get_bitrate_mp3stream(ALMP3_MP3STREAM *mp3) {
  return mp3->bitrate;
}


int almp3_get_layer_mp3stream(ALMP3_MP3STREAM *mp3) {
  return mp3->layer;
}


int almp3_get_wave_bits_mp3stream(ALMP3_MP3STREAM *mp3) {
  return mp3->bits;
}


int almp3_get_wave_is_stereo_mp3stream(ALMP3_MP3STREAM *mp3) {
  return mp3->stereo;
}


int almp3_get_wave_freq_mp3stream(ALMP3_MP3STREAM *mp3) {
  return mp3->freq;
}


void almp3_adjust_mp3stream(ALMP3_MP3STREAM *mp3, int vol, int pan, int speed) {
  /* return if we are not playing */
  if (!almp3_is_playing_mp3stream(mp3))
    return;

  /* adjust the sample */
  adjust_sample(mp3->audiostream->samp, vol, pan, speed, TRUE);
}


void *almp3_get_output_wave_mp3stream(ALMP3_MP3STREAM *mp3, int *buffer_size) {
  /* return if we are not playing */
  if (!almp3_is_playing_mp3stream(mp3))
    return NULL;

  /* set the buffer_size */
  *buffer_size = mp3->outbytes_per_frame * mp3->frames_per_poll;

  /* return the current audiostream sample data */
  return (mp3->audiostream->samp->data);
}


int almp3_get_length_frames_mp3stream(ALMP3_MP3STREAM *mp3, int total_size) {
  /* if we have the frames from the XING header, use it */
  if (mp3->total_frames != -1)
    return mp3->total_frames;
  else
    return (total_size / mp3->framebytes);
}


int almp3_get_length_msecs_mp3stream(ALMP3_MP3STREAM *mp3, int total_size) {
  int msecs;
  msecs = almp3_get_length_frames_mp3stream(mp3, total_size) * almp3_get_msecs_per_frame_mp3stream(mp3);
  return msecs;
}


int almp3_get_length_secs_mp3stream(ALMP3_MP3STREAM *mp3, int total_size) {
  return (almp3_get_length_msecs_mp3stream(mp3, total_size) / 1000);
}


int almp3_get_length_bytes_mp3stream(ALMP3_MP3STREAM *mp3, int total_size) {
  return total_size;
}


int almp3_is_playing_mp3stream(ALMP3_MP3STREAM *mp3) {
  if (mp3->audiostream == NULL)
    return FALSE;
  else
    return TRUE;
}


AUDIOSTREAM *almp3_get_audiostream_mp3stream(ALMP3_MP3STREAM *mp3) {
  return mp3->audiostream;
}


int almp3_seek_abs_frames_mp3stream(ALMP3_MP3STREAM *mp3, int frame, int total_size) {
  int bytes;
  int total_frames;

  /* get the number of total frames */
  total_frames = almp3_get_length_frames_mp3stream(mp3, total_size);

  /* check if the frame is valid */
  if ((frame < 0) || (frame >= total_frames))
    return -1;

  /* first try using the xing header */
  bytes = almp3_get_bytes_at_frame_using_toc_mp3stream(mp3, frame, total_size);

  /* seems like we are using it */
  if (bytes >= 0)
    return bytes;

  /* no, use normal method */
  else
    return (frame * mp3->framebytes);
}


int almp3_seek_abs_msecs_mp3stream(ALMP3_MP3STREAM *mp3, int msecs, int total_size) {
  int frame;
  frame = msecs / almp3_get_msecs_per_frame_mp3stream(mp3);
  return almp3_seek_abs_frames_mp3stream(mp3, frame, total_size);
}


int almp3_seek_abs_secs_mp3stream(ALMP3_MP3STREAM *mp3, int secs, int total_size){
  return almp3_seek_abs_msecs_mp3stream(mp3, secs * 1000, total_size);
}


int almp3_seek_abs_bytes_mp3stream(ALMP3_MP3STREAM *mp3, int bytes, int total_size) {
  /* check the pos in bytes is valid */
  /* we check against -mp3->data_start_skipped because our data buffer */
  /* skipped the very first ID3v2 header */
  if ((bytes < -mp3->data_start_skipped) || (bytes >= total_size))
    return -1;

  /* if the poiner wanted was inside the ID3v2 header, skip it and */
  /* point directly to the data */
  if (bytes < 0)
    bytes = 0;

  return bytes;
}


int almp3_seek_rel_frames_mp3stream(ALMP3_MP3STREAM *mp3, int frame, int total_size) {
  int bytes;
  int wanted_frame = mp3->current_frame + frame;
  int total_frames;

  /* get total frames */
  total_frames = almp3_get_length_frames_mp3stream(mp3, total_size);

  /* check the frame is valid */
  if ((wanted_frame < 0) || (wanted_frame >= total_frames))
    return -1;

  /* first try using the xing header */
  bytes = almp3_get_bytes_at_frame_using_toc_mp3stream(mp3, wanted_frame, total_size);

  /* seems like we are using it */
  if (bytes >= 0)
    return bytes;

  /* no, use normal method */
  else
    return (frame * mp3->framebytes);
}


int almp3_seek_rel_msecs_mp3stream(ALMP3_MP3STREAM *mp3, int msec, int total_size) {
  int frame;
  frame = msec / almp3_get_msecs_per_frame_mp3stream(mp3);
  return almp3_seek_rel_frames_mp3stream(mp3, frame, total_size);
}


int almp3_seek_rel_secs_mp3stream(ALMP3_MP3STREAM *mp3, int sec, int total_size) {
  return almp3_seek_rel_msecs_mp3stream(mp3, sec * 1000, total_size);
}


int almp3_seek_rel_bytes_mp3stream(ALMP3_MP3STREAM *mp3, int bytes, int total_size) {
  int current_byte = almp3_get_pos_bytes_mp3stream(mp3);
  int wanted_byte = current_byte + bytes;

  /* check the byte is valid */
  /* we check against -mp3->data_start_skipped because our data buffer */
  /* skipped the very first ID3v2 header */
  if ((wanted_byte < -mp3->data_start_skipped) || (wanted_byte >= total_size))
    return -1;

  /* if the poiner wanted was inside the ID3v2 header, skip it and */
  /* point directly to the data */
  if (wanted_byte < 0)
    wanted_byte = 0;

  return wanted_byte;
}
