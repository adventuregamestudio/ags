//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// Threading utilities
//
//=============================================================================
#ifndef __AGS_EE_UTIL__THREADING_H
#define __AGS_EE_UTIL__THREADING_H
#include <condition_variable>
#include <mutex>

namespace AGS
{
namespace Engine
{
    
// LockedObjectPtr wraps an object pointer guarded by a mutex lock.
// Unlocks the mutex on destruction (e.g. when going out of scope).
// Optionally accepts a condition variable, sends notification on destruction.
template <typename T>
class LockedObjectPtr
{
public:
    LockedObjectPtr(T *object, std::unique_lock<std::mutex> &&ulk, std::condition_variable *cv)
        : _object(object)
        , _ulock(std::move(ulk))
        , _cv(cv)
    {
    }

    LockedObjectPtr(LockedObjectPtr<T> &&lock)
        : _object(lock._object)
        , _ulock(std::move(lock._ulock))
        , _cv(lock._cv)
    {
        lock._cv = nullptr;
    }

    ~LockedObjectPtr()
    {
        if (_cv)
            _cv->notify_all();
    }

    operator bool() const { return _object != nullptr; }
    const T *operator ->() const { return _object; }
    T *operator ->() { return _object; }
    const T &operator *() const { return *_object; }
    T &operator *() { return *_object; }

private:
    T *_object = nullptr;
    std::unique_lock<std::mutex> _ulock;
    std::condition_variable *_cv = nullptr;
};

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_UTIL__THREADING_H
