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
// SOUNDCLIP - an interface for an audio clip configuration and control.
//
// TODO: one of the biggest problems with sound clips currently is that it
// provides several methods of applying volume, which may ignore or override
// each other, and does not shape a consistent interface.
// Improving this situation is only possible with massive refactory of
// sound clip use, taking backwards-compatible audio system in account.
//
//=============================================================================
#ifndef __AGS_EE_MEDIA__SOUNDCLIP_H__
#define __AGS_EE_MEDIA__SOUNDCLIP_H__

struct SOUNDCLIP final
{
    SOUNDCLIP(int slot);
    ~SOUNDCLIP();

    // TODO: move these to private
    int sourceClipID;
    int sourceClipType;
    int soundType; // legacy sound format type (MUS_*)
    bool repeat;
    int lengthMs;
    int priority; // relative clip priority
    int xSource, ySource;
    int maximumPossibleDistanceAway;

    int  play();
    void pause();
    void resume();
    void seek(int);

    inline int play_from(int position)
    {
        seek(position);
        return play();
    }

    bool is_playing(); // true if playing or paused. false if never played or stopped.
    bool is_paused();  // true if paused

    // Get legacy sound format type (MUS_*)
    inline int get_sound_type() const { return soundType; }
    // Gets clip's volume property, as percentage (0 - 100);
    // note this may not be the real volume of playback (which could e.g. be muted)
    inline int get_volume100() const { return vol100; }
    // Gets clip's volume measured in 255 units
    inline int get_volume255() const { return vol255; }
    // Gets clip's panning (-100 - +100)
    inline int get_panning() const { return panning; }
    // Gets clip's playback speed in clip ms per real second
    inline int get_speed() const { return speed; }
    // Gets if clip is muted (without changing the volume setting)
    inline bool is_muted() const { return muted; }

    int get_pos();        // return 0 to indicate seek not supported
    int get_pos_ms();     // this must always return valid value if poss
    int get_length_ms();  // return total track length in ms (or 0)

    // Sets the current volume property, as percentage (0 - 100).
    inline void set_volume100(int volume)
    {
        vol100 = volume;
        vol255 = (volume * 255) / 100;
        adjust_volume();
    }
    // Sets the current volume property in units of 255
    inline void set_volume255(int volume)
    {
        vol255 = volume;
        vol100 = (vol255 * 100) / 255;
        adjust_volume();
    }
    // Explicitly defines both percentage and 255-based volume values,
    // without calculating it from given percentage.
    inline void set_volume_direct(int vol_percent, int vol_absolute)
    {
        vol255 = vol255;
        vol100 = vol_percent;
        adjust_volume();
    }
    // Mutes sound clip, while preserving current volume property
    // for the future reference; when unmuted, that property is
    // used to restart previous volume.
    inline void set_mute(bool enable)
    {
        muted = enable;
        adjust_volume();
    }
    // Apply arbitrary permanent volume modifier, in absolute units (0 - 255);
    // this is distinct value that is used in conjunction with current volume
    // (can be both positive and negative).
    inline void apply_volume_modifier(int mod)
    {
        volModifier = mod;
        adjust_volume();
    }
    // Apply permanent directional volume modifier, in absolute units (0 - 255)
    // this is distinct value that is used in conjunction with current volume
    // (can be both positive and negative).
    inline void apply_directional_modifier(int mod)
    {
        directionalVolModifier = mod;
        adjust_volume();
    }

    void set_panning(int newPanning);
    void set_speed(int new_speed);

private:
    // helper function for calculating volume with applied modifiers
    inline int get_final_volume() const
    {
        if (muted) return 0;
        int final_vol = vol255 + volModifier + directionalVolModifier;
        return final_vol >= 0 ? final_vol : 0;
    }

    void adjust_volume();
    void configure_slot();

    // audio core slot handle
    const int slot_;
    // current volume, in legacy units of 255
    int vol255;
    // current volume, in percents
    int vol100;
    // volModifier is used when there's a need to temporarily change and
    // the restore the clip's absolute volume (vol)
    int volModifier;
    // directional modifier is used to simulate positional sound origin
    int directionalVolModifier;
    // sound panning, in offset percents, from -100 to +100
    int panning;
    // speed of playback, in clip ms per real second
    int speed;
    // mute mode overrides the volume; if set, any volume assigned is stored
    // in properties, but not applied to playback itself
    bool muted;
};

#endif // __AGS_EE_MEDIA__SOUNDCLIP_H__
