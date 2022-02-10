//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
//
// Video playback interface
//
//=============================================================================
#ifndef __AGS_EE_MEDIA__VIDEO_H
#define __AGS_EE_MEDIA__VIDEO_H
#include <atomic>
#include "util/geometry.h"
#include "util/string.h"
#include "media/audio/openalsource.h"

namespace AGS
{

namespace Common { class Bitmap; }

namespace Engine
{

enum VideoFlags
{
    kVideo_EnableVideo    = 0x0001,
    kVideo_Stretch        = 0x0002,
    kVideo_ClearScreen    = 0x0004,
    kVideo_EnableAudio    = 0x0010,
};

enum VideoSkipType
{
    VideoSkipNone         = 0,
    VideoSkipEscape       = 1,
    VideoSkipAnyKey       = 2,
    VideoSkipKeyOrMouse   = 3
};

class IDriverDependantBitmap;
using AGS::Common::Bitmap;
using AGS::Common::String;

// Parent video player class, provides basic playback logic,
// while relying on frame decoding being implemented in derived classes.
class VideoPlayer
{
public:
    virtual ~VideoPlayer();
    // Tries to open a video file of a given name
    bool Open(const AGS::Common::String &name, int flags, VideoSkipType skip);
    // Stops the playback, releasing any resources
    void Close();
    // Restores the video after display switch
    virtual void Restore() {};

    // Tells if video playback is looping
    bool IsLooping() const { return _loop; }

    // Updates the video playback
    bool Poll();

protected:
    // Opens the video, implementation-specific; allows to modify flags
    virtual bool OpenImpl(const String &name, int &flags) { return false; };
    // Closes the video, implementation-specific
    virtual void CloseImpl() {};
    // Retrieves next video frame, implementation-specific
    virtual bool NextFrame() { return false; };

    int GetAudioPos(); // in ms

    int _audioChannels = 0;
    int _audioFreq = 0;
    SDL_AudioFormat _audioFormat = 0;
    SoundBuffer _audioFrame{};
    bool _wantAudio = false;

    std::unique_ptr<Bitmap> _videoFrame;
    std::unique_ptr<Bitmap> _hicolBuf;
    std::unique_ptr<Bitmap> _targetBitmap;
    IDriverDependantBitmap *_videoDDB = nullptr;
    Size _targetSize{};
    uint32_t _frameTime = 0u;

private:
    // Timer callback, triggers next frame
    static uint32_t VideoTimerCallback(uint32_t interval, void *param);
    // Checks input events, tells if the video should be skipped
    bool CheckUserInputSkip();
    // Renders the current audio data
    bool RenderAudio();
    // Renders the current video frame
    bool RenderVideo();

    bool _loop = false;
    int _flags = 0;
    VideoSkipType _skip = VideoSkipNone;
    uint32_t _sdlTimer = 0u;
    std::atomic<int> _timerPos{};
    std::unique_ptr<OpenAlSource> _audioOut;
};

} // namespace Engine
} // namespace AGS


void play_theora_video(const char *name, int flags, AGS::Engine::VideoSkipType skip);
void play_flc_video(int numb, int flags, AGS::Engine::VideoSkipType skip);

// Update video playback if the display mode has changed
void video_on_gfxmode_changed();
// Stop current playback and dispose all video resource
void video_shutdown();

#endif // __AGS_EE_MEDIA__VIDEO_H
