/*
  miniaudio_vorbis.h -- libvorbis decoder for miniaudio
  Project URL: https://github.com/edubart/minivorbis

  Use this file to replace miniaudio vorbis decoder with minivorbis.h.

  You should make the implementation like this:
    #include "miniaudio.h"
    #include "miniaudio_vorbis.h"
    #define MINIAUDIO_IMPLEMENTATION
    #include "miniaudio.h"
    #include "miniaudio_vorbis.h"

  LICENSE
    MIT No Attribution, see end of file.
*/

#ifndef miniaudio_vorbis_h
#define miniaudio_vorbis_h

#if !defined(miniaudio_h) || defined(MINIAUDIO_IMPLEMENTATION)
#error "include miniaudio.h without implementation first"
#endif

#ifndef MA_HAS_VORBIS
#define MA_HAS_VORBIS
#endif

static ma_result ma_decoder_init_vorbis__internal(const ma_decoder_config* pConfig, ma_decoder* pDecoder);

#endif /* miniaudio_vorbis_h */

#ifdef MINIAUDIO_IMPLEMENTATION

#define OGG_IMPL
#define VORBIS_IMPL
#define OV_EXCLUDE_STATIC_CALLBACKS
#include "minivorbis.h"

static ma_result ma_decoder_internal_on_seek_to_pcm_frame__vorbis(ma_decoder* pDecoder, ma_uint64 frameIndex)
{
    OggVorbis_File* pVorbis = (OggVorbis_File*)pDecoder->pInternalDecoder;
    MA_ASSERT(pVorbis != NULL);
    return ov_pcm_seek(pVorbis, frameIndex) == 0 ? MA_SUCCESS : MA_ERROR;
}

static ma_result ma_decoder_internal_on_uninit__vorbis(ma_decoder* pDecoder)
{
    OggVorbis_File* pVorbis = (OggVorbis_File*)pDecoder->pInternalDecoder;
    MA_ASSERT(pVorbis != NULL);
    ov_clear(pVorbis);
    ma__free_from_callbacks(pVorbis, &pDecoder->allocationCallbacks);
    return MA_SUCCESS;
}

static ma_uint64 ma_decoder_internal_on_read_pcm_frames__vorbis(ma_decoder* pDecoder, void* pFramesOut, ma_uint64 frameCount)
{
    MA_ASSERT(pDecoder != NULL);
    OggVorbis_File* pVorbis = (OggVorbis_File*)pDecoder->pInternalDecoder;
    MA_ASSERT(pVorbis != NULL);
    MA_ASSERT(pFramesOut != NULL);
    MA_ASSERT(pDecoder->internalFormat == ma_format_f32);
    float* pFramesOutF = (float*)pFramesOut;
    int section = 0;
    long totalFramesRead = 0;
    int channels = pDecoder->internalChannels;
    long framesLeft = frameCount;

    /* Keep reading until we read the desired frames amount. */
    while(framesLeft > 0) {
        float** outFrames = NULL;
        long framesRead = ov_read_float(pVorbis, &outFrames, framesLeft, &section);
        if(framesRead <= 0)
            break;

        for(int j=0;j<channels;++j) {
            for(int i=0;i<framesRead;++i) {
                pFramesOutF[i*channels+j] = outFrames[j][i];
            }
        }

        framesLeft      -= framesRead;
        totalFramesRead += framesRead;
        pFramesOutF     += framesRead * channels;
    }

    return totalFramesRead;
}

static ma_uint64 ma_decoder_internal_on_get_length_in_pcm_frames__vorbis(ma_decoder* pDecoder)
{
    MA_ASSERT(pDecoder != NULL);
    OggVorbis_File* pVorbis = (OggVorbis_File*)pDecoder->pInternalDecoder;
    return ov_pcm_total(pVorbis, -1);
}

static size_t ma_vorbis_ov_read__internal(void *ptr, size_t size, size_t nmemb, void* param)
{
    ma_decoder* pDecoder = (ma_decoder*)param;
    MA_ASSERT(pDecoder != NULL);
    return ma_decoder_read_bytes(pDecoder, ptr, size*nmemb);
}

static int ma_vorbis_ov_seek__internal(void* param, ogg_int64_t offset, int whence)
{
    if(whence == SEEK_END)
        return -1; /* Seek to end is not supported in miniaudio decoder. */
    ma_decoder* pDecoder = (ma_decoder*)param;
    MA_ASSERT(pDecoder != NULL);
    ma_seek_origin origin = (whence == SEEK_CUR) ? ma_seek_origin_current : ma_seek_origin_start;
    return ma_decoder_seek_bytes(pDecoder, offset, origin);
}

static int ma_vorbis_ov_close__internal(void* param)
{
    return 0;
}

static long ma_vorbis_ov_tell__internal(void* param)
{
    ma_decoder* pDecoder = (ma_decoder*)param;
    MA_ASSERT(pDecoder != NULL);
    return pDecoder->readPointerInBytes;
}

static ov_callbacks ma_vorbis_ov_decoder_callbacks = {
    ma_vorbis_ov_read__internal,
    ma_vorbis_ov_seek__internal,
    ma_vorbis_ov_close__internal,
    ma_vorbis_ov_tell__internal
};

ma_result ma_decoder_init_vorbis__internal(const ma_decoder_config* pConfig, ma_decoder* pDecoder)
{
    MA_ASSERT(pConfig != NULL);
    MA_ASSERT(pDecoder != NULL);

    OggVorbis_File* pVorbis = (OggVorbis_File*)ma__malloc_from_callbacks(sizeof(OggVorbis_File), &pDecoder->allocationCallbacks);
    if (pVorbis == NULL)
        return MA_OUT_OF_MEMORY;
    MA_ZERO_MEMORY(pVorbis, sizeof(OggVorbis_File));

    if(ov_open_callbacks(pDecoder, pVorbis, NULL, 0, ma_vorbis_ov_decoder_callbacks) != 0) {
        ma__free_from_callbacks(pVorbis, &pDecoder->allocationCallbacks);
        return MA_INVALID_FILE;  /* Failed to read vorbis file */
    }

    vorbis_info* vorbisInfo = ov_info(pVorbis, -1);
    if(vorbisInfo == NULL) {
        ov_clear(pVorbis);
        ma__free_from_callbacks(pVorbis, &pDecoder->allocationCallbacks);
        return MA_INVALID_FILE;  /* Failed to retrieve vorbis info */
    }

    /* Don't allow more than MA_MAX_CHANNELS channels. */
    if (vorbisInfo->channels > MA_MAX_CHANNELS) {
        ov_clear(pVorbis);
        ma__free_from_callbacks(pVorbis, &pDecoder->allocationCallbacks);
        return MA_ERROR; /* Too many channels. */
    }

    pDecoder->onReadPCMFrames        = ma_decoder_internal_on_read_pcm_frames__vorbis;
    pDecoder->onSeekToPCMFrame       = ma_decoder_internal_on_seek_to_pcm_frame__vorbis;
    pDecoder->onUninit               = ma_decoder_internal_on_uninit__vorbis;
    pDecoder->onGetLengthInPCMFrames = ma_decoder_internal_on_get_length_in_pcm_frames__vorbis;
    pDecoder->pInternalDecoder       = pVorbis;

    /* The internal format is always f32. */
    pDecoder->internalFormat     = ma_format_f32;
    pDecoder->internalChannels   = vorbisInfo->channels;
    pDecoder->internalSampleRate = vorbisInfo->rate;
    ma_get_standard_channel_map(ma_standard_channel_map_vorbis, pDecoder->internalChannels, pDecoder->internalChannelMap);

    return MA_SUCCESS;
}

#endif /* MINIAUDIO_IMPLEMENTATION */

/*
Copyright (c) 2020 Eduardo Bart (https://github.com/edubart)

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
