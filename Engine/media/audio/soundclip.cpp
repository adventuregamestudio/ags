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
#include "media/audio/soundclip.h"
#include "media/audio/audio_core.h"

SOUNDCLIP::SOUNDCLIP(int slot)
    : slot_(slot)
{
    sourceClipID = -1;
    sourceClipType = 0;
    soundType = 0;
    repeat = false;
    priority = 50;
    vol255 = 0;
    vol100 = 0;
    volModifier = 0;
    xSource = -1;
    ySource = -1;
    maximumPossibleDistanceAway = 0;
    directionalVolModifier = 0;
    muted = false;
    panning = 0;
    speed = 1000;

    lengthMs = 0;
    state = PlaybackState::PlayStateInitial;
    pos = posMs = -1;
    paramsChanged = true;

    freq = audio_core_slot_get_freq(slot_);
}

SOUNDCLIP::~SOUNDCLIP()
{
    if (slot_ >= 0)
        audio_core_slot_stop(slot_);
}

int SOUNDCLIP::play()
{
    if (!is_ready())
        return false;
    state = PlaybackState::PlayStatePlaying;
    return true;
}

void SOUNDCLIP::pause()
{
    if (!is_ready())
        return;
    state = audio_core_slot_pause(slot_);
}

void SOUNDCLIP::resume()
{
    if (!is_ready())
        return;
    state = PlaybackState::PlayStatePlaying;
}

int SOUNDCLIP::posms_to_pos(int pos_ms)
{
    switch (soundType)
    {
    case MUS_WAVE: // Pos is in samples
        return static_cast<int>((static_cast<int64_t>(pos_ms) * freq) / 1000);
    case MUS_MIDI: /* TODO: reimplement */
    case MUS_MOD:  /* TODO: reimplement */
        return 0;  /* better say that it does not work than return wrong value */
    default:
        return pos_ms;
    }
}

int SOUNDCLIP::pos_to_posms(int pos)
{
    switch (soundType)
    {
    case MUS_WAVE: // Pos is in samples
        return static_cast<int>((static_cast<int64_t>(pos) * 1000) / freq);
    case MUS_MIDI: /* TODO: reimplement */
    case MUS_MOD:  /* TODO: reimplement */
        return 0;  /* better say that it does not work than return wrong value */
    default:
        return pos;
    }
}

void SOUNDCLIP::seek(int pos)
{
    // TODO: we probably would need a separate implementation eventually
    seek_ms(pos_to_posms(pos));
}

void SOUNDCLIP::seek_ms(int pos_ms)
{
    if (slot_ < 0) { return; }
    audio_core_slot_pause(slot_);
    // TODO: for backward compatibility and MOD/XM music support
    // need to reimplement seeking to a position which units
    // are defined according to the sound type
    audio_core_slot_seek_ms(slot_, (float)pos_ms);
    float posms_f;
    audio_core_slot_get_play_state(slot_, posms_f);
    posMs = static_cast<int>(posms_f);
    pos = posms_to_pos(posMs);
}

bool SOUNDCLIP::update()
{
    if (!is_ready()) return false;

    if (paramsChanged)
    {
        auto vol_f = static_cast<float>(get_final_volume()) / 255.0f;
        if (vol_f < 0.0f) { vol_f = 0.0f; }
        if (vol_f > 1.0f) { vol_f = 1.0f; }

        auto speed_f = static_cast<float>(speed) / 1000.0f;
        if (speed_f <= 0.0) { speed_f = 1.0f; }

        // Sets the pan position, ranging from -100 (left) to +100 (right)
        auto panning_f = (static_cast<float>(panning) / 100.0f);
        if (panning_f < -1.0f) { panning_f = -1.0f; }
        if (panning_f > 1.0f) { panning_f = 1.0f; }

        audio_core_slot_configure(slot_, vol_f, speed_f, panning_f);
        paramsChanged = false;
    }

    float posms_f;
    PlaybackState core_state = audio_core_slot_get_play_state(slot_, posms_f);
    posMs = static_cast<int>(posms_f);
    pos = posms_to_pos(posMs);
    if (state == core_state || IsPlaybackDone(core_state))
    {
        state = core_state;
        return is_ready();
    }

    switch (state)
    {
    case PlaybackState::PlayStatePlaying:
        state = audio_core_slot_play(slot_);
        break;
    default: /* do nothing */
        break;
    }
    return is_ready();
}
