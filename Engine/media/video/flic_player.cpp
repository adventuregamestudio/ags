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
#include "media/video/flic_player.h"

#ifndef AGS_NO_VIDEO_PLAYER

#include <inttypes.h>
#include "ac/asset_helper.h"
#include "debug/out.h"

namespace AGS
{
namespace Engine
{

using namespace Common;

FlicPlayer::~FlicPlayer()
{
    CloseImpl();
}

HError FlicPlayer::OpenImpl(std::unique_ptr<Common::Stream> data_stream,
    const String &/*name*/, int& flags, int /*target_depth*/)
{
    data_stream->Seek(8);
    const int fliwidth = data_stream->ReadInt16();
    const int fliheight = data_stream->ReadInt16();
    data_stream->Seek(0, kSeekBegin);

    PACKFILE *pf = PackfileFromStream(std::move(data_stream));
    if (open_fli_pf(pf) != FLI_OK)
    {
        pack_fclose(pf);
        return new Error("Failed to open FLI/FLC animation; could be an invalid or unsupported format");
    }
    _pf = pf;

    Debug::Printf("FlicPlayer: opened video: %dx%d 8-bit, fps: %d", fliwidth, fliheight, 1000 / fli_speed);

    get_palette_range(_oldpal, 0, 255);

    _frameDepth = 8;
    _frameSize = Size(fliwidth, fliheight);
    _frameRate = 1000.f / fli_speed;
    _frameTime = fli_speed;
    _frameCount = static_cast<uint32_t>(fli_frame_count);
    _durationMs = fli_frame_count * fli_speed;
    // FLIC must accumulate frame image because its frames contain diff since the last frame
    flags |= kVideo_AccumFrame;
    _videoFramesDecoded = 0u;
    return HError::None();
}

void FlicPlayer::CloseImpl()
{
    if (fli_bitmap) // just a way to test that FLI was opened
    {
        Debug::Printf("FlicPlayer: closed, total video frames decoded: %" PRIu64 "", _videoFramesDecoded);
        _videoFramesDecoded = 0u;
    }

    close_fli();
    if (_pf)
        pack_fclose(_pf);
    _pf = nullptr;

    set_palette_range(_oldpal, 0, 255, 0);
}

bool FlicPlayer::NextVideoFrame(Bitmap *dst, float &ts)
{
    ts = -1.f; // reset in case of error

    // actual FLI playback state, base on original Allegro 4's do_play_fli

    /* get next frame */
    if (next_fli_frame(IsLooping() ? 1 : 0) != FLI_OK)
        return false;

    /* update the palette */
    if (fli_pal_dirty_from <= fli_pal_dirty_to)
        set_palette_range(fli_palette, fli_pal_dirty_from, fli_pal_dirty_to, TRUE);

    /* blit the changed portion of the frame */
    if (fli_bmp_dirty_from <= fli_bmp_dirty_to) {
        blit(fli_bitmap, dst->GetAllegroBitmap(), 0, fli_bmp_dirty_from, 0, fli_bmp_dirty_from,
            fli_bitmap->w, 1 + fli_bmp_dirty_to - fli_bmp_dirty_from);
    }

    ts = _videoFramesDecoded * _frameTime;
    _videoFramesDecoded++;
    reset_fli_variables();
    return true;
}

float FlicPlayer::PeekVideoFrame()
{
    if (fli_frame < fli_frame_count)
        return _videoFramesDecoded * _frameTime;
    return -1.f;
}

// Drop next video frame from stream.
void FlicPlayer::DropVideoFrame()
{
    skip_fli_frame(IsLooping() ? 1 : 0);
}

} // namespace Engine
} // namespace AGS

#endif // AGS_NO_VIDEO_PLAYER
