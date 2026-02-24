// AGS Editor ImGui - Simple audio preview player implementation
#include "audio_player.h"
#include <cstring>
#include <cstdio>
#include <cmath>
#include <algorithm>

// Include minivorbis header (implementation is in ogg_vorbis_impl.c)
#include "minivorbis/minivorbis.h"

namespace AGSEditor
{

AudioPlayer::AudioPlayer() = default;

AudioPlayer::~AudioPlayer()
{
    Shutdown();
}

bool AudioPlayer::Init()
{
    if (initialized_) return true;

    if (!(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO))
    {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
        {
            fprintf(stderr, "[AudioPlayer] Failed to init SDL audio: %s\n", SDL_GetError());
            return false;
        }
    }
    initialized_ = true;
    return true;
}

void AudioPlayer::Shutdown()
{
    Stop();
    if (device_)
    {
        SDL_CloseAudioDevice(device_);
        device_ = 0;
    }
    initialized_ = false;
}

bool AudioPlayer::LoadWAV(const std::string& filepath)
{
    if (!Init()) return false;

    // Stop any current playback
    Stop();

    // Close previous device
    if (device_)
    {
        SDL_CloseAudioDevice(device_);
        device_ = 0;
    }

    // Load WAV file
    SDL_AudioSpec wav_spec;
    Uint8* wav_buf = nullptr;
    Uint32 wav_len = 0;

    if (!SDL_LoadWAV(filepath.c_str(), &wav_spec, &wav_buf, &wav_len))
    {
        fprintf(stderr, "[AudioPlayer] Failed to load WAV '%s': %s\n",
                filepath.c_str(), SDL_GetError());
        return false;
    }

    // Open audio device
    SDL_AudioSpec desired;
    SDL_zero(desired);
    desired.freq = wav_spec.freq;
    desired.format = wav_spec.format;
    desired.channels = wav_spec.channels;
    desired.samples = 4096;
    desired.callback = AudioCallback;
    desired.userdata = this;

    device_ = SDL_OpenAudioDevice(nullptr, 0, &desired, &audio_spec_, 0);
    if (!device_)
    {
        fprintf(stderr, "[AudioPlayer] Failed to open audio device: %s\n", SDL_GetError());
        SDL_FreeWAV(wav_buf);
        return false;
    }

    // Copy WAV data
    audio_buffer_.assign(wav_buf, wav_buf + wav_len);
    audio_len_ = wav_len;
    audio_pos_ = 0;
    SDL_FreeWAV(wav_buf);

    loaded_file_ = filepath;

    // Generate waveform data for visualization
    GenerateWaveform();

    fprintf(stderr, "[AudioPlayer] Loaded '%s': %d Hz, %d ch, %d bytes\n",
            filepath.c_str(), audio_spec_.freq, audio_spec_.channels, (int)audio_len_);
    return true;
}

bool AudioPlayer::LoadOGG(const std::string& filepath)
{
    if (!Init()) return false;

    // Stop any current playback
    Stop();

    // Close previous device
    if (device_)
    {
        SDL_CloseAudioDevice(device_);
        device_ = 0;
    }

    // Open OGG file with minivorbis
    FILE* fp = fopen(filepath.c_str(), "rb");
    if (!fp)
    {
        fprintf(stderr, "[AudioPlayer] Failed to open OGG file: %s\n", filepath.c_str());
        return false;
    }

    OggVorbis_File vorbis;
    if (ov_open_callbacks(fp, &vorbis, nullptr, 0, OV_CALLBACKS_DEFAULT) != 0)
    {
        fprintf(stderr, "[AudioPlayer] Invalid OGG file: %s\n", filepath.c_str());
        fclose(fp);
        return false;
    }

    vorbis_info* info = ov_info(&vorbis, -1);
    if (!info)
    {
        fprintf(stderr, "[AudioPlayer] Cannot read OGG info: %s\n", filepath.c_str());
        ov_clear(&vorbis);
        return false;
    }

    int sample_rate = info->rate;
    int channels = info->channels;

    // Read entire OGG stream to PCM (signed 16-bit, little-endian)
    std::vector<Uint8> pcm_data;
    char decode_buf[4096];
    int bitstream = 0;
    while (true)
    {
        long bytes = ov_read(&vorbis, decode_buf, sizeof(decode_buf), 0, 2, 1, &bitstream);
        if (bytes <= 0)
            break;
        pcm_data.insert(pcm_data.end(), decode_buf, decode_buf + bytes);
    }
    ov_clear(&vorbis);

    if (pcm_data.empty())
    {
        fprintf(stderr, "[AudioPlayer] No PCM data decoded from OGG: %s\n", filepath.c_str());
        return false;
    }

    // Open audio device matching the decoded format
    SDL_AudioSpec desired;
    SDL_zero(desired);
    desired.freq = sample_rate;
    desired.format = AUDIO_S16LSB;
    desired.channels = (Uint8)channels;
    desired.samples = 4096;
    desired.callback = AudioCallback;
    desired.userdata = this;

    device_ = SDL_OpenAudioDevice(nullptr, 0, &desired, &audio_spec_, 0);
    if (!device_)
    {
        fprintf(stderr, "[AudioPlayer] Failed to open audio device: %s\n", SDL_GetError());
        return false;
    }

    audio_buffer_ = std::move(pcm_data);
    audio_len_ = (Uint32)audio_buffer_.size();
    audio_pos_ = 0;

    loaded_file_ = filepath;
    GenerateWaveform();

    fprintf(stderr, "[AudioPlayer] Loaded OGG '%s': %d Hz, %d ch, %d bytes PCM\n",
            filepath.c_str(), sample_rate, channels, (int)audio_len_);
    return true;
}

bool AudioPlayer::LoadFile(const std::string& filepath)
{
    // Determine format from extension
    std::string lower = filepath;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower.size() >= 4 && lower.substr(lower.size() - 4) == ".ogg")
        return LoadOGG(filepath);
    if (lower.size() >= 4 && lower.substr(lower.size() - 4) == ".wav")
        return LoadWAV(filepath);

    fprintf(stderr, "[AudioPlayer] Unsupported audio format: %s\n", filepath.c_str());
    return false;
}

void AudioPlayer::Play()
{
    if (!device_) return;
    // If we reached the end, reset position to replay from start
    if (audio_pos_ >= audio_len_)
        audio_pos_ = 0;
    state_ = AudioPlayerState::Playing;
    SDL_PauseAudioDevice(device_, 0);
}

void AudioPlayer::Pause()
{
    if (!device_) return;
    state_ = AudioPlayerState::Paused;
    SDL_PauseAudioDevice(device_, 1);
}

void AudioPlayer::Stop()
{
    if (device_)
        SDL_PauseAudioDevice(device_, 1);
    audio_pos_ = 0;
    state_ = AudioPlayerState::Stopped;
}

void AudioPlayer::Update()
{
    // Detect when audio callback set state to Stopped (playback reached end)
    // and pause the device to avoid unnecessary silence callbacks
    if (device_ && state_ == AudioPlayerState::Stopped && audio_pos_ >= audio_len_ && audio_len_ > 0)
    {
        SDL_PauseAudioDevice(device_, 1);
    }
}

float AudioPlayer::GetProgress() const
{
    if (audio_len_ == 0) return 0.0f;
    return (float)audio_pos_ / (float)audio_len_;
}

float AudioPlayer::GetDuration() const
{
    if (audio_spec_.freq == 0 || audio_spec_.channels == 0) return 0.0f;
    int bytes_per_sample = SDL_AUDIO_BITSIZE(audio_spec_.format) / 8;
    if (bytes_per_sample == 0) return 0.0f;
    float total_samples = (float)audio_len_ / (float)(bytes_per_sample * audio_spec_.channels);
    return total_samples / (float)audio_spec_.freq;
}

float AudioPlayer::GetPosition() const
{
    return GetProgress() * GetDuration();
}

void AudioPlayer::AudioCallback(void* userdata, Uint8* stream, int len)
{
    auto* self = static_cast<AudioPlayer*>(userdata);

    if (self->audio_pos_ >= self->audio_len_)
    {
        // Reached end — fill with silence and stop
        SDL_memset(stream, 0, (size_t)len);
        self->state_ = AudioPlayerState::Stopped;
        return;
    }

    Uint32 remaining = self->audio_len_ - self->audio_pos_;
    Uint32 to_copy = (Uint32)len < remaining ? (Uint32)len : remaining;

    SDL_memcpy(stream, self->audio_buffer_.data() + self->audio_pos_, to_copy);
    self->audio_pos_ += to_copy;

    // Fill remaining with silence if we ran out
    if (to_copy < (Uint32)len)
        SDL_memset(stream + to_copy, 0, (size_t)(len - to_copy));
}

void AudioPlayer::GenerateWaveform()
{
    waveform_.clear();

    if (audio_buffer_.empty() || audio_spec_.channels == 0) return;

    int bytes_per_sample = SDL_AUDIO_BITSIZE(audio_spec_.format) / 8;
    if (bytes_per_sample == 0) return;

    int num_samples = (int)(audio_len_ / (bytes_per_sample * audio_spec_.channels));
    if (num_samples == 0) return;

    // Downsample to ~1000 points for waveform display
    const int target_points = 1000;
    int step = std::max(1, num_samples / target_points);

    waveform_.reserve(target_points);

    for (int i = 0; i < num_samples; i += step)
    {
        float sample = 0.0f;
        size_t offset = (size_t)i * bytes_per_sample * audio_spec_.channels;

        if (offset + bytes_per_sample > audio_buffer_.size()) break;

        // Read first channel only
        if (audio_spec_.format == AUDIO_S16LSB || audio_spec_.format == AUDIO_S16SYS)
        {
            int16_t s;
            memcpy(&s, audio_buffer_.data() + offset, 2);
            sample = (float)s / 32768.0f;
        }
        else if (audio_spec_.format == AUDIO_U8)
        {
            sample = ((float)audio_buffer_[offset] - 128.0f) / 128.0f;
        }
        else if (audio_spec_.format == AUDIO_S32LSB || audio_spec_.format == AUDIO_S32SYS)
        {
            int32_t s;
            memcpy(&s, audio_buffer_.data() + offset, 4);
            sample = (float)s / 2147483648.0f;
        }
        else if (audio_spec_.format == AUDIO_F32LSB || audio_spec_.format == AUDIO_F32SYS)
        {
            memcpy(&sample, audio_buffer_.data() + offset, 4);
        }
        else
        {
            // Unknown format — treat as silence
            sample = 0.0f;
        }

        waveform_.push_back(sample);
    }
}

} // namespace AGSEditor
