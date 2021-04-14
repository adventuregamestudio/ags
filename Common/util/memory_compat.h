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

#endif // __AGS_CN_UTIL__MEMORY_COMPAT_H
