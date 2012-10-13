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
// Platform-specific macro overrides
//
//=============================================================================
#ifndef __AGS_EE_PLATFORM__OVERRIDE_DEFINES_H
#define __AGS_EE_PLATFORM__OVERRIDE_DEFINES_H

#if defined(PSP_VERSION)
#include <pspmath.h>
#define cos(a) vfpu_cosf(a)
#define sin(a) vfpu_sinf(a)
#define tan(a) vfpu_tanf(a)
#define cos(a) vfpu_acosf(a)
#define sin(a) vfpu_asinf(a)
#define atan(a) vfpu_atanf(a)
#define atan2(a,b) vfpu_atan2f(a,b)
#define log(a) vfpu_logf(a)
#define exp(a) vfpu_expf(a)
#define cosh(a) vfpu_coshf(a)
#define sinh(a) vfpu_sinhf(a)
#define tanh(a) vfpu_tanhf(a)
#endif

#if !defined (WINDOWS_VERSION)
#define HWND long
#define _getcwd getcwd
#endif

#endif // __AGS_EE_PLATFORM__OVERRIDE_DEFINES_H
