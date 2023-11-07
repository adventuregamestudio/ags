//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#ifndef __AC_AUDIODEFINES_H
#define __AC_AUDIODEFINES_H

// Media playback status
enum PlaybackState
{
    PlayStateInvalid,   // was not initialized
    PlayStateInitial,   // initialized, but not started
    PlayStatePlaying,   // playing
    PlayStatePaused,    // paused
    PlayStateStopped,   // stopped by command
    PlayStateFinished,  // stopped by reaching the end
    PlayStateError      // stopped due to the error
};

// Tells if the playback state defines a valid and ready state
inline bool IsPlaybackReady(PlaybackState state)
{
    return state == PlayStateInitial || state == PlayStatePlaying || state == PlayStatePaused;
}

// Tells if the playback state defines an invalid or completed state
inline bool IsPlaybackDone(PlaybackState state)
{
    return state == PlayStateInvalid || state == PlayStateStopped ||
        state == PlayStateFinished || state == PlayStateError;
}

// Max channels that are distributed among game's audio types
#define MAX_GAME_CHANNELS         16
#define SPECIAL_CROSSFADE_CHANNEL (MAX_GAME_CHANNELS)
// Total number of channels: game chans + utility chans
#define TOTAL_AUDIO_CHANNELS      (MAX_GAME_CHANNELS + 1)
// Number of game channels reserved for speech voice-over
#define NUM_SPEECH_CHANS          1
// Fixed audio channel ID used for speech
#define SCHAN_SPEECH              0

// Used in locational sounds (FIXME: rename)
#define AMBIENCE_FULL_DIST 25

#endif // __AC_AUDIODEFINES_H