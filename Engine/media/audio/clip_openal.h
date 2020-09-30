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
// SOUNDCLIP implementation using OpenAL.
// Supports following formats:
//   - MIDI (?)
//   - MOD (?)
//   - MP3
//   - OGG
//   - WAV
//
//=============================================================================
#ifndef __AGS_EE_MEDIA__CLIP_OPENAL_H__
#define __AGS_EE_MEDIA__CLIP_OPENAL_H__
#include "media/audio/soundclip.h"
#include "ac/asset_helper.h"

struct OPENAL_SOUNDCLIP final : public SOUNDCLIP
{
public:
    OPENAL_SOUNDCLIP();
    ~OPENAL_SOUNDCLIP();
    void destroy() override;

    virtual int get_sound_type() override { return soundType; }

    int play() override;
    virtual int play_from(int position) override;
    void pause() override;
    void resume() override;
    bool is_playing() const override;

    void seek(int pos) override;

    void poll() override;

    void set_volume(int newvol) override;
    void set_speed(int new_speed) override;
    void set_panning(int newPanning) override;

    int get_pos() override;    
    int get_pos_ms() override;
    int get_length_ms() override;

    // TODO: make these private
    int slot_ = -1;
    int lengthMs = -1;
    int soundType = 0;

protected:
    void adjust_volume() override;
    int get_voice() override { return 0; } // only for Allegro-specific implementations

private:
    void configure_slot();
};


// Factory methods
SOUNDCLIP *my_load_openal(const AssetPath &asset_name, const char *extension_hint, int voll, bool loop);

#endif // __AGS_EE_MEDIA__CLIP_OPENAL_H__
