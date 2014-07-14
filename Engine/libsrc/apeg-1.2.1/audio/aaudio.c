#include <string.h>

#include "apeg.h"
#include "mpg123.h"
#include "mpeg1dec.h"


int _apeg_audio_get_position(APEG_LAYER *layer)
{
    int np;

#ifndef DISABLE_MPEG_AUDIO
    if(layer->audio.rd.eof)
        return -1;
#endif
    if(!layer->audio.stream)
        return 0;

    np = layer->audio.pos;
    if(np < 0)
    {
        layer->audio.last_pos = np;
        return 0;
    }

    if(np <= layer->audio.last_pos)
        return -1;

    layer->audio.last_pos = np;
    return np;
}

void _apeg_audio_set_speed_multiple(APEG_LAYER *layer, float multiple)
{
    if(layer->audio.stream)
    {
        int newfreq = (float)layer->stream.audio.freq * multiple;
        int voice = layer->audio.voice;

        if(newfreq > 0)
        {
            voice_set_frequency(voice, newfreq);
            voice_start(voice);
        }
        else
            voice_stop(voice);
    }
}


int _apeg_audio_flush(APEG_LAYER *layer)
{
    unsigned char *buf = layer->audio.pcm.samples;
    unsigned char *data;
    int hs;
    int ret = APEG_OK;

    if(layer->audio.pcm.point < layer->audio.bufsize)
    {
        int count = layer->audio.pcm.point / 2;
        int samplesend = layer->audio.bufsize / 2;

        while(count < samplesend)
            ((short*)buf)[count++] = 0x8000;

        if(layer->audio.pcm.point == 0)
            ret = APEG_EOF;
    }

    if(layer->audio.callback)
    {
        if(ret != APEG_OK)
            return ret;

        ret = layer->audio.callback((APEG_STREAM*)layer, buf,
            layer->audio.pcm.point,
            layer->audio.callback_arg);
        if(ret < 0)
            return APEG_ERROR;

        if(ret > 0)
        {
            layer->audio.pos += ret / 2 / layer->stream.audio.channels;
            layer->audio.pcm.point -= ret;
            if(layer->audio.pcm.point > 0)
                memmove(buf, buf+ret, layer->audio.pcm.point);
            layer->stream.audio.flushed = TRUE;

            if(!(layer->stream.flags&APEG_HAS_VIDEO))
                layer->stream.pos = (double)layer->audio.pos /
                (double)layer->stream.audio.freq;
        }

        return APEG_OK;
    }

    /* We need to test the stream buffer to see if it's ready for more audio
    * yet.
    */
    hs = layer->audio.stream->len/2;
    if((layer->audio.buf_segment &&
        voice_get_position(layer->audio.voice) >= hs) ||
        (!layer->audio.buf_segment &&
        voice_get_position(layer->audio.voice) < hs))
        return ret;

    voice_stop(layer->audio.voice);
    data  = layer->audio.stream->data;
    data += layer->audio.buf_segment * hs * layer->stream.audio.channels * 2;

    /* Commit the buffer to the stream and update the time */
    memcpy(data, buf, layer->audio.bufsize);

    voice_start(layer->audio.voice);
    layer->audio.buf_segment ^= 1;

    layer->audio.pos += ((layer->audio.pcm.point >= layer->audio.bufsize) ?
        layer->audio.samples_per_update :
    (layer->audio.pcm.point/2/layer->stream.audio.channels));
    if(!(layer->stream.flags&APEG_HAS_VIDEO))
        layer->stream.pos = (double)layer->audio.pos /
        (double)layer->stream.audio.freq;

    /* Remove the old data and put the unused samples at the beginning */
    layer->audio.pcm.point -= layer->audio.bufsize;
    if(layer->audio.pcm.point > 0)
        memmove(buf, buf+layer->audio.bufsize, layer->audio.pcm.point);
    else if(layer->audio.pcm.point < 0)
        layer->audio.pcm.point = 0;

    layer->stream.audio.flushed = TRUE;

    return ret;
}

int _apeg_audio_close(APEG_LAYER *layer)
{
    if(layer->audio.stream)
        destroy_sample(layer->audio.stream);
    layer->audio.stream = NULL;
    layer->audio.voice = -1;

    return 0;
}
