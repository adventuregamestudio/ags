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

void SOUNDCLIP::seek(int pos_ms)
{
    if (slot_ < 0) { return; }
    audio_core_slot_pause(slot_);
    audio_core_slot_seek_ms(slot_, (float)pos_ms);
    float pos_f, posms_f;
    audio_core_slot_get_play_state(slot_, pos_f, posms_f);
    pos = static_cast<int>(pos_f);
    posMs = static_cast<int>(posms_f);
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

    float pos_f, posms_f;
    PlaybackState core_state = audio_core_slot_get_play_state(slot_, pos_f, posms_f);
    pos = static_cast<int>(pos_f);
    posMs = static_cast<int>(posms_f);
    if (state == core_state || core_state == PlayStateError || core_state == PlayStateFinished)
    {
        state = core_state;
        return is_ready();
    }

    switch (state)
    {
    case PlaybackState::PlayStatePlaying:
        state = audio_core_slot_play(slot_);
        break;
    }
    return is_ready();
}
