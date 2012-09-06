
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
#elif defined(LINUX_VERSION) || defined(MAC_VERSION)
extern "C" int fli_callback();
#else
int fli_callback(...);
#endif

#endif // __AGS_EE_MEDIA__VIDEO_H
