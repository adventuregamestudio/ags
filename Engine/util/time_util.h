//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// Various time-related utilities
//
//=============================================================================
#ifndef __AGS_EE_UTIL_TIMEUTIL_H
#define __AGS_EE_UTIL_TIMEUTIL_H

#include <type_traits>
#include <chrono>

namespace AGS
{
namespace Engine
{

// Clock is a high-resolution clock, used when we need max precision.
// Use high resolution clock only if we know it is monotonic/steady.
// refer to https://stackoverflow.com/a/38253266/84262
using Clock = std::conditional<
    std::chrono::high_resolution_clock::is_steady,
    std::chrono::high_resolution_clock, std::chrono::steady_clock
>::type;
// FastClock is a higher performance clock, used when the speed is a priority,
// but precision may be sacrificed.
using FastClock = std::chrono::system_clock;

template <typename TDur>
inline int64_t ToMilliseconds(TDur dur)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
}

template <typename TDur>
inline float ToMillisecondsF(TDur dur)
{
    return std::chrono::duration_cast<std::chrono::microseconds>(dur).count() * 0.001;
}

template <typename TClock>
class TStopwatch final
{
public:
    using Duration = typename TClock::duration;
    using TimePoint = typename TClock::time_point;

    TStopwatch()
    {
        _start = TClock::now();
    }

    void Start()
    {
        _start = TClock::now();
    }

    Duration Check() const
    {
        return TClock::now() - _start;
    }

private:
    TimePoint _start;
};

typedef TStopwatch<Clock> Stopwatch;
typedef TStopwatch<FastClock> FastStopwatch;

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_UTIL_TIMEUTIL_H
