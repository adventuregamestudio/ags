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
//
// Video Core: an video backend interface.
//
// TODO:
//     - multiple working threads.
//     - separate thread(s) for buffering, so that it continues even while
//       the videoplayer is being synchronized with the game logic.
//
//=============================================================================
#ifndef __AGS_EE_MEDIA__VIDEOCORE_H
#define __AGS_EE_MEDIA__VIDEOCORE_H
#include <memory>
#include "media/video/videoplayer.h"
#include "util/string.h"
#include "util/threading.h"

// AudioPlayerLock wraps a AudioPlayer pointer guarded by a mutex lock.
// Unlocks the mutex on destruction (e.g. when going out of scope).
typedef AGS::Engine::LockedObjectPtr<AGS::Engine::VideoPlayer>
    VideoPlayerLock;

// Initializes video core system;
// starts polling on a background thread.
void video_core_init(/*config*/);
// Shut downs video core system;
// stops any associated threads.
void video_core_shutdown();

struct VideoInitParams
{
    AGS::Engine::VideoFlags Flags = AGS::Engine::kVideo_None;
    Size TargetSize;
    int TargetColorDepth = 0;
    float FPS = 0.f;
};

// Video slot controls: slots are abstract holders for a playback.
//
// Initializes video playback on a free playback slot, uses provided
// stream for data streaming.
// TODO: return HError on error?
int video_core_slot_init(std::unique_ptr<AGS::Common::Stream> in,
    const AGS::Common::String &name, const AGS::Common::String &ext_hint, const VideoInitParams &params);
// Returns a VideoPlayer from the given slot, wrapped in a auto-locking struct.
VideoPlayerLock video_core_get_player(int slot_handle);
// Stop and release the video player at the given slot
void video_core_slot_stop(int slot_handle);

#if defined(AGS_DISABLE_THREADS)
// Polls the video core if we have no threads, polled in WaitForNextFrame()
void video_core_entry_poll();
#endif

#endif // __AGS_EE_MEDIA__VIDEOCORE_H
