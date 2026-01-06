//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "media/video/theora_player.h"

#ifndef AGS_NO_VIDEO_PLAYER

#include <inttypes.h>
#include "debug/out.h"

namespace AGS
{
namespace Engine
{

using namespace Common;

TheoraPlayer::~TheoraPlayer()
{
    CloseImpl();
}

//
// Theora stream reader callbacks. We need these because APEG library does not
// provide means to supply user's PACKFILE directly.
//
// Open stream for reading (return suggested cache buffer size).
int apeg_stream_init(void *ptr)
{
    if (!ptr) return 0;
    ((Stream*)ptr)->Seek(0, kSeekBegin);
    return F_BUF_SIZE;
}
// Read requested number of bytes into provided buffer,
// return actual number of bytes managed to read.
int apeg_stream_read(void *buffer, int bytes, void *ptr)
{
    return ((Stream*)ptr)->Read(buffer, bytes);
}
// Skip requested number of bytes
void apeg_stream_skip(int bytes, void *ptr)
{
    ((Stream*)ptr)->Seek(bytes);
}
//

HError TheoraPlayer::OpenImpl(std::unique_ptr<Stream> data_stream,
    const String &name, int &flags, int target_depth)
{
    // Init APEG
    HError err = OpenAPEGStream(data_stream.get(), name, flags, target_depth);
    if (!err)
        return err;

    if ((_apegStream->flags & APEG_HAS_VIDEO) == 0)
        flags &= ~kVideo_EnableVideo;
    if ((_apegStream->flags & APEG_HAS_AUDIO) == 0)
        flags &= ~kVideo_EnableAudio;
    _dataStream = std::move(data_stream);
    return HError::None();
}

HError TheoraPlayer::OpenAPEGStream(Stream *data_stream, const Common::String &name, int flags, int target_depth)
{
    if (_apegStream)
    {
        apeg_close_stream(_apegStream);
        _apegStream = nullptr;
    }

    // NOTE: following settings affect only next apeg_open_stream* or
    // apeg_reset_stream.
    apeg_set_stream_reader(apeg_stream_init, apeg_stream_read, apeg_stream_skip);
    apeg_set_display_depth(target_depth);
    // we must disable length detection, otherwise it takes ages to start
    // playing if the file is large because it seeks through the whole thing
    apeg_disable_length_detection(TRUE);
    apeg_ignore_audio((flags & kVideo_EnableAudio) == 0);

    APEG_STREAM* apeg_stream = apeg_open_stream_ex(data_stream);
    if (!apeg_stream)
    {
        return new Error(String::FromFormat("Failed to open theora video '%s'; could be an invalid or unsupported format", name.GetCStr()));
    }
    int video_w = apeg_stream->w, video_h = apeg_stream->h;
    if (video_w <= 0 || video_h <= 0)
    {
        return new Error(String::FromFormat("Failed to run theora video '%s': invalid frame dimensions (%d x %d)", name.GetCStr(), video_w, video_h));
    }

    _apegStream = apeg_stream;
    _usedFlags = flags;
    _usedDepth = target_depth;

    _frameDepth = target_depth;
    _frameSize = Size(video_w, video_h);
    _frameRate = _apegStream->frame_rate;
    _frameTime = 1000.f / _apegStream->frame_rate;
    _frameCount = static_cast<uint32_t>(_apegStream->length / _frameTime);
    _durationMs = _apegStream->length;
    // According to the documentation:
    // encoded theora frames must be a multiple of 16 in width and height.
    // Which means that the original content may end up positioned on a larger frame.
    // In such case we store this surface in a separate wrapper for the reference,
    // while the actual video frame is assigned a sub-bitmap (a portion of the full frame).
    if (((flags & kVideo_LegacyFrameSize) == 0) &&
        Size(_apegStream->bitmap->w, _apegStream->bitmap->h) != _frameSize)
    {
        _theoraFullFrame.reset(BitmapHelper::CreateRawBitmapWrapper(_apegStream->bitmap));
        _theoraSrcFrame.reset(BitmapHelper::CreateSubBitmap(_theoraFullFrame.get(), RectWH(_frameSize)));
    }
    else
    {
        _theoraSrcFrame.reset(BitmapHelper::CreateRawBitmapWrapper(_apegStream->bitmap));
    }

    _audioChannels = _apegStream->audio.channels;
    _audioFreq = _apegStream->audio.freq;
    _audioFormat = AUDIO_S16SYS;
    apeg_set_error(_apegStream, NULL);
    _videoFramesDecodedTotal = 0u;
    _videoFramesDecoded = 0u;
    _nextFrameTs = 0.f;

    const char *pixelfmt_str[] = { "APEG_420", "APEG_422", "APEG_444" };
    Debug::Printf("TheoraPlayer: opened video \"%s\": %dx%d fmt: %s, fps: %.4f"
                  "\n\taudio: %d Hz, chans: %d",
                  name.GetCStr(),
                  apeg_stream->w, apeg_stream->h,
                  (apeg_stream->pixel_format >= APEG_STREAM::APEG_420 && apeg_stream->pixel_format <= APEG_STREAM::APEG_444) ? pixelfmt_str[apeg_stream->pixel_format] : "unknown",
                  static_cast<float>(apeg_stream->frame_rate),
                  _audioFreq, _audioChannels);

    return HError::None();
}

void TheoraPlayer::CloseImpl()
{
    if (_apegStream)
    {
        apeg_close_stream(_apegStream);
        _apegStream = nullptr;
        Debug::Printf("TheoraPlayer: closed, total video frames decoded: %" PRIu64 "", _videoFramesDecodedTotal);
        _videoFramesDecodedTotal = 0u;
    }
}

bool TheoraPlayer::RewindImpl()
{
    if (apeg_reset_stream(_apegStream) != APEG_OK)
    {
        OpenAPEGStream(_dataStream.get(), GetName(), _usedFlags, _usedDepth);
    }
    // reset video position record
    _videoFramesDecoded = 0u;
    _nextFrameTs = 0.f;
    return _apegStream != nullptr;
}

bool TheoraPlayer::NextVideoFrame(Bitmap *dst, float &ts)
{
    ts = -1.f; // reset in case of error

    assert(_apegStream);
    assert((_apegStream->flags & APEG_HAS_VIDEO) != 0);
    if ((_apegStream->flags & APEG_HAS_VIDEO) == 0)
        return false;

    // Read video frame (encoded)
    int ret = apeg_get_video_frame(_apegStream);
    if (ret == APEG_ERROR)
        return false;

    // Update the display frame (decode to RGB)
    ret = apeg_display_video_frame(_apegStream);
    if (ret == APEG_ERROR || ret == APEG_EOF)
        return false; // NOTE: apeg_display_video_frame returns EOF when picture is NULL

    _videoFramesDecoded++;
    _videoFramesDecodedTotal++;
    // TODO: investigate if it's possible to optimize this by providing our own src bitmap directly;
    // but in theory the frame image may be composed of multiple frames which contain partial image,
    // in which case we probably cannot do this...
    dst->Blit(_theoraSrcFrame.get());
    ts = _nextFrameTs;
    _nextFrameTs = _apegStream->pos * 1000.f; // to milliseconds (FIXME: should we keep ours in seconds?)
    return true;
}

bool TheoraPlayer::NextAudioFrame(SoundBuffer &abuf)
{
    assert(_apegStream);
    assert((_apegStream->flags & APEG_HAS_AUDIO) != 0);
    if ((_apegStream->flags & APEG_HAS_AUDIO) == 0)
        return SoundBufferPtr();

    // reset some data
    _apegStream->audio.flushed = FALSE;

    unsigned char *buf = nullptr;
    int count = 0;
    int ret = apeg_get_audio_frame(_apegStream, &buf, &count);
    if (ret == APEG_ERROR || ret == APEG_EOF)
        return false;

    abuf.AssignData(buf, count, -1.f, SoundHelper::MillisecondsFromBytes(count, _audioFormat, _audioChannels, _audioFreq));
    return true;
}

float TheoraPlayer::PeekVideoFrame()
{
    assert(_apegStream);
    assert((_apegStream->flags & APEG_HAS_VIDEO) != 0);
    if ((_apegStream->flags & APEG_HAS_VIDEO) == 0)
        return -1.f;

    if (apeg_eof(_apegStream))
        return -1.f;

    return _nextFrameTs;
}

void TheoraPlayer::DropVideoFrame()
{
    assert(_apegStream);
    assert((_apegStream->flags & APEG_HAS_VIDEO) != 0);
    if ((_apegStream->flags & APEG_HAS_VIDEO) == 0)
        return;

    if (apeg_eof(_apegStream))
        return;

    // Skip video frame
    int ret = apeg_skip_video_frame(_apegStream);
    if (ret == APEG_ERROR)
        return;

    _videoFramesDecoded++;
    _videoFramesDecodedTotal++;
    _nextFrameTs = _apegStream->pos * 1000.f; // to milliseconds (FIXME: should we keep ours in seconds?)
}

} // namespace Engine
} // namespace AGS

#endif // AGS_NO_VIDEO_PLAYER
