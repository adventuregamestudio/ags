//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// Theora (OGV) video player implementation.
//
// TODO:
//     - support random Seek (APEG only provides reset/rewind atm).
//     - for the long term - consider replacing APEG with a contemporary and
//       feature-complete library.
//
//=============================================================================
#ifndef __AGS_EE_MEDIA__THEORAPLAYER_H
#define __AGS_EE_MEDIA__THEORAPLAYER_H

#include <apeg.h>
#include "media/video/videoplayer.h"

namespace AGS
{
namespace Engine
{

class TheoraPlayer : public VideoPlayer
{
public:
    TheoraPlayer() = default;
    ~TheoraPlayer();

    bool IsValid() override { return _apegStream != nullptr; }

private:
    Common::HError OpenImpl(std::unique_ptr<Common::Stream> data_stream,
        const String &name, int &flags, int target_depth) override;
    void CloseImpl() override;
    bool RewindImpl() override;
    // Retrieves next video frame, implementation-specific
    bool NextVideoFrame(Common::Bitmap *dst, float &ts) override;
    // Retrieves next audio frame, implementation-specific
    bool NextAudioFrame(SoundBuffer &abuf) override;
    // Checks the next video frame in stream and returns its timestamp.
    float PeekVideoFrame() override;
    // Drop next video frame from stream.
    void DropVideoFrame() override;

    Common::HError OpenAPEGStream(Stream *data_stream, const String &name, int flags, int target_depth);

    std::unique_ptr<Stream> _dataStream;
    int _usedFlags = 0;
    int _usedDepth = 0;
    APEG_STREAM *_apegStream = nullptr;
    // Optional wrapper around original buffer frame (in case we want to extract a portion of it)
    std::unique_ptr<Common::Bitmap> _theoraFullFrame;
    // Wrapper over portion of theora frame which we want to use
    std::unique_ptr<Common::Bitmap> _theoraSrcFrame;
    uint64_t _videoFramesDecodedTotal = 0u; // how many frames loaded and decoded total (includes rewinds!)
    uint64_t _videoFramesDecoded = 0u; // sequential count of video frames since the video beginning
    float _nextFrameTs = 0.f; // next frame presentation time
                              // this is based on previous frame's end pos granule in Ogg stream
};

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_MEDIA__THEORAPLAYER_H
