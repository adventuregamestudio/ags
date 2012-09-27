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

#ifdef WINDOWS_VERSION
// nothing
#elif defined(LINUX_VERSION) || defined(MAC_VERSION)

#define HWND long
#define _getcwd getcwd

#else   // it's DOS (DJGPP)

#define _getcwd getcwd

#endif

#endif // __AGS_EE_PLATFORM__OVERRIDE_DEFINES_H
