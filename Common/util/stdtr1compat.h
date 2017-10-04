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

#define MAKE_HEADER(arg) <arg>
#define TR1INCLUDE(arg) MAKE_HEADER(arg) // default for C++11 compilers and MSVC (no tr1 folder)

#if __cplusplus >= 201103L
// C++11, doesn't need TR1
namespace std {}
namespace stdtr1compat = std;
#elif defined(_MSC_VER)
#if _MSC_VER < 1600
// MSVC prior to VS2010 needs TR1
#define AGS_NEEDS_TR1
#define AGS_NEEDS_TR1_MSVC // additional macro because MSVC headers aren't in tr1 folder
namespace std { namespace tr1 {} }
namespace stdtr1compat = std::tr1;
#else
// MSVC2010 and later do not need TR1
namespace std {}
namespace stdtr1compat = std;
#endif // _MSC_VER < 1600
#else // !_MSC_VER
// not C++11, needs TR1
#define AGS_NEEDS_TR1
#undef TR1INCLUDE
#define TR1INCLUDE(arg) MAKE_HEADER(tr1/arg) // non-MSVC compilers prior to C++11 need tr1 folder
namespace std { namespace tr1 {} }
namespace stdtr1compat = std::tr1;
#endif // C++11

#endif // __AGS_CN_UTIL__STDTR1COMPAT_H
