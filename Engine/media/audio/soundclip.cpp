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
#include <cmath>
#include "media/audio/soundclip.h"
#include "media/audio/audio_core.h"

SoundClip::SoundClip(int slot, AudioFileType snd_type, bool loop)
    : slot_(slot)
    , soundType(snd_type)
    , repeat(loop)
{
    sourceClipID = -1;
    sourceClipType = 0;
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

    state = PlaybackState::PlayStateInitial;
    pos = posMs = -1;
    paramsChanged = true;

    const auto player = audio_core_get_player(slot);
    lengthMs = (int)std::round(player->GetDurationMs());
    freq = player->GetFrequency();
}

SoundClip::~SoundClip()
{
    if (slot_ >= 0)
        audio_core_slot_stop(slot_);
}

bool SoundClip::play()
{
    if (!is_ready())
        return false;
    state = PlaybackState::PlayStatePlaying;
    return true;
}

void SoundClip::pause()
{
    if (!is_ready())
        return;
    auto player = audio_core_get_player(slot_);
    player->Pause();
    state = player->GetPlayState();
}

void SoundClip::resume()
{
    if (!is_ready())
        return;
    state = PlaybackState::PlayStatePlaying;
}

int SoundClip::posms_to_pos(int pos_ms)
{
    switch (soundType)
    {
    case eAudioFileWAV: // Pos is in samples
        return static_cast<int>((static_cast<int64_t>(pos_ms) * freq) / 1000);
    case eAudioFileMIDI: /* TODO: reimplement */
    case eAudioFileMOD:  /* TODO: reimplement */
        return 0;  /* better say that it does not work than return wrong value */
    default:
        return pos_ms;
    }
}

int SoundClip::pos_to_posms(int pos_)
{
    switch (soundType)
    {
    case eAudioFileWAV: // Pos is in samples
        return static_cast<int>((static_cast<int64_t>(pos_) * 1000) / freq);
    case eAudioFileMIDI: /* TODO: reimplement */
    case eAudioFileMOD:  /* TODO: reimplement */
        return 0;  /* better say that it does not work than return wrong value */
    default:
        return pos_;
    }
}

void SoundClip::seek(int pos_)
{
    // TODO: we probably would need a separate implementation eventually
    seek_ms(pos_to_posms(pos_));
}

void SoundClip::seek_ms(int pos_ms)
{
    if (slot_ < 0) { return; }
    auto player = audio_core_get_player(slot_);
    player->Pause();
    // TODO: for backward compatibility and MOD/XM music support
    // need to reimplement seeking to a position which units
    // are defined according to the sound type
    player->Seek((float)pos_ms);
    float posms_f = player->GetPositionMs();
    posMs = static_cast<int>(posms_f);
    pos = posms_to_pos(posMs);
}

bool SoundClip::update()
{
    if (!is_ready()) return false;

    auto player = audio_core_get_player(slot_);
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

        player->SetVolume(vol_f);
        player->SetSpeed(speed_f);
        player->SetPanning(panning_f);
        paramsChanged = false;
    }

    PlaybackState core_state = player->GetPlayState();
    float posms_f = player->GetPositionMs();
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
        player->Play();
        state = player->GetPlayState();
        break;
    default: /* do nothing */
        break;
    }
    return is_ready();
}
