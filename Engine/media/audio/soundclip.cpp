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
}

SOUNDCLIP::~SOUNDCLIP()
{
    if (slot_ >= 0)
        audio_core_slot_stop(slot_);
}

int SOUNDCLIP::play()
{
    if (slot_ < 0) { return 0; }
    configure_slot(); // volume, speed, panning, repeat
    audio_core_slot_play(slot_);
    return 1;
}

void SOUNDCLIP::pause()
{
    if (slot_ < 0) { return; }
    audio_core_slot_pause(slot_);
}

void SOUNDCLIP::resume()
{
    if (slot_ < 0) { return; }
    audio_core_slot_play(slot_);
}

bool SOUNDCLIP::is_playing()
{
    if (slot_ < 0) { return false; }
    auto status = audio_core_slot_get_play_state(slot_);
    switch (status) {
    case PlayStateInitial:
    case PlayStatePlaying:
    case PlayStatePaused:
        return true;
    }
    return false;
}

bool SOUNDCLIP::is_paused()
{
    if (slot_ < 0) { return false; }
    auto status = audio_core_slot_get_play_state(slot_);
    return status == PlayStatePaused;
}

void SOUNDCLIP::seek(int pos_ms)
{
    if (slot_ < 0) { return; }
    audio_core_slot_seek_ms(slot_, (float)pos_ms);
}

void SOUNDCLIP::configure_slot()
{
    if (slot_ < 0) { return; }

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
}

void SOUNDCLIP::set_speed(int new_speed)
{
    speed = new_speed;
    configure_slot();
}

void SOUNDCLIP::set_panning(int newPanning)
{
    panning = newPanning;
    configure_slot();
}

void SOUNDCLIP::adjust_volume()
{
    configure_slot();
}

int SOUNDCLIP::get_pos()
{
    // until we can figure out other details, pos will always be in milliseconds
    return get_pos_ms();
}

int SOUNDCLIP::get_pos_ms()
{
    if (slot_ < 0) { return -1; }
    // given how unaccurate mp3 length is, maybe we should do differently here
    // on the other hand, if on repeat, maybe better to just return an infinitely increasing position?
    // but on the other other hand, then we can't always seek to that position.
    if (lengthMs <= 0.0f) {
        return (int)std::round(audio_core_slot_get_pos_ms(slot_));
    }
    return (int)std::round(fmod(audio_core_slot_get_pos_ms(slot_), lengthMs));
}

int SOUNDCLIP::get_length_ms()
{
    return lengthMs;
}
