// AGS Editor ImGui - Simple audio preview player using SDL2 audio
// Supports WAV, OGG (via SDL2 audio callback).
// For editor preview only — not for game runtime.
#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <SDL.h>

namespace AGSEditor
{

enum class AudioPlayerState
{
    Stopped,
    Playing,
    Paused
};

class AudioPlayer
{
public:
    AudioPlayer();
    ~AudioPlayer();

    // Initialize SDL audio subsystem
    bool Init();
    void Shutdown();

    // Load and play an audio file
    bool LoadWAV(const std::string& filepath);
    bool LoadOGG(const std::string& filepath);

    // Load any supported audio file (auto-detects format from extension)
    bool LoadFile(const std::string& filepath);

    // Playback controls
    void Play();
    void Pause();
    void Stop();

    // State
    AudioPlayerState GetState() const { return state_; }
    bool IsPlaying() const { return state_ == AudioPlayerState::Playing; }

    // Call every frame to detect playback completion and pause device
    void Update();

    // Playback progress (0.0 - 1.0)
    float GetProgress() const;

    // Duration in seconds
    float GetDuration() const;

    // Current position in seconds
    float GetPosition() const;

    // Get raw audio samples for waveform rendering (mono, normalized to -1..1)
    const std::vector<float>& GetWaveformData() const { return waveform_; }

    // Get the loaded filename
    const std::string& GetLoadedFile() const { return loaded_file_; }

private:
    static void AudioCallback(void* userdata, Uint8* stream, int len);

    SDL_AudioDeviceID device_ = 0;
    SDL_AudioSpec audio_spec_{};

    // Audio buffer
    std::vector<Uint8> audio_buffer_;
    Uint32 audio_pos_ = 0;
    Uint32 audio_len_ = 0;

    // Waveform data for visualization
    std::vector<float> waveform_;

    AudioPlayerState state_ = AudioPlayerState::Stopped;
    std::string loaded_file_;
    bool initialized_ = false;

    void GenerateWaveform();
};

} // namespace AGSEditor
