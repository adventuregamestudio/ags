/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Android specific header defines.
 *
 *      By JJS for the Adventure Game Studio runtime port.
 *      Based on the Allegro PSP port.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALANDA_H
#define ALANDA_H

#ifndef ALLEGRO_ANDROID
   #error bad include
#endif

#include <stdio.h>
#include <jni.h>

/* The Android C library doesn't include stricmp */
#define stricmp strcasecmp

/* System driver */
#define SYSTEM_ANDROID              AL_ID('A','N','D',' ')
AL_VAR(SYSTEM_DRIVER, system_android);

/* Timer driver */
#define TIMER_ANDROID               AL_ID('A','N','D','T')
AL_VAR(TIMER_DRIVER, timer_android);

/* Keyboard driver */
#define KEYBOARD_ANDROID            AL_ID('A','N','D','K')
AL_VAR(KEYBOARD_DRIVER, keyboard_android);

/* Mouse drivers */
#define MOUSE_ANDROID               AL_ID('A','N','D','M')
AL_VAR(MOUSE_DRIVER, mouse_android);

/* Gfx driver */
#define GFX_ANDROID                 AL_ID('A','N','D','G')
AL_VAR(GFX_DRIVER, gfx_android);

/* Digital sound driver */
#define DIGI_ANDROID                AL_ID('A','N','D','S')
AL_VAR(DIGI_DRIVER, digi_android);

/* Joystick drivers */
#define JOYSTICK_ANDROID            AL_ID('A','N','D','J')
AL_VAR(JOYSTICK_DRIVER, joystick_android);


#ifdef __cplusplus
   extern "C" {
#endif

AL_FUNC(void, android_allegro_initialize_jni, (JNIEnv* env, jclass java_class, jobject java_object));

#ifdef __cplusplus
   }
#endif



#endif
