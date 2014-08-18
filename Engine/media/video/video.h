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
//
//
//=============================================================================
#ifndef __AGS_EE_MEDIA__VIDEO_H
#define __AGS_EE_MEDIA__VIDEO_H

void calculate_destination_size_maintain_aspect_ratio(int vidWidth, int vidHeight, int *targetWidth, int *targetHeight);
void play_theora_video(const char *name, int skip, int flags);
void pause_sound_if_necessary_and_play_video(const char *name, int skip, int flags);

#if defined(WINDOWS_VERSION)
int __cdecl fli_callback();
#else
extern "C" int fli_callback();
#endif

#endif // __AGS_EE_MEDIA__VIDEO_H
