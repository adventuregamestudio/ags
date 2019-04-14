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
#ifndef __AGS_CN_UTIL__STDTR1COMPAT_H
#define __AGS_CN_UTIL__STDTR1COMPAT_H

#if defined(WINDOWS_VERSION)
    // MSVC didn't report correct __cplusplus value until 2017
    #if !defined(_MSC_VER) || (_MSC_VER < 1900)
    #error Visual Studio 2015 or later required.
    #endif
#else
    #if __cplusplus < 201103L
    #error C++11 or later required.
    #endif
#endif

#define MAKE_HEADER(arg) <arg>
#define TR1INCLUDE(arg) MAKE_HEADER(arg) // default for C++11 compilers and MSVC (no tr1 folder)

namespace std {}
namespace stdtr1compat = std;

#endif // __AGS_CN_UTIL__STDTR1COMPAT_H
