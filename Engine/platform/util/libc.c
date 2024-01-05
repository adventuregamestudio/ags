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
// Implementations for missing libc functions.
// Some of these were required for old mobile SDK/NDKs, which missed
// particular string functions. Make certain to keep this updated,
// and link STRICTLY on platforms that require these.
//
//=============================================================================

#include "core/platform.h"

#if (0)

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <ctype.h>

// This is a pure mbstowcs placeholder that copies src to dest,
// not a real implementation
size_t mbstowcs(wchar_t *wcstr, const char *mbstr, size_t max)
{
  size_t count = 0;
  // Copy until reached the end of dest buffer, or 0
  while ((count < max) && (*mbstr != 0))
  {
    *wcstr++ = *mbstr++;
    count++;
  }
  // Terminate the string, if possible
  if (count < max)
    *wcstr = 0;
  return count;
}

// This is a pure wcstombs placeholder that copies src to dest,
// not a real implementation
size_t wcstombs(char* mbstr, const wchar_t *wcstr, size_t max)
{
  size_t count = 0;
  // Copy until reached the end of dest buffer, or 0
  while ((count < max) && (*wcstr != 0))
  {
    *mbstr++ = *wcstr++;
    count++;
  }
  // Terminate the string, if possible
  if (count < max)
    *mbstr = 0;
  return count;
}

#endif // ! 0
