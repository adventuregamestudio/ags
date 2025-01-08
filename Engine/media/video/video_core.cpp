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
#include "media/video/video_core.h"
#include <condition_variable>
#include <mutex>
#include <thread>
#include <unordered_map>
#include "debug/out.h"
#include "media/video/videoplayer.h"
#include "media/video/flic_player.h"
#include "media/video/theora_player.h"

using namespace AGS::Common;
using namespace AGS::Engine;

#ifndef AGS_NO_VIDEO_PLAYER

// Global audio core state and resources
static struct 
{
    // Audio thread: polls sound decoders, feeds OpenAL sources
    std::thread video_core_thread;
    bool video_core_thread_running = false;

    // Slot id counter
    int nextId = 0;

    // One mutex to lock them all... any operation on individual players
    // is done under this only mutex, which means that they are currently
    // polled one by one, any action like pause/resume is also synced.
    std::mutex poll_mutex_m;
    std::condition_variable poll_cv;
    std::unordered_map<int, std::unique_ptr<VideoPlayer>> slots_;
} g_vcore;

static void video_core_entry();

// -------------------------------------------------------------------------------------------------
// INIT / SHUTDOWN
// -------------------------------------------------------------------------------------------------

void video_core_init()
{
    if (g_vcore.video_core_thread_running)
        return; // already running

    g_vcore.video_core_thread_running = true;
#if !defined(AGS_DISABLE_THREADS)
    g_vcore.video_core_thread = std::thread(video_core_entry);
#endif
    Debug::Printf(kDbgMsg_Info, "VideoCore: init thread");
}

void video_core_shutdown()
{
    if (!g_vcore.video_core_thread_running)
        return; // not running

    Debug::Printf(kDbgMsg_Info, "VideoCore: shutting down...");
    g_vcore.video_core_thread_running = false;
#if !defined(AGS_DISABLE_THREADS)
    if (g_vcore.video_core_thread.joinable())
        g_vcore.video_core_thread.join();
#endif

    // dispose all the active slots
    g_vcore.slots_.clear();
    Debug::Printf(kDbgMsg_Info, "VideoCore: shutdown");
}


// -------------------------------------------------------------------------------------------------
// SLOTS
// -------------------------------------------------------------------------------------------------

static int avail_slot_id()
{
    return g_vcore.nextId++;
}

static int video_core_slot_init(std::unique_ptr<VideoPlayer> player)
{
    auto handle = avail_slot_id();
    std::lock_guard<std::mutex> lk(g_vcore.poll_mutex_m);
    g_vcore.slots_[handle] = std::move(player);
    g_vcore.poll_cv.notify_all();
    return handle;
}

static std::unique_ptr<VideoPlayer> create_video_player(const AGS::Common::String &ext_hint)
{
    std::unique_ptr<VideoPlayer> player;
    // Table of video format detection
    if (ext_hint.CompareNoCase("flc") == 0 ||
        ext_hint.CompareNoCase("fli") == 0)
        player.reset(new FlicPlayer());
    else if (ext_hint.CompareNoCase("ogv") == 0)
        player.reset(new TheoraPlayer());
    else
        return nullptr; // not supported
    return player;
}

int video_core_slot_init(std::unique_ptr<AGS::Common::Stream> in,
    const String &name, const AGS::Common::String &ext_hint, const VideoInitParams &params)
{
    auto player = create_video_player(ext_hint);
    if (!player)
        return -1;
    if (!player->Open(std::move(in), name,
            params.Flags, params.TargetSize, params.TargetColorDepth, params.FPS))
        return -1;
    return video_core_slot_init(std::move(player));
}

VideoPlayerLock video_core_get_player(int slot_handle)
{
    std::unique_lock<std::mutex> ulk(g_vcore.poll_mutex_m);
    auto it = g_vcore.slots_.find(slot_handle);
    if (it == g_vcore.slots_.end())
        return VideoPlayerLock(nullptr, std::move(ulk), &g_vcore.poll_cv);
    return VideoPlayerLock(it->second.get(), std::move(ulk), &g_vcore.poll_cv);
}

void video_core_slot_stop(int slot_handle)
{
    std::lock_guard<std::mutex> lk(g_vcore.poll_mutex_m);
    g_vcore.slots_[slot_handle]->Stop();
    g_vcore.slots_.erase(slot_handle);
    g_vcore.poll_cv.notify_all();
}

// -------------------------------------------------------------------------------------------------
// VIDEO PROCESSING
// -------------------------------------------------------------------------------------------------

void video_core_entry_poll()
{
    for (auto &entry : g_vcore.slots_) {
        auto &slot = entry.second;

        try {
            slot->Poll();
        } catch (const std::exception& e) {
            Debug::Printf(kDbgMsg_Error, "VideoCore poll exception: %s", e.what());
        }
    }
}

#if !defined(AGS_DISABLE_THREADS)
static void video_core_entry()
{
    std::unique_lock<std::mutex> lk(g_vcore.poll_mutex_m);

    while (g_vcore.video_core_thread_running) {

        video_core_entry_poll();

        g_vcore.poll_cv.wait_for(lk, std::chrono::milliseconds(8));
    }
}
#endif // !AGS_DISABLE_THREADS

#else // AGS_NO_VIDEO_PLAYER

void video_core_init(/*config*/)
{
    Debug::Printf(kDbgMsg_Warn, "VideoCore: video playback is not supported in this engine build.");
}
void video_core_shutdown() { }
int video_core_slot_init(std::unique_ptr<AGS::Common::Stream>,
    const AGS::Common::String &, const AGS::Common::String &, const VideoInitParams &)
{
    return -1;
}
VideoPlayerLock video_core_get_player(int)
{
    throw std::runtime_error("Video playback is not supported in this engine build.");
}
void video_core_slot_stop(int) {}

#if defined(AGS_DISABLE_THREADS)
void video_core_entry_poll() {}
#endif

#endif // !AGS_NO_VIDEO_PLAYER
