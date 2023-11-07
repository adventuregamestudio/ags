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
#include "media/audio/sdldecoder.h"
#include "util/sdl2_util.h"

namespace AGS
{
namespace Engine
{

//-----------------------------------------------------------------------------
// SDLResampler
//-----------------------------------------------------------------------------
bool SDLResampler::Setup(SDL_AudioFormat src_fmt, int src_chans, int src_rate,
    SDL_AudioFormat dst_fmt, int dst_chans, int dst_rate)
{
    SDL_zero(_cvt);
    return SDL_BuildAudioCVT(&_cvt, src_fmt, static_cast<uint8_t>(src_chans), src_rate,
        dst_fmt, static_cast<uint8_t>(dst_chans), dst_rate) >= 0;
}

const void *SDLResampler::Convert(const void *data, size_t sz, size_t &out_sz)
{
    if (_cvt.needed == 0)
    { // no conversion necessary, return the input pointer
        out_sz = sz;
        return data;
    }
    // Reallocate if necessary and perform the conversion
    if (_buf.size() < sz * _cvt.len_mult)
    {
        size_t len = sz * _cvt.len_mult;
        _buf.resize(len);
    }
    _cvt.buf = &_buf[0];
    _cvt.len = _cvt.len_cvt = sz;
    SDL_memcpy(_cvt.buf, data, sz);
    if (SDL_ConvertAudio(&_cvt) < 0)
        return nullptr;
    out_sz = _cvt.len_cvt;
    return &_buf[0];
}

//-----------------------------------------------------------------------------
// SDLDecoder
//-----------------------------------------------------------------------------
const auto SampleDefaultBufferSize = 64 * 1024;

SDLDecoder::SDLDecoder(std::shared_ptr<std::vector<uint8_t>> &data,
    const AGS::Common::String &ext_hint, bool repeat)
    : _sampleData(data)
    , _sampleExt(ext_hint)
    , _repeat(repeat)
{
}

SDLDecoder::SDLDecoder(std::unique_ptr<Stream> in,
    const AGS::Common::String &ext_hint, bool repeat)
    : _rwops(SDL2Util::OpenRWops(std::move(in)))
    , _sampleExt(ext_hint)
    , _repeat(repeat)
{
}

SDLDecoder::SDLDecoder(SDLDecoder &&dec)
{
    _sampleData = (std::move(dec._sampleData));
    _rwops = std::move(dec._rwops);
    dec._rwops = nullptr;
    _sampleExt = std::move(dec._sampleExt);
    _repeat = dec._repeat;
}

bool SDLDecoder::Open(float pos_ms)
{
    // Prevent from "reopening" twice
    assert(!_sample);
    if (_sample && pos_ms > 0.f)
    {
        Seek(pos_ms);
        return true;
    }

    SoundSampleUniquePtr sample{};
    if (_rwops)
    {
        sample = SoundSampleUniquePtr(Sound_NewSample(_rwops,
            _sampleExt.GetCStr(), nullptr, SampleDefaultBufferSize));
    }
    else
    {
        sample = SoundSampleUniquePtr(Sound_NewSampleFromMem(
            _sampleData->data(), _sampleData->size(), _sampleExt.GetCStr(), nullptr, SampleDefaultBufferSize));
    }
    if (!sample)
    {
        _rwops = nullptr; // rwops was closed by the Sound_NewSample
        _sampleData = nullptr;
        return false;
    }

    _sample = std::move(sample);
    int dur = Sound_GetDuration(_sample.get()); // may return -1 for unknown
    _durationMs = dur > 0 ? static_cast<float>(dur) : 0.f;
    _posBytes = 0u;
    _posMs = 0.f;
    if (pos_ms > 0.f) {
        Seek(pos_ms);
    }
    return true;
}

void SDLDecoder::Close()
{
    _sample.reset();
    _rwops = nullptr; // rwops was closed by the Sound_NewSample
    _sampleData = nullptr;
}

float SDLDecoder::Seek(float pos_ms)
{
    if (!_sample || pos_ms < 0.f)
        return _posMs;
    if (Sound_Seek(_sample.get(), static_cast<uint32_t>(pos_ms)) == 0)
        return _posMs; // old pos on failure (CHECKME?)
    _posMs = pos_ms;
    _posBytes = SoundHelper::BytesPerMs(_posMs,
        _sample->desired.format, _sample->desired.channels, _sample->desired.rate);
    return pos_ms; // new pos on success
}

SoundBuffer SDLDecoder::GetData()
{
    if (!_sample || _EOS)
        return SoundBuffer();
    float old_pos = _posMs;
    size_t sz = 0;
    do
    {
        sz = Sound_Decode(_sample.get());
        _posBytes += sz;
        _posMs = SoundHelper::MillisecondsFromBytes(_posBytes,
            _sample->desired.format, _sample->desired.channels, _sample->desired.rate);
        // If we reached end of stream, or read less than the buffer size
        // (in which case it may also be decode error), then finish playing
        if ((_sample->flags & SOUND_SAMPLEFLAG_EOF) || (sz < _sample->buffer_size))
        {
            _EOS = true;
            if ((_sample->flags & SOUND_SAMPLEFLAG_ERROR) != 0)
                return SoundBuffer();
            // if repeat, then seek to start.
            else if (_repeat) {
                _EOS = Sound_Rewind(_sample.get()) == 0;
                _posBytes = 0u;
                _posMs = 0.f;
            }
        }
    } while (!_EOS && (sz == 0));
    return SoundBuffer(_sample->buffer, sz, old_pos,
        SoundHelper::MillisecondsFromBytes(sz, _sample->desired.format, _sample->desired.channels, _sample->desired.rate));
}

} // namespace Engine
} // namespace AGS
