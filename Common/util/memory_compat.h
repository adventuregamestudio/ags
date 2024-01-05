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
//
// For compatibility with different compilers that may or not support
// particular new C++ and STL functions.
//
//=============================================================================
#ifndef __AGS_CN_UTIL__MEMORY_COMPAT_H
#define __AGS_CN_UTIL__MEMORY_COMPAT_H
#include <memory>

// C++14 features
#if __cplusplus < 201402L && !((defined(_MSC_VER) && _MSC_VER >= 1900))
namespace std
{
    template<typename T, typename... Args>
    std::unique_ptr<T> make_unique(Args&&... args)
    {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }
} // std
#endif

// C++17 features
#if __cplusplus < 201703L && !((defined(_MSC_VER) && _MSC_VER >= 1900))
namespace std
{
    template <class C> 
    constexpr auto size(const C& c) -> decltype(c.size())
    {
        return c.size();
    }

    template <class T, std::size_t N>
    constexpr std::size_t size(const T (&array)[N]) noexcept
    {
        return N;
    }
}
#endif

#endif // __AGS_CN_UTIL__MEMORY_COMPAT_H
