//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
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

    _name = name;
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
    _frameRate = _apegStream->frame_rate;
    _frameSize = Size(video_w, video_h);
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
    return HError::None();
}

void TheoraPlayer::CloseImpl()
{
    apeg_close_stream(_apegStream);
    _apegStream = nullptr;
}

bool TheoraPlayer::RewindImpl()
{
    if (apeg_reset_stream(_apegStream) != APEG_OK)
    {
        OpenAPEGStream(_dataStream.get(), _name, _usedFlags, _usedDepth);
    }
    return _apegStream != nullptr;
}

bool TheoraPlayer::NextVideoFrame(Bitmap *dst)
{
    assert(_apegStream);
    assert((_apegStream->flags & APEG_HAS_VIDEO) != 0);
    if ((_apegStream->flags & APEG_HAS_VIDEO) == 0)
        return false;

    // reset some data
    _apegStream->frame_updated = -1;

    // Read video frame (encoded)
    int ret = apeg_get_video_frame(_apegStream);
    if (ret == APEG_ERROR)
        return false;

    // Update frame count
    ++(_apegStream->frame);

    // Update the display frame (decode to RGB)
    _apegStream->frame_updated = 0;
    ret = apeg_display_video_frame(_apegStream);
    if (ret == APEG_ERROR || ret == APEG_EOF)
        return false;

    // TODO: find a way to optimize Theora decoder by providing our own src bitmap directly
    dst->Blit(_theoraSrcFrame.get());
    return true;
}

SoundBuffer TheoraPlayer::NextAudioFrame()
{
    assert(_apegStream);
    assert((_apegStream->flags & APEG_HAS_AUDIO) != 0);
    if ((_apegStream->flags & APEG_HAS_AUDIO) == 0)
        return SoundBuffer();

    // reset some data
    _apegStream->audio.flushed = FALSE;

    unsigned char *buf = nullptr;
    int count = 0;
    int ret = apeg_get_audio_frame(_apegStream, &buf, &count);
    if (ret == APEG_ERROR || ret == APEG_EOF)
        return SoundBuffer();
    return SoundBuffer(buf, count);
}

} // namespace Engine
} // namespace AGS

#endif // AGS_NO_VIDEO_PLAYER
