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
#include "util/string.h"

namespace AGS
{
namespace Engine
{

using AGS::Common::String;

// Parent video player class, provides basic playback logic,
// while relying on frame decoding being implemented in derived classes.
class VideoPlayer
{
public:
    virtual ~VideoPlayer();
    // Tries to open a video file of a given name
    bool Open(const String &name, int skip, int flags);
    // Stops the playback, releasing any resources
    void Close();
    // Restores the video after display switch
    virtual void Restore() {};

    // Tells if video playback is looping
    bool IsLooping() const { return _loop; }

    // Updates the video playback
    bool Poll();

protected:
    // Opens the video, implementation-specific
    virtual bool OpenImpl(const String &name, int flags) { return false; };
    // Closes the video, implementation-specific
    virtual void CloseImpl() {};
    // Retrieves next video frame, implementation-specific
    virtual bool NextFrame() { return false; };

    // Renders the current frame
    bool Render();

private:
    bool _loop = false;
    int _flags = 0;
    int _skip = 0;
    uint32_t _sdlTimer = 0u;
};

} // namespace Engine
} // namespace AGS


void play_theora_video(const char *name, int skip, int flags);
void play_flc_file(int numb,int playflags);

// Update video playback if the display mode has changed
void video_on_gfxmode_changed();
// Stop current playback and dispose all video resource
void video_shutdown();

#endif // __AGS_EE_MEDIA__VIDEO_H
