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
//
// FLIC video player implementation.
//
//=============================================================================
#ifndef __AGS_EE_MEDIA__FLICPLAYER_H
#define __AGS_EE_MEDIA__FLICPLAYER_H

#include "media/video/videoplayer.h"

namespace AGS
{
namespace Engine
{

class FlicPlayer : public VideoPlayer
{
public:
    FlicPlayer() = default;
    ~FlicPlayer();

    bool IsValid() override { return _pf != nullptr; }

private:
    Common::HError OpenImpl(std::unique_ptr<Common::Stream> data_stream,
        const String &name, int &flags, int target_depth) override;
    void CloseImpl() override;
    // Retrieves next video frame, implementation-specific
    bool NextVideoFrame(Common::Bitmap *dst) override;

    PACKFILE *_pf = nullptr;
    RGB _oldpal[256]{};
    uint64_t _videoFramesDecoded = 0u; // how many frames loaded and decoded
};

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_MEDIA__FLICPLAYER_H
