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
// Includes <windows.h> and deals with the potential macro conflicts.
//
//=============================================================================
#ifndef __AGS_CN_PLATFORM__WINDOWS_H
#define __AGS_CN_PLATFORM__WINDOWS_H
#include "core/platform.h"

#if AGS_PLATFORM_OS_WINDOWS
#ifndef _WINDOWS_ // do not include if windows.h was included before

#define NOMINMAX
#define BITMAP WINDOWS_BITMAP

#include <windows.h>

#undef BITMAP
#undef CreateFile
#undef CreateDirectory
#undef DeleteFile
#undef GetCurrentDirectory
#undef SetCurrentDirectory

#endif // _WINDOWS_
#endif // AGS_PLATFORM_OS_WINDOWS

#endif // __AGS_CN_PLATFORM__WINDOWS_H
